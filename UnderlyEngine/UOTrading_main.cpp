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
     Cosmos::Market::Market< Cosmos::Market::CtpMarket, decltype(driver)> market(&driver, md_config);

    auto trade_config = pt.get_child("Cosmos").get_child("trade").get<std::string>("<xmlattr>.configfile");
//    TradeBots::Trader::Trader<TradeBots::Trader::RemTraderSpi> trader(&driver, trade_config, symbolConfigMap.size());
     Cosmos::Trader::Trader< Cosmos::Trader::CtpTrader, decltype(driver)> trader(&driver, trade_config);
    fprintf(stderr, "trade end\n");


    std::string store_config = "mysql.xml";
     Cosmos::Utils::CppMySQL3DB *mySql = new  Cosmos::Utils::CppMySQL3DB();
    InitMySql(mySql, store_config);
    bool isDay = Cosmos::Utils::is_day();
    int tradingDay = 0;
    if (trader.start(tradingDay, isDay) != 0) {
        fprintf(stderr, "trade error\n");
        spdlog::error("trader start error, program terminal");
        return -1;
    };
    spdlog::info("trader login successfully");


    spdlog::info("initial policies");
    int policyID = 0;
    std::map<std::string, Cosmos::Engine::UnderlyEngine *> engines_map;
    std::map<std::string, Cosmos::Types::InitParam> configParamMap;
    parseConfig(pt, configParamMap);


    for (auto & params : configParamMap) {
        auto underlyngine = new Cosmos::Engine::UnderlyEngine(&driver,
             params.first, policyID++, tradingDay, mySql, isDay, false);
        engines_map[params.first] = underlyngine;
        underlyngine->onInitParams(params.second);
    }



    for (auto & iengineItr: engines_map) {
        iengineItr.second->onStart();
    }

    auto initMarketVec = trader.getInitMarketVec();
    market.start(*initMarketVec, isDay);
    driver.onStart();


    while (true) {
        sleep(5 * 60);
    }
    return 1;

}
