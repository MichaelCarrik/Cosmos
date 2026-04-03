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


void parseConfig(boost::property_tree::ptree const &pt, std::map<std::string, Cosmos::Types::InitParam> &configParamMap) {
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
        configParamMap[ param.engineName] = param;
    }
}


void parseNetParams(std::string & receiveData, Cosmos::Types::NetModifyParam * netModifyParam) {
    std::string separator = ",";
    unsigned int start = 0;
    auto index = receiveData.find_first_of(separator, start);
    std::vector<std::string> line_vector;
    std::string substring = "";
    do {

        if (index != std::string::npos) {
            substring = receiveData.substr(start, index - start);

            line_vector.emplace_back(substring);
            start = index + separator.size();
            index = receiveData.find(separator, start);


            if (start == std::string::npos) {
                break;
            }

        }
    } while (index != std::string::npos);

    if (line_vector.size() == 4) {
        netModifyParam->engineName = line_vector[0];
        netModifyParam->subPolicyName = line_vector[1];
        netModifyParam->paramName = line_vector[2];
        netModifyParam->paramValue = line_vector[3];
    }else if (line_vector.size() == 4) {
        netModifyParam->engineName = line_vector[0];
        netModifyParam->subPolicyName = line_vector[1];
        netModifyParam->paramName = line_vector[2];
        strcpy(netModifyParam->symbolName.data() , line_vector[3].c_str());
        netModifyParam->paramValue = line_vector[4];
    }
}

void updateParams(Cosmos::Types::NetModifyParam * netModifyParam, Cosmos::Types::EventData *eventData,
    std::map<std::string, Cosmos::Engine::UnderlyEngine *> & engines_map) {
    auto itr = engines_map.find(netModifyParam->engineName);
    if (itr == engines_map.end()) {

        return;
    }
    eventData->point = netModifyParam;
    eventData->eventType = Cosmos::Types::EventType::paramsEvent;

    itr->second->m_driver->callback_asyncEventData(eventData, itr->second->m_policyID);
}

void Session(boost::asio::ip::tcp::socket socket,  std::map<std::string, Cosmos::Engine::UnderlyEngine *> & engines_map)
{

    try {
        Cosmos::Utils::MemoryList<Cosmos::Types::NetModifyParam,32> netModifyParamList(0);
        Cosmos::Utils::MemoryList<Cosmos::Types::EventData,  32> eventDataList(0);
        while (true) {

            std::string receiveData{""};

            boost::system::error_code ec;
            std::size_t length = socket.read_some(boost::asio::buffer(receiveData), ec);
            fprintf(stderr, "receiveData : %s\n", receiveData.c_str());


            if (ec == boost::asio::error::eof)
            {
                std::cout << "connect is closed by client" << std::endl;
                break;
            }
            else if (ec)
            {

                throw boost::system::system_error(ec);
            }

            auto netModifyParam = netModifyParamList.getNewMemory();
            auto eventData = eventDataList.getNewMemory();
            parseNetParams(receiveData, netModifyParam );


            boost::asio::write(socket, boost::asio::buffer(receiveData, length));
            std::cout<<"echo server send back!"<<std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " <<  e.what() << std::endl;
    }
}


int main(int argc, char *argv[]) {
    int tradingday = atoi(argv[1]);
    std::string config_tradinghours = "tradinghour.xml";
    std::string config_path = "CosmosTrading_test.xml";
    spdlog::init_thread_pool(1024 * 64, 1);
    //  auto daily_logger = spdlog::daily_logger_mt<spdlog::async_factory_nonblock>("daily_logger", "logs/system/daily.txt", 19, 30);
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/system/daily.txt", 19, 30);

    spdlog::set_default_logger(daily_logger);
    spdlog::set_level(spdlog::level::info);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    spdlog::flush_every(std::chrono::seconds(5));

    Cosmos::Utils::TradingHours::loadConfig(config_tradinghours);

    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config_path, pt);
    std::string rawTickPath = pt.get_child("Cosmos").get_child("rawTickPath").get<std::string>("<xmlattr>.value");

    std::string store_config = "mysql.xml";
    Cosmos::Utils::CppMySQL3DB *mySql = new Cosmos::Utils::CppMySQL3DB();
    InitMySql(mySql, store_config);

    std::array<bool, 2> isDayArray{false, true};

    for (auto isDay: isDayArray) {
        Cosmos::Driver::TestDriver driver;
        std::vector<Cosmos::Types::InstrumentInfo> queryInstruments;
        Cosmos::Market::Market<Cosmos::Market::MockMarket, decltype(driver)> market(&driver, rawTickPath);
        Cosmos::Trader::Trader<Cosmos::Trader::MockTrader, decltype(driver)> trader(
            &driver, rawTickPath, tradingday, isDay);
        trader.start(tradingday);
        spdlog::info("initial policies");
        int policyID = 0;
        std::map<std::string, Cosmos::Engine::UnderlyEngine *> engines_map;

        std::map<std::string, Cosmos::Types::InitParam> configParamMap;
        parseConfig(pt, configParamMap);
        for (auto & params : configParamMap) {

           auto underlyngine = new Cosmos::Engine::UnderlyEngine(&driver,
                params.first, policyID++, tradingday, mySql, isDay, false);

            engines_map[params.first] = underlyngine;
            underlyngine->onInitParams(params.second);
        }


        for (auto & iengineItr: engines_map) {
            iengineItr.second->onStart();
        }

        driver.setPolicySize(16);
        market.start(tradingday, isDay);
        driver.onStart();
        while (true) {
            unsigned short port =25000;
            boost::asio::io_context ioc;

            boost::asio::ip::tcp::acceptor acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
            try {
                // 一次处理一个连接
                while (true) {
                    Session(acceptor.accept(),  engines_map);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception: " <<  e.what() << std::endl;
            }
        }
    }
    printf("close");
    return 1;
}
