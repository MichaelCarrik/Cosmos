#include <iostream>

#include "../KData/KDataManager.h"
#include "MemoryList.h"
#include "ReadCtp.h"

int main() {


//
//    std::string config_tradinghours = "tradinghour.xml";
//     Utils::TradingHours::loadConfig(config_tradinghours);
//     KData::KDataManager<  KData::KData,  Types::MarketData> klineManager;
//
//     Types::Instrument_t instrument1{"ag2102"};
//     Utils::TradingHours::initInstrumentTradingHours(instrument1);
//    klineManager.initKSeries(instrument1, 20201217);
//
//    std::vector< Types::MarketData> tickVector;
//
//     ReadCtp::read_tick("/home/zhangyw/raw_tick/ag2102_20201217_night.txt", tickVector);
//     ReadCtp::read_tick("/home/zhangyw//raw_tick/ag2102_20201217_day.txt", tickVector);
//    for(auto &pMD : tickVector){
//     //   printf("pmd, updateTime=%s, milliseconds=%d\n",pMD.updateTime.data(), pMD.milliSeconds);
//        klineManager.addTick(&pMD);
//    }
//
//    auto kseries_itr = klineManager.m_allKLineSeries.find(instrument1);
//    for(auto se_itr = kseries_itr->second->begin(); se_itr != kseries_itr->second->end(); se_itr++){
//
//     //    klineManager.m_database.saveKData(instrument1,se_itr->second->m_Period, se_itr->second->m_kseries);
//
//
//    }

    return 0;
}
