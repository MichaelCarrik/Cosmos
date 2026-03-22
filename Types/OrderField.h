//
// Created by zhangyw on 5/9/19.
//

#ifndef TRADEBOTS_ORDERFIELD_H
#define TRADEBOTS_ORDERFIELD_H

#define TRADE_Femas
//#define TRADE_EES

#include <string>
#include <array>
#include "Type.h"


namespace Cosmos{
    namespace Types {

        struct QryRspTrade{
            Instrument_t instrumentID;
            int lastFilledVolume;
            int tradeOrderID{-1};
            int tradePolicyID;
            double lastFilledPrice;
            std::string tradeNo;
            OrderSide orderSide;
            PositionEffectType pet;
        };

        struct OrderField {

            int64_t  epoch_time;
            double orderPrice;
            double lastFilledPrice;
            int policyID;
            int tOrderID;
            int pOrderID;
            int orderVolume;
            int lastFilledVolume;
            int filledVolume;
            bool isTerminal{true};

            Instrument_t instrumentID{""}; //7

            PositionEffectType pet;
            OrderSide orderSide;

            OrderTimeType orderTimeType{OrderTimeType::COMMON};
            OrderStatus orderStatus;
            SignalIntension intension;
            HedgeType hedgeType;
            std::array<char, 31> orderSysID{""};
            std::array<char, 31> orderRef{""};
            std::string netRootContractNum{""};
            ExchangeType exchangeId;


            }__attribute__((__aligned__(64)));


    }
}



#endif //TRADEBOTS_ORDERFIELD_H
