//
// Created by zhangyingwei on 2026/3/26.
//

#ifndef COSMOS_IFUTUREPOLICY_H
#define COSMOS_IFUTUREPOLICY_H

#include "../Policy/IPolicy.h"
#include "../Types/Type.h"
#include "../Utils/Utils.h"


namespace Cosmos {
    namespace Policy {
        struct TrendSignal {
            double signalPrice{0.0};
            int marketPosition{0};
            int preMarketPosition{0};

        };

        class IFuturePolicy : public IPolicy {
        public:
            TrendSignal m_trendSignal;
            TargetSignal m_targetSignal;
            int m_adjRiskTime{-1};
            int m_lots{0};

            IFuturePolicy(std::string const &policyName, std::string const &engineName, Types::Instrument_t &instrument,
                Types::KPeriod kperiod, double mv, double multi, int tradingDay, int adjRiskTime) :
                            IPolicy(policyName, engineName,instrument, kperiod, mv, multi, tradingDay),
                            m_adjRiskTime(adjRiskTime) {
            }

            virtual void writePolicyLog(const KData::KData *lastUnderlyKB , const Types::MarketData * pMD) =0;

            const KData::KSeries* getSeries(std::unordered_map<Types::Instrument_t, Types::Symbol *, Types::InstrumentHash> &inputSymbolMap,
                Types::KPeriod kPeriod, Types::Instrument_t &instrument) {
                auto itr = inputSymbolMap.find(instrument);
                if (itr == inputSymbolMap.end()) {
                    assert(false);
                }

                auto itrkP= itr->second->m_kSeriesMap.find(kPeriod);
                if (itrkP == itr->second->m_kSeriesMap.end()) {
                    assert(false);
                }
                return itrkP->second;
            }

            void initTrendSignal(TrendSignal & trendSignal, std::string const& engineName, std::string const& policyName,
                Types::Instrument_t const& underlyInstrument) {
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(), underlyInstrument.data());
                m_trendSignal.signalPrice = atof(this->GetLastValueFromFile(configPath, "sgnPrice").c_str());
                m_trendSignal.marketPosition = atoi(this->GetLastValueFromFile(configPath, "mktPos").c_str());
                m_targetSignal.targetPosMaps[m_underlyInstrument] = atoi(this->GetLastValueFromFile(configPath, "tgtPos").c_str());
                m_targetSignal.lastTargetPosMaps[m_underlyInstrument] =  atoi(this->GetLastValueFromFile(configPath, "tgtPos").c_str());

            }

            int calAdjRiksLots(double close){
                int lots = std::max(std::round(m_MV *10000 / (close * m_multi)), 1.0);
                return lots;
            }



        };
    }
}
#endif //COSMOS_IFUTUREPOLICY_H
