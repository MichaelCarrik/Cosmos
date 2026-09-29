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


void getInstruments(int tradingDay, std::string &etfProductId, std::string &rawPath,
                    std::vector<Cosmos::Types::InstrumentInfo> &optionSymbols) {
    char insInfoBuff[256]{""};
    sprintf(insInfoBuff, rawPath.c_str(), etfProductId.c_str());
    std::filesystem::path filePath(insInfoBuff);
    if (!std::filesystem::exists(filePath)) {
        fprintf(stderr, "path is not exists %s\n", filePath.c_str());
        return;
    }

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
            if (line_vector.size() == 8 && atoi(line_vector[2].c_str()) <= tradingDay &&
                tradingDay <= atoi(line_vector[3].c_str()) &&
                strcmp(line_vector[6].c_str(), etfProductId.c_str()) == 0 ) {
                Cosmos::Types::InstrumentInfo instrumentInfo;
                strcpy(instrumentInfo.instrumentID.data(), line_vector[0].substr(0,8).c_str());
                instrumentInfo.productIDClass = Cosmos::Types::ProductClass::option; // atoi(line_vector[6].c_str());
                instrumentInfo.exchanges = Cosmos::Types::ExchangeType::SHSZ;
                strcpy(instrumentInfo.productID.data(), etfProductId.c_str());
                instrumentInfo.optionType = line_vector[1][0];
                instrumentInfo.strikePrice = atof(line_vector[4].c_str());
                instrumentInfo.expireDate = atoi(line_vector[3].c_str());
                strcpy(instrumentInfo.underly.data(), etfProductId.c_str());
                instrumentInfo.multi = 10000;
                instrumentInfo.tickSize = 0.0001;
                optionSymbols.emplace_back(instrumentInfo);
            }
        }
        i++;
    }
}


int main(int argc, char *argv[]) {
    int tradingDay = atoi(argv[1]);
    std::string productid = argv[2];
    std::string fileName = argv[3];
    fprintf(stderr, "tradingday=%d, productid=%s, fileName=%s\n", tradingDay, productid.c_str(), fileName.c_str());
    //  std::string config_tradinghours = "tradinghour.xml";
    std::string config_path = "CosmosKBarETFRead.xml";
    spdlog::init_thread_pool(1024 * 64, 1);
    //  auto daily_logger = spdlog::daily_logger_mt<spdlog::async_factory_nonblock>("daily_logger", "logs/system/daily.txt", 19, 30);
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/system/daily.txt", 19, 30);

    spdlog::set_default_logger(daily_logger);
    spdlog::set_level(spdlog::level::info);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    spdlog::flush_every(std::chrono::seconds(5));

    //     Utils::TradingHours::loadConfig(config_tradinghours);

    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config_path, pt);
    auto config_tradinghours = pt.get_child("Cosmos").get_child("tradinghours").get<
        std::string>("<xmlattr>.configfile");
    Cosmos::Utils::TradingHours::loadConfig(config_tradinghours);

    std::string rawTickPath = pt.get_child("Cosmos").get_child("params").get_child("rawTickPath").get<std::string>(
        "<xmlattr>.value");

    std::string optionDescripePath = pt.get_child("Cosmos").get_child("params").get_child("optionDescripePath").get<
        std::string>(
        "<xmlattr>.value");

    std::string savePath = pt.get_child("Cosmos").get_child("params").get_child("savePath").get<std::string>(
        "<xmlattr>.value");
    std::string isUseUnderlyPrice = pt.get_child("Cosmos").get_child("params").get_child("isUseUnderlyPrice").get<std::string>(
    "<xmlattr>.value");

    std::string engineName{"KBarReadEngine"};


    char read_path[256]{""};
    Cosmos::Driver::TestDriver driver;
    //    fprintf(stderr,"%d_%s\n",tradingday, isDay== false ? "ngt":"day");
    std::vector<Cosmos::Types::InstrumentInfo> queryOptionInstruments;
    std::vector<Cosmos::Types::InstrumentInfo> queryFutureInstruments;

    if (strcmp(productid.c_str(), "000016") != 0 && strcmp(productid.c_str(), "000300") != 0 &&
        strcmp(productid.c_str(), "000905") != 0 &&   strcmp(productid.c_str(), "000852") != 0) {

        sprintf(read_path, "%s/%s/%d", rawTickPath.c_str(), productid.c_str(), tradingDay);
        std::filesystem::path filePath(read_path);
        if (std::filesystem::exists(filePath) == false) {
            printf("close");
            return 1;
        }

        // std::set< Types::Instrument_t> queryInstruments{ Types::Instrument_t {"au2108"}} ;
        getInstruments(tradingDay, productid, optionDescripePath, queryOptionInstruments);
    }else {
        rawTickPath =  pt.get_child("Cosmos").get_child("params").get_child("rawIndexTickPath").get<std::string>(
        "<xmlattr>.value");
    }

    Cosmos::Types::InstrumentInfo futureInInfo;
    strcpy(futureInInfo.instrumentID.data(), productid.c_str());
    futureInInfo.productIDClass = Cosmos::Types::ProductClass::future; // atoi(line_vector[6].c_str());
    futureInInfo.exchanges = Cosmos::Types::ExchangeType::SHSZ;
    strcpy(futureInInfo.productID.data(), productid.c_str());
    strcpy(futureInInfo.underly.data(), productid.c_str());
    futureInInfo.multi = 100;
    futureInInfo.tickSize = 0.001;
    queryFutureInstruments.emplace_back(futureInInfo);


    Cosmos::Market::Market<Cosmos::Market::MockMarket, decltype(driver)> market(&driver, rawTickPath, productid, false);

    Cosmos::KBarSaverEngine::KBarReadEngine saveEngine(&driver, engineName, queryOptionInstruments,
                                                       queryFutureInstruments, tradingDay, true, savePath, atoi(isUseUnderlyPrice.c_str()));
    driver.setPolicySize(2);
    saveEngine.m_policyID = 0;
    saveEngine.onStart();
    // auto log_epoch_time = std::chrono::duration_cast<std::chrono::seconds>(
    //      std::chrono::system_clock::now().time_since_epoch()).count();
    market.start(tradingDay, true);
    // fprintf(stderr, "consume TIME = %d\n", std::chrono::duration_cast<std::chrono::seconds>(
    //      std::chrono::system_clock::now().time_since_epoch()).count()- log_epoch_time);
    driver.onStart();

    saveEngine.dumpKline(fileName);


    printf("close");
    return 1;
}
