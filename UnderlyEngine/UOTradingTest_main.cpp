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
#include "../Market/MockMarket.h"
#include "../Market/Market.h"
#include "../Trader/MockTrader.h"
#include "../Trader/Trader.h"
#include "UnderlyEngine.h"
#include <filesystem>
#include <chrono>
#include "../Types/Param.h"
#include <boost/asio.hpp>


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

void parseStringBySeparator(std::string const& originString, std::string&& separator, std::vector<std::string> & resultVec) {

    unsigned int start = 0;
    auto index = originString.find_first_of(separator, start);
    std::string substring = "";
    while (true) {
        if (index != std::string::npos) {
            substring = originString.substr(start, index - start);

            resultVec.emplace_back(substring);
            start = index + separator.size();
            index = originString.find(separator, start);
            if (start == std::string::npos) {
                break;
            }
            if (index == std::string::npos) {
                if (start < originString.size()-1) {
                    resultVec.emplace_back(originString.substr(start, originString.size()-1));
                }
                break;;
            }
        }
    }
}


void parseNetParams(std::string &lingStr, Cosmos::Types::NetModifyParam *netModifyParam) {
    std::vector<std::string> paramVec;
    parseStringBySeparator(lingStr, ",", paramVec);

   if (paramVec.size() == 5) {
        netModifyParam->engineName = paramVec[0];
        netModifyParam->subPolicyName = paramVec[1];
        netModifyParam->paramName = paramVec[2];
        strcpy(netModifyParam->symbolName.data(), paramVec[3].c_str());
        netModifyParam->paramValue = paramVec[4];
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

void Session(Cosmos::Driver::TestDriver * driver ,boost::asio::ip::tcp::socket socket, std::map<std::string, Cosmos::Engine::UnderlyEngine *> &engines_map) {
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
            std::vector<std::string> lineVec;
            parseStringBySeparator(receiveDataStr, "\n", lineVec);
            for (auto &line : lineVec) {
                auto netModifyParam = netModifyParamList.getNewMemory();
                auto eventData = eventDataList.getNewMemory();
                parseNetParams(line, netModifyParam);
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
    int tradingdayBegin = atoi(argv[1]);
    int tradingdayEnd = atoi(argv[2]);

    std::string config_path = "CosmosTrading_test.xml";
    spdlog::init_thread_pool(1024 * 64, 1);

    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/system/daily.txt", 19, 30);
    spdlog::set_default_logger(daily_logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    spdlog::flush_every(std::chrono::seconds(5));

    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config_path, pt);
    std::string rawTickPath = pt.get_child("Cosmos").get_child("rawTickPath").get<std::string>("<xmlattr>.value");
    auto config_tradinghours = pt.get_child("Cosmos").get_child("tradinghours").get<
        std::string>("<xmlattr>.configfile");
    Cosmos::Utils::TradingHours::loadConfig(config_tradinghours);
    std::string store_config = "mysql.xml";
    Cosmos::Utils::CppMySQL3DB *mySql = new Cosmos::Utils::CppMySQL3DB();
    InitMySql(mySql, store_config);

    std::array<bool, 2> isDayArray{false, true};
    for (int tradingDay = tradingdayBegin; tradingDay <= tradingdayEnd; tradingDay++) {
        char read_path[256]{""};
        sprintf(read_path, "%s/%d_day", rawTickPath.c_str(), tradingDay);
        std::filesystem::path filePath(read_path);
        if (std::filesystem::exists(filePath) == false) {
            continue;
        }
        for (auto isDay: isDayArray) {
            fprintf(stderr, "tradingDay=%d, isDay=%d\n", tradingDay, isDay == true ? 1 : 0);
            Cosmos::Driver::TestDriver driver;
         //   std::vector<Cosmos::Types::InstrumentInfo> queryInstruments;
            Cosmos::Market::Market<Cosmos::Market::MockMarket, decltype(driver)> market(&driver, rawTickPath);
            Cosmos::Trader::Trader<Cosmos::Trader::MockTrader, decltype(driver)> trader(
                &driver, rawTickPath, tradingDay, isDay);
            trader.start(tradingDay);
            spdlog::info("initial policies");
            int policyID = 0;
            std::map<std::string, Cosmos::Engine::UnderlyEngine *> engines_map;
            std::map<std::string, Cosmos::Types::InitParam> configParamMap;
            parseConfig(pt, configParamMap);
            for (auto &params: configParamMap) {
                auto underlyngine = new Cosmos::Engine::UnderlyEngine(&driver,
                                                                      params.first, policyID++, tradingDay, mySql,
                                                                      isDay, false);
                engines_map[params.first] = underlyngine;
                underlyngine->onInitParams(params.second);
            }
            for (auto &iengineItr: engines_map) {
                iengineItr.second->onStart();
            }
            driver.setPolicySize(16);
            market.start(tradingDay, isDay);
            driver.onStart();


            fprintf(stderr, "start asio\n");

            unsigned short port = 25000;
            boost::asio::io_context ioc;
            boost::asio::ip::tcp::acceptor acceptor(
                ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
            try {
                // 一次处理一个连接
                while (true) {
                    Session(&driver, acceptor.accept(), engines_map);
                    int a = 1;
                }
            } catch (const std::exception &e) {
                std::cerr << "Exception: " << e.what() << std::endl;
            }
        }
    }


    fprintf(stderr,"close\n");
    return 1;
}
