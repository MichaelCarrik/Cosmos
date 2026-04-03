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

static std::shared_ptr<spdlog::logger>
initLogs(std::string &typeName, std::string &engineName, std::string &policyName) {
    char logSymbolPath[128];
    memset(logSymbolPath, 0, sizeof(128));
    sprintf(logSymbolPath, "./logs/%s/%s_%s.txt", typeName.c_str(), engineName.c_str(), policyName.c_str());

    std::filesystem::path tradeSymbolPath(logSymbolPath);
    if (!std::filesystem::exists(tradeSymbolPath.parent_path())) {
        std::filesystem::create_directories(tradeSymbolPath.parent_path());
    }
    char logKey[128]{""};
    sprintf(logKey, ".%s_%s_%s", typeName.c_str(), engineName.c_str(), policyName.c_str());
    fprintf(stderr, "initLogs %s %s\n", logKey, logSymbolPath);

    return spdlog::basic_logger_st(logKey, logSymbolPath);
}






int main(int argc, char *argv[]) {

    std::string config_tradinghours = "tradinghour.xml";
    std::string config_path = "OptionTrading.xml";
    spdlog::init_thread_pool(1024 * 64, 1);
    //  auto daily_logger = spdlog::daily_logger_mt<spdlog::async_factory_nonblock>("daily_logger", "logs/system/daily.txt", 19, 30);
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/system/daily.txt", 19, 30);

    spdlog::set_default_logger(daily_logger);
    spdlog::set_level(spdlog::level::info);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    spdlog::flush_every(std::chrono::seconds(5));

     Utils::TradingHours::loadConfig(config_tradinghours);

    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config_path, pt);
     Driver::RealtimeDriver driver;
    driver.setPolicySize(22);
    //initial md
    fprintf(stderr, "init market begin\n");
    spdlog::info("initial md");
    auto md_config = pt.get_child("OptionTrading").get_child("md").get<std::string>("<xmlattr>.configfile");
    //    Market::Market< Market::MockMarket> market(&driver, md_config);
     Market::Market< Market::CtpMarket, decltype(driver)> market(&driver, md_config);

    auto trade_config = pt.get_child("OptionTrading").get_child("trade").get<std::string>("<xmlattr>.configfile");
//    TradeBots::Trader::Trader<TradeBots::Trader::RemTraderSpi> trader(&driver, trade_config, symbolConfigMap.size());
     Trader::Trader< Trader::CtpTrader, decltype(driver)> trader(&driver, trade_config);
    fprintf(stderr, "trade end\n");


    std::string store_config = "mysql.xml";
     Utils::CppMySQL3DB *mySql = new  Utils::CppMySQL3DB();
    InitMySql(mySql, store_config);

    int tradingday = 0;
    if (trader.start(tradingday) != 0) {
        fprintf(stderr, "trade error\n");
        spdlog::error("trader start error, program terminal");
        return -1;
    };
    spdlog::info("trader login successfully");


    spdlog::info("initial policies");
    int policyID = 0;
    std::vector< UnderlyOptionEngine::UnderlyOptionEngine *> engines_vec;

    for (auto policy_pt: boost::make_iterator_range(
            pt.get_child("OptionTrading").get_child("Engines").equal_range("Engine"))) {
        auto engineName = policy_pt.second.get<std::string>("<xmlattr>.name");
        spdlog::info("initial policy {}", engineName.c_str());
        auto underlyOptionEngine = new  UnderlyOptionEngine::UnderlyOptionEngine(&driver, engineName, mySql,
                                                                                             is_day(), true);  //TradeBots::Engine::IPolicyFactory::CreateIPolicy();
        underlyOptionEngine->m_policyID = policyID++;
        underlyOptionEngine->m_tradingDay = tradingday;
        engines_vec.emplace_back(underlyOptionEngine);
        for (auto param_pt: boost::make_iterator_range(policy_pt.second.get_child("params").equal_range("param"))) {
             Types::Param param;
            param.name = param_pt.second.get<std::string>("<xmlattr>.name");
            param.value = param_pt.second.get<std::string>("<xmlattr>.value");
            underlyOptionEngine->onParams(param);
        }
        for (auto subPolicy_pt: boost::make_iterator_range(
                policy_pt.second.get_child("params").equal_range("subPolicy"))) {
             Types::Param param;
            param.name = "subPolicy";
            for (auto subParam_pt: boost::make_iterator_range(subPolicy_pt.second.equal_range("subParam"))) {
                auto subPolicyParamName = subParam_pt.second.get<std::string>("<xmlattr>.name");
                if (strcmp(subPolicyParamName.c_str(), "policyName") == 0) {
                    param.value = subParam_pt.second.get<std::string>("<xmlattr>.value");
                }
                param.paramMap[subPolicyParamName] = subParam_pt.second.get<std::string>("<xmlattr>.value");
            }
            underlyOptionEngine->onParams(param);
        }
    }


    for (auto iengine: engines_vec) {
         Types::LogStruct *logStruct = new  Types::LogStruct();
        std::string record = "record";
        logStruct->recordLog = initLogs(record, iengine->m_engineName, record);

        std::string position = "position";
        logStruct->m_positionLog = initLogs(position, iengine->m_engineName, position);

        iengine->setInitLog(*logStruct);
        iengine->onStart();
    }

    auto initMarketVec = trader.getInitMarketVec();
    market.start(*initMarketVec);
    driver.onStart();


    while (true) {
        sleep(5 * 60);
    }
    return 1;

}
