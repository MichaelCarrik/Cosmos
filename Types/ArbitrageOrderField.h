//
// Created by zhangyw on 8/13/21.
//

#ifndef PAIRSTRADING_ARBITRAGEORDERFIELD_H
#define PAIRSTRADING_ARBITRAGEORDERFIELD_H
#include <string>
#include <array>
#include "Type.h"


namespace TrendFollow{
    namespace Types {

        struct ArbitrageOrderField {

            int64_t  epoch_time;
            double  orderPrice;
            std::array<double,2> lastFilledPrice;
            int policyID;
            int tOrderID;
            int pOrderID;
            int subPolicyID;
            int orderVolume;
            std::array<int,2> lastFilledVolume;
            std::array<int,2> filledVolume;
            bool isTerminal{true};

            std::array<Instrument_t ,2> instrumentIDs; //7

            std::array<PositionEffectType,2> pets;
            std::array<OrderSide,2> orderSides;

            OrderTimeType orderTimeType{OrderTimeType::COMMON};
            OrderStatus orderStatus;
            SignalIntension intension;
            std::array<char,4> orderPrex{""};
            std::array<char, 31> orderSysID{""};
            std::array<char, 31> orderRef{""};
            std::string netRootContractNum{""};
            ExchangeType exchangeId;


        }__attribute__((__aligned__(64)));


    }
}

#endif //PAIRSTRADING_ARBITRAGEORDERFIELD_H
