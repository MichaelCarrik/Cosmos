//
// Created by zhangyw on 5/10/19.
//

#ifndef TRADEBOTS_MARKETDATA_H
#define TRADEBOTS_MARKETDATA_H

#include <array>
#include <string.h>
#include "Type.h"

namespace Cosmos{
    namespace Types {
        struct MarketData {
            int64_t epoch_time;
            double lastPrice;
            double midPrice;
            double upperLimitPrice;
            double lowerLimitPrice;
            double settlementPrice;
            double highestPrice;
            double lowestPrice;
            double openPrice;
            std::array<double,5> askPrice;
            std::array<double,5>  bidPrice;
       //     int id;

            int milliSeconds;
            int psSecond;  //seconds *10 + millisconds
            std::array<int,5>  askVolume;
            std::array<int,5> bidVolume;
            double amount;
            double oi;
            int volume;
            Instrument_t instrumentID{""};
            UpdateTime_t  updateTime;
            int isInit{0};
            MarketData() { memset(this, 0, sizeof(MarketData)); };
        };






    };


}




#endif //TRADEBOTS_MARKETDATA_H
