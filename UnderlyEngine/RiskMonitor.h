//
// Created by zhangyingwei on 2026/4/2.
//

#ifndef COSMOS_RISKMONITOR_H
#define COSMOS_RISKMONITOR_H

#include "../Types/Type.h"
#include "../Types/Param.h"
#include "../Types/Symbol.h"
#include "../Utils/TradingHours.h"

namespace Cosmos {
    namespace Engine {
        class RiskMonitor {
        public:
            RiskMonitor( std::string const& engineName, int policyID, int tradingDay,
                Types::EngineParam const& engineParam) : m_engineName(engineName),
            m_policyID{policyID},m_tradingDay(tradingDay), m_engineParam(engineParam) {

            };
            Types::EngineParam m_engineParam;
            int m_policyID{-1};
            std::string m_engineName;
            int m_tradingDay{0};
            bool isRiskForNoTrade(const Types::Symbol * underlySymbol, bool isOption) ;
            bool isRiskForOrder(const Types::Symbol * underlySymbol, const Types::OrderField * orderField);

            void modifyOrderByRisk(const Types::MarketData *pMD, Types::OrderField * orderField, bool isOption);
        };
    }
}


#endif //COSMOS_RISKMONITOR_H