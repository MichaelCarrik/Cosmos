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

static std::shared_ptr<spdlog::logger> initLogs(std::string & typeName, std::string& engineName,   std::string& policyName){
    char logSymbolPath[128];
    memset(logSymbolPath, 0, sizeof(128));
    sprintf(logSymbolPath, "./logs/%s/%s_%s.txt",  typeName.c_str(), engineName.c_str(),  policyName.c_str());

    std::filesystem::path tradeSymbolPath(logSymbolPath);
    if(!std::filesystem::exists(tradeSymbolPath.parent_path())){
        std::filesystem::create_directories(tradeSymbolPath.parent_path());
    }
    char logKey[128]{""};
    sprintf(logKey, ".%s_%s_%s",  typeName.c_str(), engineName.c_str(),  policyName.c_str());
    fprintf(stderr, "initLogs %s %s\n",logKey, logSymbolPath);

    return spdlog::basic_logger_st(logKey, logSymbolPath);
}

void InitMySql( Utils::CppMySQL3DB *mySql, std::string mysql_config) {
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
    }
    catch (...) {
        spdlog::error("got exception when opening the mysql db.");
    }
}

bool is_day(void) {
    auto current_time = std::time(nullptr);
    auto ptm = std::localtime(&current_time);

    return ptm->tm_hour >= 6 && ptm->tm_hour < 18;
}


int main(int argc, char* argv[]) {

    int tradingday = atoi(argv[1]);

    std::string config_tradinghours = "tradinghour.xml";
    std::string config_path = "OptionTrading_test.xml";
    spdlog::init_thread_pool(1024*64, 1);
    //  auto daily_logger = spdlog::daily_logger_mt<spdlog::async_factory_nonblock>("daily_logger", "logs/system/daily.txt", 19, 30);
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/system/daily.txt", 19, 30);

    spdlog::set_default_logger(daily_logger);
    spdlog::set_level(spdlog::level::info);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    spdlog::flush_every(std::chrono::seconds(5));

     Utils::TradingHours::loadConfig(config_tradinghours);

    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config_path, pt);
    std::string rawTickPath = pt.get_child("OptionTrading").get_child("rawTickPath").get<std::string>("<xmlattr>.value");

    std::string store_config = "mysql.xml";
     Utils::CppMySQL3DB *mySql = new  Utils::CppMySQL3DB();
    InitMySql(mySql, store_config);

    std::array<bool ,2> isDayArray{true}; //{false, true};

    for(auto isDay: isDayArray) {
         Driver::TestDriver driver;
        std::vector< Types::InstrumentInfo> queryInstruments;
         Market::Market< Market::MockMarket,  decltype(driver)> market(&driver, rawTickPath);
         Trader::Trader< Trader::MockTrader, decltype(driver)> trader(&driver, rawTickPath, tradingday, isDay);
        trader.start(tradingday);
        spdlog::info("initial policies");
        int policyID = 0;
        std::vector< UnderlyOptionEngine::UnderlyOptionEngine*> engines_vec;

        for (auto policy_pt :boost::make_iterator_range(pt.get_child("OptionTrading").get_child("Engines").equal_range("Engine"))){
            auto engineName = policy_pt.second.get<std::string>("<xmlattr>.name");
            spdlog::info("initial policy {}", engineName.c_str());
            auto underlyOptionEngine= new  UnderlyOptionEngine::UnderlyOptionEngine(&driver,engineName, mySql,isDay );  //TradeBots::Engine::IPolicyFactory::CreateIPolicy();
            underlyOptionEngine->m_policyID = policyID++;
            underlyOptionEngine->m_tradingDay = tradingday;
            engines_vec.emplace_back(underlyOptionEngine);
            for(auto param_pt : boost::make_iterator_range(policy_pt.second.get_child("params").equal_range("param"))){
                 Types::Param param ;
                param.name = param_pt.second.get<std::string>("<xmlattr>.name") ;
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


        for(auto iengine : engines_vec){
             Types::LogStruct * logStruct = new  Types::LogStruct();
            std::string record = "record";
            logStruct->recordLog = initLogs(record, iengine->m_engineName, record);

            std::string position = "position";
            logStruct->m_positionLog = initLogs(position, iengine->m_engineName, position);

            iengine->setInitLog(*logStruct);
            iengine->onStart();
        }

        driver.setPolicySize(16);
        market.start(tradingday,is_day()) ;
        driver.onStart();

    }

    printf("close");
    return 1;

}
