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
#include "../Utils/Utils.h"
#include "KBarReadEngine.h"
#include <filesystem>
#include <chrono>




void getInstruments(int tradingday, std::string& productid, std::string& rawPath,
                    std::vector< TrendFollow::Types::InstrumentInfo>& optionSymbols,
                    std::vector< TrendFollow::Types::InstrumentInfo>& futureSymbols, int isDay){


    char buff[256]{""};
    sprintf(buff,"%s/%d_%s/instruments", rawPath.c_str(), tradingday, isDay==true? "day": "ngt");
    std::filesystem::path filePath(buff);
    if(!std::filesystem::exists(filePath)){
        fprintf(stderr,"path is not exists %s\n",filePath.c_str());
        return;
    }
    char insInfoBuff[256]{""};
    sprintf(insInfoBuff,"%s/%d_%s/instrument_info.csv", rawPath.c_str(), tradingday, isDay==true? "day": "ngt");
    std::ifstream file;
    std::string strLine;
    file.open(insInfoBuff);
    std::string separator = ",";
    int i = 0;
    while (getline(file, strLine)) {
        if (strLine.empty()) {
            continue;
        }
        if (i >= 1) {
            unsigned int start = 0;
            auto index = strLine.find_first_of(separator, start);
            std::vector<std::string> line_vector;
            std::string substring = "";
            do {

                if (index != std::string::npos) {
                    substring = strLine.substr(start, index - start);

                    line_vector.emplace_back(substring);
                    start = index + separator.size();
                    index = strLine.find(separator, start);

                    if (start == std::string::npos) {
                        break;
                    }
                }
            } while (index != std::string::npos);
            line_vector.emplace_back(strLine.substr(start, index - start));
            if (line_vector.size() < 45 && (atoi(line_vector[6].c_str()) ==2 || atoi(line_vector[6].c_str()) ==6) ) {
                 TrendFollow::Types::InstrumentInfo instrumentInfo;
            //    fprintf(stderr, "%s\n", line_vector[1].c_str());
                line_vector[1].erase(std::remove(line_vector[1].begin(), line_vector[1].end(), '-'), line_vector[1].end());
                strcpy(instrumentInfo.instrumentID.data(), line_vector[1].c_str());
                instrumentInfo.exchanges = TrendFollow::Utils::getExchangeType(line_vector[2].c_str());
                instrumentInfo.productIDClass =   TrendFollow::Types::ProductClass::option;// atoi(line_vector[6].c_str());
                instrumentInfo.expireDate = atoi(line_vector[17].c_str());


                 TrendFollow::Utils::InstrumentToProduct(instrumentInfo.instrumentID, instrumentInfo.productID);
                 TrendFollow::Utils::parseInstruemnt(instrumentInfo.instrumentID, instrumentInfo.underly, instrumentInfo.optionType, instrumentInfo.strikePrice);
                if(strcmp(instrumentInfo.productID.data(), productid.c_str()) ==0){
                    optionSymbols.emplace_back(instrumentInfo);
                }

            }else if(line_vector.size() < 45 && atoi(line_vector[6].c_str()) ==1 ){
                 TrendFollow::Types::InstrumentInfo instrumentInfo;
                strcpy(instrumentInfo.instrumentID.data(), line_vector[1].c_str());
                instrumentInfo.productIDClass =   TrendFollow::Types::ProductClass::future;// atoi(line_vector[6].c_str());
                instrumentInfo.expireDate = atoi(line_vector[17].c_str());
                 TrendFollow::Utils::InstrumentToProduct(instrumentInfo.instrumentID, instrumentInfo.productID);
                if( strcmp(instrumentInfo.productID.data(), productid.c_str()) ==0 ||
                    (strcmp(productid.data(), "MO")==0 && strcmp(instrumentInfo.productID.data(), "IM") ==0) ||
                    (strcmp(productid.data(), "HO")==0 && strcmp(instrumentInfo.productID.data(), "IH") ==0) ||
                    (strcmp(productid.data(), "IO")==0 && strcmp(instrumentInfo.productID.data(), "IF") ==0)
                    ){
                    futureSymbols.emplace_back(instrumentInfo);
                }
            }

        }
        i++;
    }
}


int main(int argc, char* argv[]) {

    int tradingday = atoi(argv[1]);
    std::string productid = argv[2];
    std::string fileName = argv[3];
    fileName += std::string("_") + productid;
    fprintf(stderr, "tradingday=%d, productid=%s, fileName=%s\n", tradingday, productid.c_str(),  fileName.c_str());
  //  std::string config_tradinghours = "tradinghour.xml";
    std::string config_path = "KBarRead.xml";
    spdlog::init_thread_pool(1024*64, 1);
    //  auto daily_logger = spdlog::daily_logger_mt<spdlog::async_factory_nonblock>("daily_logger", "logs/system/daily.txt", 19, 30);
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/system/daily.txt", 19, 30);

    spdlog::set_default_logger(daily_logger);
    spdlog::set_level(spdlog::level::info);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    spdlog::flush_every(std::chrono::seconds(5));

//     Utils::TradingHours::loadConfig(config_tradinghours);

    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config_path, pt);
    auto config_tradinghours = pt.get_child("TrendFollow").get_child("tradinghours").get<std::string>("<xmlattr>.configfile");
     TrendFollow::Utils::TradingHours::loadConfig(config_tradinghours);

    std::string rawTickPath = pt.get_child("TrendFollow").get_child("params").get_child("rawTickPath").get<std::string>("<xmlattr>.value");

    std::string savePath = pt.get_child("TrendFollow").get_child("params").get_child("savePath").get<std::string>("<xmlattr>.value");

    std::string engineName{"KBarReadEngine"};

    std::array<bool ,2> isDayArray{false, true};

    for(auto isDay: isDayArray) {
         TrendFollow::Driver::TestDriver driver;
    //    fprintf(stderr,"%d_%s\n",tradingday, isDay== false ? "ngt":"day");
        std::vector< TrendFollow::Types::InstrumentInfo> queryOptionInstruments;
        std::vector< TrendFollow::Types::InstrumentInfo> queryFutureInstruments;
       // std::set< Types::Instrument_t> queryInstruments{ Types::Instrument_t {"au2108"}} ;
        getInstruments(tradingday,productid, rawTickPath, queryOptionInstruments, queryFutureInstruments, isDay);
        TrendFollow::Market::Market<TrendFollow::Market::MockMarket,  decltype(driver)> market(&driver, rawTickPath);

         TrendFollow::KBarSaverEngine::KBarReadEngine saveEngine(&driver, engineName , queryOptionInstruments,
                                                                queryFutureInstruments, tradingday, isDay ,savePath);
        driver.setPolicySize(2);
        saveEngine.m_policyID =0;
        saveEngine.onStart();
        market.start(tradingday ,isDay) ;
        driver.onStart();

       saveEngine.dumpKline(fileName);
    }

    printf("close");
    return 1;

}
