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
#include "KBarSaverEngine.h"
#include <filesystem>
#include <chrono>
#include "../Types/InstrumentInfo.h"
#include "../Types/LogStruct.h"




bool is_so_loaded(const std::string &so_name) {
    const std::string proc_path = "/proc/self/maps";
    std::ifstream proc(proc_path);
    std::string str;
    while (std::getline(proc, str)) {
        if (str.find(so_name) != std::string::npos) return true;
    }
    return false;
}





void parseConfig(boost::property_tree::ptree const &pt, std::map<std::string, Cosmos::Types::InitParam> &configParamMap) {
    for (auto policy_pt:
         boost::make_iterator_range(pt.get_child("Cosmos").get_child("Engines").equal_range("Engine"))) {
        Cosmos::Types::InitParam param;
        param.engineName = policy_pt.second.get<std::string>("<xmlattr>.name");
        spdlog::info("parseConfig {}", param.engineName.c_str());
        // for (auto param_pt: boost::make_iterator_range(policy_pt.second.get_child("params").equal_range("param"))) {
        //     auto name = param_pt.second.get<std::string>("<xmlattr>.name");
        //     auto value = param_pt.second.get<std::string>("<xmlattr>.value");
        //     param.paramMap[name] = value;
        // }

        configParamMap[ param.engineName] = param;
         }
}

int main() {
    std::string config_tradinghours = "tradinghour.xml";
    std::string config_path = "CosmosKBarSaver.xml";
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

    int tradingday = 0;
    if (trader.start(tradingday) != 0) {
        fprintf(stderr, "trade error\n");
        spdlog::error("trader start error, program terminal");
        return -1;
    };
    spdlog::info("trader login successfully");

    std::vector< Cosmos::Types::InstrumentInfo*>* tradeInsinfoVec{nullptr};
    tradeInsinfoVec = trader.getInstrumentInfoVec();

    spdlog::info("initial policies");
    std::map<std::string, Cosmos::Types::InitParam> configParamMap;
    parseConfig(pt, configParamMap);
    int policyID = 0;
    std::map<std::string, Cosmos::KBarEngine::KBarSaverEngine *> engines_map;

    Cosmos::KBarEngine::KBarSaverEngine *saveEngine = nullptr;

    for (auto & params : configParamMap) {

        auto saveEngine = new Cosmos::KBarEngine::KBarSaverEngine(&driver, params.first ,   mySql,
                             tradeInsinfoVec,  tradingday , Cosmos::Utils::is_day());
        saveEngine->m_policyID = policyID++;
    //    underlyngine->m_tradingDay = tradingday;
        engines_map[params.first] = saveEngine;
        saveEngine->onInitParams(params.second);
    }





    for (auto & iengineItr: engines_map) {
        iengineItr.second->onStart();
    }


    auto initMarketVec = trader.getInitMarketVec();
    market.start(*initMarketVec);
    driver.onStart();


    while (true) {
        sleep( 3600);
        // std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        // std::chrono::minutes minute = std::chrono::duration_cast<std::chrono::minutes>(d);
        // auto today_minutes = (minute.count() + 7 * 60) % (60 * 24);
        // int time1 = 2 * 60 + 30; //02:30
        // int time2 = 11 * 60 + 30; //11:30
        // int time3 = 15 * 60; //15:00
        // // int time3 = 17*60 + 35;
        // if ((today_minutes - time1 > 1 and today_minutes - time1 < 30) ||
        //     (today_minutes - time2 > 1 and today_minutes - time2 < 30) ||
        //     (today_minutes - time3 > 1 and today_minutes - time3 < 30)) {
        //     spdlog::info("save kline in ks_main 1");
        //   //  spdlog::info("save kline in ks_main 1");
        //
        //     for (auto &kv:   engines_vec[0]->m_kDataManager->m_allKLineSeries) {
        //         for (auto &kse: *kv.second) {
        //             // fprintf(stderr, "crontab today_minutes=%d, instrumentid=%s, index=%d, peroid=%d\n", today_minutes,
        //             //         kse.second->m_insInfo.instrumentID.data(), kse.second->m_seriesIndex,
        //             //         Cosmos::Types::KPeroidToIntervalMap[kse.first]);
        //             if (kse.second->m_KDataVecs.size() > kse.second->m_seriesIndex) {
        //                 saveEngine->_saveKline(kse.second, kse.second->m_seriesIndex, kse.first);
        //             }
        //         }
        //     }
        // }
    }
    return 1;
}
