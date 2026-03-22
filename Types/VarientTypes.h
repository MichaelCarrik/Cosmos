//
// Created by zhangyw on 12/18/20.
//

#ifndef HFT_MM_VARIENTTYPES_H
#define HFT_MM_VARIENTTYPES_H

#include "Type.h"
#include "MarketData.h"
#include "OrderField.h"
//#include "../MMEngine/MMEngine.h"
//#include "../TrendEngine/TrendEngine.h"



namespace Cosmos{
    namespace Types {

        enum class EngineState{pending, noTrading, hedging, FAK};

        struct EngineComState{
            Instrument_t pendingInstrumentID{""};
            EngineState eventState;

        };

    //    using EventData = std::variant<const MarketData*, const OrderField*, const EngineComState*>;
       struct EventData{
           const void* point;
           int type;
       };
   //    using EngineTypes = std::variant<TradeBots::MMEngine::MMEngine, TradeBots::TrendEngine::TrendEngine>;

    }

}

#endif //HFT_MM_VARIENTTYPES_H
