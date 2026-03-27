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

bool is_day(void) {
    auto current_time = std::time(nullptr);
    auto ptm = std::localtime(&current_time);
    return ptm->tm_hour >= 6 && ptm->tm_hour < 18;
}

bool is_so_loaded(const std::string &so_name) {
    const std::string proc_path = "/proc/self/maps";
    std::ifstream proc(proc_path);
    std::string str;
    while (std::getline(proc, str)) {
        if (str.find(so_name) != std::string::npos) return true;
    }
    return false;
}


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
    int policyID = 0;
    //initial policies
    spdlog::info("initial policies");
    std::vector<Cosmos::KBarEngine::KBarSaverEngine *> engines_vec;
    Cosmos::KBarEngine::KBarSaverEngine *saveEngine = nullptr;
    for (auto policy_pt: boost::make_iterator_range(
             pt.get_child("Cosmos").get_child("Engines").equal_range("Engine"))) {
        auto engineName = policy_pt.second.get<std::string>("<xmlattr>.name");
        if (engineName.find("kbarsaver") != std::string::npos) {
            spdlog::info("initial policy {}", engineName.c_str());
            saveEngine = new Cosmos::KBarEngine::KBarSaverEngine(&driver, engineName, mySql, tradeInsinfoVec, tradingday,  is_day());
            //TradeBots::Engine::IPolicyFactory::CreateIPolicy();
            saveEngine->m_policyID = policyID++;
            engines_vec.emplace_back(saveEngine);
            // for (auto param_pt: boost::make_iterator_range(policy_pt.second.get_child("params").equal_range("param"))) {
            //     Cosmos::Types::Param param;
            //     param.name = param_pt.second.get<std::string>("<xmlattr>.name");
            //     param.value = param_pt.second.get<std::string>("<xmlattr>.value");
            //     saveEngine->onParams(param);
            // }
        }
    }


    for (auto iengine: engines_vec) {
        auto logStruct = new Cosmos::Types::LogStruct();
        std::string record = "record";
        logStruct->recordLog = initLogs(record, iengine->m_engineName, record);

        std::string position = "position";
        logStruct->m_positionLog = initLogs(position, iengine->m_engineName, position);

    //    iengine->setInitLog(*logStruct);
        // auto startTime = std::chrono::duration_cast<std::chrono::microseconds>(
        //   std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        iengine->onStart();
        // auto timeConsume =  std::chrono::duration_cast<std::chrono::microseconds>(
        //        std::chrono::high_resolution_clock::now().time_since_epoch()).count() - startTime;
       // fprintf(stderr,"saveEngine start consume : timeConsume=%d\n", timeConsume);
    }

    auto initMarketVec = trader.getInitMarketVec();
    market.start(*initMarketVec);
    driver.onStart();


    while (true) {
        sleep(1.2 * 60);
        std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::minutes minute = std::chrono::duration_cast<std::chrono::minutes>(d);
        auto today_minutes = (minute.count() + 7 * 60) % (60 * 24);
        int time1 = 2 * 60 + 30; //02:30
        int time2 = 11 * 60 + 30; //11:30
        int time3 = 15 * 60; //15:00
        // int time3 = 17*60 + 35;
        if ((today_minutes - time1 > 1 and today_minutes - time1 < 30) ||
            (today_minutes - time2 > 1 and today_minutes - time2 < 30) ||
            (today_minutes - time3 > 1 and today_minutes - time3 < 30)) {
            spdlog::info("save kline in ks_main 1");
          //  spdlog::info("save kline in ks_main 1");

            for (auto &kv:   engines_vec[0]->m_kDataManager->m_allKLineSeries) {
                for (auto &kse: *kv.second) {
                    // fprintf(stderr, "crontab today_minutes=%d, instrumentid=%s, index=%d, peroid=%d\n", today_minutes,
                    //         kse.second->m_insInfo.instrumentID.data(), kse.second->m_seriesIndex,
                    //         Cosmos::Types::KPeroidToIntervalMap[kse.first]);
                    if (kse.second->m_KDataVecs.size() > kse.second->m_seriesIndex) {
                        saveEngine->_saveKline(kse.second, kse.second->m_seriesIndex, kse.first);
                    }
                }
            }
        }
    }
    return 1;
}
