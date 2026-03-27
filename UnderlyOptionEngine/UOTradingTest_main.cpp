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
#include "UnderlyOptionEngine.h"
#include <filesystem>
#include <chrono>
#include "../Types/Param.h"

void InitMySql(Cosmos::Utils::CppMySQL3DB *mySql, std::string mysql_config) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(mysql_config, pt);
    auto host = pt.get_child("mysql.host").get_value<std::string>();
    auto port = pt.get_child("mysql.port").get_value<int>();
    auto dbname = pt.get_child("mysql.database").get_value<std::string>();
    auto username = pt.get_child("mysql.user").get_value<std::string>();
    auto password = pt.get_child("mysql.password").get_value<std::string>();

    int nTried = 0;
    int retCode = -1;
    try {
        do {
            retCode = mySql->open(host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port);
            if (retCode < 0) {
                const char *errorMsg = mysql_error(mySql->getMysql());
                int errorNo = mysql_errno(mySql->getMysql());
                spdlog::error("failed to connect mysql db. please check settings,error no:{},error msg:{}",
                              errorNo, errorMsg);
                sleep(2);
            }
        } while (nTried++ < 2 && retCode < 0);
        if (nTried > 2 and retCode != 0) {
            assert(false && "connect mysql failed");
        }
    } catch (...) {
        spdlog::error("got exception when opening the mysql db.");
    }
}

bool is_day(void) {
    auto current_time = std::time(nullptr);
    auto ptm = std::localtime(&current_time);

    return ptm->tm_hour >= 6 && ptm->tm_hour < 18;
}


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
        std::map<std::string, Cosmos::UnderlyOptionEngine::UnderlyOptionEngine *> engines_map;

        std::map<std::string, Cosmos::Types::InitParam> configParamMap;
        parseConfig(pt, configParamMap);
        for (auto & params : configParamMap) {
            Cosmos::UnderlyOptionEngine::UnderlyOptionEngine *underlyOptionEngine;
            underlyOptionEngine = new Cosmos::UnderlyOptionEngine::UnderlyOptionEngine(&driver,
                params.first, mySql, isDay, false);
            underlyOptionEngine->m_policyID = policyID++;
            underlyOptionEngine->m_tradingDay = tradingday;
            engines_map[params.first] = underlyOptionEngine;
            underlyOptionEngine->onInitParams(params.second);
        }


        for (auto & iengineItr: engines_map) {
            iengineItr.second->onStart();
        }

        driver.setPolicySize(16);
        market.start(tradingday, isDay);
        driver.onStart();
    }
    printf("close");
    return 1;
}
