//
// Created by zhangyingwei on 2026/3/27.
//

#ifndef COSMOS_TESTOPTION_H
#define COSMOS_TESTOPTION_H


#include "IOptionPolicy.h"
#include "../Utils/Indicator.h"

namespace Cosmos {
    namespace Policy {
        class TestTrendOption : public IOptionPolicy {
        private:
            int m_length{1000};
            double m_alpha{0.7};
            double m_mark{2.0};

            int m_offset;
            const KData::KSeries *m_dayKSeries{nullptr};

            int m_onoff{0};
            int m_tradeNum{0};
            std::vector<double> m_dayHighestVec;
            std::vector<double> m_dayLowestVec;
            std::vector<double> m_dayCloseVec;
            std::vector<double> m_fiveMinCloseVec;
            double m_band{9999.0};
            double m_upBand{9999999.0};
            double m_downBand{0.0};
            double m_minsMA{0.0};
            int m_dayMinutesCount{0};

        public:
            TestTrendOption(std::string const &policyName, std::string const &engineName,
                          Types::Instrument_t &instrument, Types::KPeriod kperiod, double mv, double multi,
                          int tradingDay, int expireDay) :  IOptionPolicy(policyName, engineName, instrument,
                                                kperiod, mv, multi,  tradingDay, expireDay) {

            }

            ~TestTrendOption() {}

            virtual void updateParam(const Types::NetModifyParam *netModifyParam) override {};

            virtual void start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) override{};

            virtual void runTick(const Types::MarketData *pMD) override {
                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {
                     if ( strcmp(pMD->updateTime.data(), "10:00:00") ==0 ) {  //waiting all instrtuments finish KData
                        Types::Instrument_t ins{"MO2403P5600"};
                        m_callPolicySymbols.targetSignal.targetPosMaps[ins] = -3;
                    }
                }
            }
        };
    }
}
#endif //COSMOS_TESTOPTION_H
