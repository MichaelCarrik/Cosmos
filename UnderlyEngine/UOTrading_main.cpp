//
// Created by zhangyw on 1/19/21.
//


#include "../Driver/RealtimeDriver.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <variant>
#include <boost/range/iterator_range_core.hpp>
#include "../Market/Market.h"
#include "../Market/CtpMarket.h"
#include "../Trader/CtpTrader.h"
#include "../Trader/Trader.h"
#include "UnderlyEngine.h"
#include <filesystem>
#include <chrono>
#include <boost/asio.hpp>
#include <string_view>


void parseConfig(boost::property_tree::ptree const &pt,
                 std::map<std::string, Cosmos::Types::InitParam> &configParamMap) {
    for (auto policy_pt:
         boost::make_iterator_range(pt.get_child("Cosmos").get_child("Engines").equal_range("Engine"))) {
        Cosmos::Types::InitParam param;
        param.engineName = policy_pt.second.get<std::string>("<xmlattr>.name");
        spdlog::info("parseConfig {}", param.engineName.c_str());
        for (auto param_pt: boost::make_iterator_range(policy_pt.second.get_child("params").equal_range("param"))) {
            auto name = param_pt.second.get<std::string>("<xmlattr>.name");
            auto value = param_pt.second.get<std::string>("<xmlattr>.value");
            param.paramMap[name] = value;
        }
        for (auto subPolicy_pt: boost::make_iterator_range(
                 policy_pt.second.get_child("params").equal_range("subPolicy"))) {
            std::map<std::string, std::string> subPolicyParamMap;

            for (auto subParam_pt: boost::make_iterator_range(subPolicy_pt.second.equal_range("subParam"))) {
                auto subPolicyParamName = subParam_pt.second.get<std::string>("<xmlattr>.name");
                subPolicyParamMap[subPolicyParamName] = subParam_pt.second.get<std::string>("<xmlattr>.value");
            }
            param.subPolicyParamsVec.emplace_back(subPolicyParamMap);
        }
        configParamMap[param.engineName] = param;
    }
}

std::string get_readable_time() {
    // 1. 获取当前系统时间点
    auto now = std::chrono::system_clock::now();

    // 2. 转换为 time_t
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // 3. 转换为 tm 结构体 (本地时间)
    std::tm tm_buf;
    localtime_r(&now_c, &tm_buf); // 线程安全版，仅限 Linux/Unix

    // 4. 使用 put_time 格式化
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void parse_csv(std::string const & data, std::vector<std::vector<std::string>>& result) {
    size_t pos = 0;
    size_t prev = 0;

    // 1. 按 '\n' 分割行
    while ((pos = data.find('\n', prev)) != std::string::npos) {
        std::string line = data.substr(prev, pos - prev);

        // 2. 处理行内数据
        std::vector<std::string> row;
        size_t col_pos = 0;
        size_t col_prev = 0;

        while ((col_pos = line.find(',', col_prev)) != std::string::npos) {
            row.emplace_back(line.substr(col_prev, col_pos - col_prev));
            col_prev = col_pos + 1;
        }
        // 最后一个字段
        row.emplace_back(line.substr(col_prev));

        result.push_back(std::move(row));
        prev = pos + 1;
    }

    // 处理最后一行（如果没有以 \n 结尾）
    if (prev < data.length()) {
        std::string line = data.substr(prev);
        std::vector<std::string> row;
        size_t col_pos = 0;
        size_t col_prev = 0;
        while ((col_pos = line.find(',', col_prev)) != std::string::npos) {
            row.emplace_back(line.substr(col_prev, col_pos - col_prev));
            col_prev = col_pos + 1;
        }
        row.emplace_back(line.substr(col_prev));
        result.push_back(std::move(row));
    }
}

void parseNetParams(std::vector<std::string> &paramVec, Cosmos::Types::NetModifyParam *netModifyParam) {

    if (paramVec.size() == 6) {
        netModifyParam->engineName = paramVec[0];
        netModifyParam->subPolicyName = paramVec[1];
        netModifyParam->paramName = paramVec[2];
        strcpy(netModifyParam->underlyInstrument.data(), paramVec[3].c_str());
        strcpy(netModifyParam->symbolName.data(), paramVec[4].c_str());
        netModifyParam->paramValue = paramVec[5];
    }
}

void updateParams(Cosmos::Types::NetModifyParam *netModifyParam, Cosmos::Types::EventData *eventData,
                  std::map<std::string, Cosmos::Engine::UnderlyEngine *> &engines_map) {
    auto itr = engines_map.find(netModifyParam->engineName);
    if (itr == engines_map.end()) {
        return;
    }
    eventData->point = netModifyParam;
    eventData->eventType = Cosmos::Types::EventType::paramsEvent;

    itr->second->m_driver->callback_asyncEventData(eventData, itr->second->m_policyID);
}

void Session(Cosmos::Driver::RealtimeDriver * driver ,boost::asio::ip::tcp::socket socket, std::map<std::string, Cosmos::Engine::UnderlyEngine *> &engines_map) {
    try {
        Cosmos::Utils::MemoryList<Cosmos::Types::NetModifyParam, 32> netModifyParamList(0);
        Cosmos::Utils::MemoryList<Cosmos::Types::EventData, 32> eventDataList(0);
        while (true) {
            char receiveData[65536]{""};
            boost::system::error_code ec;
            std::size_t length = socket.read_some(boost::asio::buffer(receiveData), ec);
            fprintf(stderr, "receiveData : %s\n", receiveData);
            if (ec == boost::asio::error::eof) {
                std::cout << "connect is closed by client" << std::endl;
                break;
            } else if (ec) {
                throw boost::system::system_error(ec);
            }

            auto receiveDataStr=  std::string(receiveData);
            std::vector<std::vector<std::string>> lineVec;
            parse_csv(receiveDataStr,  lineVec);
            auto time = get_readable_time();
            for (auto &line : lineVec) {
                auto netModifyParam = netModifyParamList.getNewMemory();
                auto eventData = eventDataList.getNewMemory();
                parseNetParams(line, netModifyParam);
                netModifyParam->time = time;
                eventData->eventType = Cosmos::Types::EventType::paramsEvent;
                eventData->point = netModifyParam;
                auto itr = engines_map.find(netModifyParam->engineName);
                if (itr == engines_map.end()) {
                    continue;
                }
                driver->callback_asyncEventData(eventData, itr->second->m_policyID);
            }

            boost::asio::write(socket, boost::asio::buffer(receiveData, length));
            std::cout<<"echo server send back!"<<std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}



int main(int argc, char *argv[]) {
  //  std::signal(SIGPIPE, SIG_IGN);
    std::string config_path = "CosmosTrading.xml";
    spdlog::init_thread_pool(1024 * 64, 1);
    //  auto daily_logger = spdlog::daily_logger_mt<spdlog::async_factory_nonblock>("daily_logger", "logs/system/daily.txt", 19, 30);
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/system/daily.txt", 19, 30);

    spdlog::set_default_logger(daily_logger);
    spdlog::set_level(spdlog::level::info);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    spdlog::flush_every(std::chrono::seconds(5));


    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config_path, pt);

    auto config_tradinghours = pt.get_child("Cosmos").get_child("tradinghours").get<
        std::string>("<xmlattr>.configfile");
    Cosmos::Utils::TradingHours::loadConfig(config_tradinghours);

    Cosmos::Driver::RealtimeDriver driver;
    driver.setPolicySize(22);
    //initial md
    fprintf(stderr, "init market begin\n");
    spdlog::info("initial md");
    auto md_config = pt.get_child("Cosmos").get_child("md").get<std::string>("<xmlattr>.configfile");
    //    Market::Market< Market::MockMarket> market(&driver, md_config);
    Cosmos::Market::Market<Cosmos::Market::CtpMarket, decltype(driver)> market(&driver, md_config);

    auto trade_config = pt.get_child("Cosmos").get_child("trade").get<std::string>("<xmlattr>.configfile");
    //    TradeBots::Trader::Trader<TradeBots::Trader::RemTraderSpi> trader(&driver, trade_config, symbolConfigMap.size());
    Cosmos::Trader::Trader<Cosmos::Trader::CtpTrader, decltype(driver)> trader(&driver, trade_config);
    fprintf(stderr, "trade end\n");


    std::string store_config = "mysql.xml";
    Cosmos::Utils::CppMySQL3DB *mySql = new Cosmos::Utils::CppMySQL3DB();
    InitMySql(mySql, store_config);
    bool isDay = Cosmos::Utils::is_day();
    int tradingDay = 0;
    int retry = 0;
    while (trader.start(tradingDay, isDay) != 0) {
        fprintf(stderr, "trade error %d\n", retry);
        spdlog::error("trader start error, program terminal {}", retry);
        if (retry++ >= 3) {
            return -1;
        }
        sleep(90);
    };
    spdlog::info("trader login successfully");


    spdlog::info("initial policies");
    int policyID = 0;
    std::map<std::string, Cosmos::Engine::UnderlyEngine *> engines_map;
    std::map<std::string, Cosmos::Types::InitParam> configParamMap;
    parseConfig(pt, configParamMap);


    for (auto &params: configParamMap) {
        auto underlyngine = new Cosmos::Engine::UnderlyEngine(&driver,
                                                              params.first, policyID++, tradingDay, mySql, isDay, true);
        engines_map[params.first] = underlyngine;
        underlyngine->onInitParams(params.second);
    }


    for (auto &iengineItr: engines_map) {
        iengineItr.second->onStart();
    }

    auto initMarketVec = trader.getInitMarketVec();

    retry = 0;
    while (0 != market.start(*initMarketVec, isDay)) {
        fprintf(stderr, "market error %d\n", retry);
        spdlog::error("market start error, program terminal {}", retry);
        if (retry++ >= 3) {
            return -1;
        }
        sleep(90);
    }

    driver.onStart();

    unsigned short port = 25000;
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

    fprintf(stderr, "start accceptor\n");
    try {
        // 一次处理一个连接
        while (true) {
            Session(&driver, acceptor.accept(), engines_map);
            int a = 1;
        }
    } catch (const std::exception &e) {
        int a = 1;
        std::cerr << "Exception: " << e.what() << std::endl;
    }catch (...) {
        int a = 1;
       // spdlog::error("got exception when opening the mysql db.");
    }


    while (true) {
        sleep(5 * 60);
    }
    return 1;
}
