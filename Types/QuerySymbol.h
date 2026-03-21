//
// Created by zhangyw on 2/1/21.
//

#ifndef TRENDFOLLOW_QUERYSYMBOL_H
#define TRENDFOLLOW_QUERYSYMBOL_H

#include "Symbol.h"

namespace TrendFollow{
    namespace Types {

        class QuerySymbol{
        public:
            int id;
            Instrument_t instrumentID{""};
        };

        class OnQuerySymbol{
        public:
            int id;
            int tradingDay{0};
            Instrument_t instrumentID{""};
            TradePosition tradePosition;
        };


    }
}

#endif //TRENDFOLLOW_QUERYSYMBOL_H
