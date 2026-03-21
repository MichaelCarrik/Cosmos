//
// Created by zhangyw on 8/9/21.
//

#ifndef PAIRSTRADING_MMEVENT_H
#define PAIRSTRADING_MMEVENT_H

#include "OrderField.h"
#include "../Types/Symbol.h"
#include "../Utils/EWMAPriceIndicator.h"


namespace TrendFollow{
    namespace Types {
        struct MMPolicyEvent {
            int subPolicyID{-1};
            Types::Symbol *tradeSymbol{nullptr};
            const Types::OrderField * buyOrder{nullptr};
            const Types::OrderField * sellOrder{nullptr};
            const Types::MarketData *tradeMD{nullptr};
            std::shared_ptr<spdlog::logger> m_orderLog;
            double lastFilledPrice{0.0};
            Types::OrderSide lastFilledOrderSide;

            int preCloseToday;
            Types::EventType eventType;
            int64_t epoch_time;
            Utils::EWMAPriceIndicator ewmaPriceIndicator;

        };

    }
};

#endif //PAIRSTRADING_MMEVENT_H
