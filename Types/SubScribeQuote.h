//
// Created by zhangyw on 6/2/19.
//

#ifndef TRADEBOTS_SUBSCRIBEQUOTE_H
#define TRADEBOTS_SUBSCRIBEQUOTE_H

#include "Type.h"
#include "MarketData.h"
#include "../Utils/MemoryList.h"
#include "../Types/KPeriod.h"
#include "../KData/KSeries.h"

namespace Cosmos{
    namespace Types{
        class SubScribeQuote{
        public:
       //     int symbolID;
            int policyID;
            Instrument_t instrumentID{""};
            QuoteType quoteType;
        };

        class OnSubScribeQuote{
        public:
            int policyID;
            Instrument_t instrumentID{""};
        };

        class PushMarket{
        public:
            PushMarket(int index):marketDataList(0),eventDataList(0){};
             Utils::MemoryList<MarketData,  Types::SingeInstrumenBuffSize> marketDataList;
             Utils::MemoryList<EventData,  Types::SingeInstrumenBuffSize> eventDataList;
            std::vector<int> subscribePolicy;
            QuoteType quoteType;
        };
    }
}

#endif //TRADEBOTS_SUBSCRIBEQUOTE_H
