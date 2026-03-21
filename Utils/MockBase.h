//
// Created by zhangyw on 1/11/21.
//

#ifndef TRENDFOLLOW_MOCKBASE_H
#define TRENDFOLLOW_MOCKBASE_H

#include "../Types/Type.h"
#include "../Types/KPeriod.h"
#include <iostream>
#include <fstream>



namespace TrendFollow {
    namespace DataBase {
        template<typename KLINE>
        class MockDB {
        public:
              int getKData( Types::Instrument_t& Instrument,  Types::KPeriod peroid,  std::vector<KLINE>& m_kseries){

              }

              int saveKData( Types::Instrument_t& Instrument,  Types::KPeriod peroid,  std::vector<KLINE>& m_kseries){
                  char fileName[128]{""};
                  sprintf(fileName, "%d_%s",  Types::KPeroidToSecondsMap[peroid]/60, Instrument.data());
                  std::ofstream destFile(fileName, std::ios::out);
                  int lastVolume =0;
                  for(auto & kline: m_kseries){
                      char lines[256]{""};

                      sprintf(lines, "%d,%s.txt,%s,%d,%.1f,%.1f,%.1f,%.1f,%d", kline.m_tradingday, kline.m_instrument.data(),
                              kline.m_updateTime.data(),   Types::KPeroidToSecondsMap[peroid]/60, kline.m_open,
                              kline.m_high, kline.m_low, kline.m_close, kline.m_volume -lastVolume );
                      destFile<<lines<<"\n";
                      lastVolume=  kline.m_volume;
                  }

                  destFile.close();

              }
        };
    }
}



#endif //TRENDFOLLOW_MOCKBASE_H
