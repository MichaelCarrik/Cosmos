//
// Created by zhangyingwei on 2026/3/27.
//

#ifndef COSMOS_TEST_H
#define COSMOS_TEST_H


#include "IFuturePolicy.h"
#include "../Utils/Indicator.h"

namespace Cosmos {
    namespace Policy {
        class TestTrend : public IFuturePolicy {
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
            TestTrend(std::string const &policyName, std::string const &engineName,
                         Types::Instrument_t &instrument, Types::KPeriod kperiod, double mv,
                         double multi, int tradingDay, int adjRiskTime,
                         double alpha, double mark) : IFuturePolicy(policyName, engineName, instrument, kperiod,
                                                                    mv, multi, tradingDay, adjRiskTime),
                                                      m_alpha(alpha), m_mark(mark) {
                _initPolicyLogger();
            }

            ~TestTrend() {
            }

            virtual void updateParam(const Types::NetModifyParam *netModifyParam) override {
                if(strcmp(netModifyParam->paramName.c_str(), "adjRiskTime") == 0) {
                    m_adjRiskTime = Utils::ToPsSeconds(netModifyParam->paramValue,false);
                    fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, adjRiskTime=%d\n",
                    m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                    m_adjRiskTime);
                }else if(strcmp(netModifyParam->paramName.c_str(), "MV") == 0) {
                    m_MV = stof(netModifyParam->paramValue);

                    fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, MV=%.3f\n",
                 m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                 m_MV);
                }
                else if (strcmp(netModifyParam->paramName.c_str(), "targetPos") == 0) {
                    auto itr = m_targetSignal.targetPosMaps.find(netModifyParam->symbolName);
                    if (itr != m_targetSignal.targetPosMaps.end()) {
                        fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, targetPosition=%s\n",
                                m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                                netModifyParam->paramValue.c_str());
                        m_targetSignal.targetPosMaps[netModifyParam->symbolName] = stoi(netModifyParam->paramValue);

                        auto lastBar = m_underlyKseries->m_KDataVecs[m_lastUnderlyBarIndex];
                        writePolicyLog(lastBar, m_underlyKseries->m_lastPMD);
                    }
                }
            };

            virtual void start(
                std::unordered_map<Types::Instrument_t, Types::Symbol *, Types::InstrumentHash> &
                inputSymbolMap) override {
                Types::Product_t product{""};
                Utils::InstrumentToProduct(m_underlyInstrument, product);
                m_dayMinutesCount = Utils::TradingHours::getDayMinutesCount(product);
                m_length = int(m_dayMinutesCount / 5) * 15;

                m_underlyKseries = getSeries(inputSymbolMap, m_kperiod, m_underlyInstrument);
                m_dayKSeries = getSeries(inputSymbolMap, Types::KPeriod::D1, m_underlyInstrument);
                initTrendSignal(m_trendSignal, m_engineName, m_policyName, m_underlyInstrument);
                _initVulture();

                fprintf(
                    stderr,
                    "createPolicy policyName=%s, engineName=%s, instrument=%s  kperiod=%d, mv=%.3f, multi=%.3f, tradingDay=%d, "
                    "adjRiskTime=%d, alpha=%.3f, mark=%.3f, dayMinutesCount=%d length=%d, signalPrice=%.3f, marketPosition=%d, targetPosition=%d, "
                    "preTargetPosition=%d\n", m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(),
                    static_cast<int>(m_kperiod), m_MV,
                    m_multi, m_tradingDay, m_adjRiskTime, m_alpha, m_mark, m_dayMinutesCount, m_length,
                    m_trendSignal.signalPrice, m_trendSignal.marketPosition,
                    m_targetSignal.targetPosMaps[m_underlyInstrument],
                    m_targetSignal.lastTargetPosMaps[m_underlyInstrument]);

                spdlog::info(
                    "createPolicy policyName={}, engineName={}, instrument={}  kperiod={}, mv={:.3f}, multi={:.3f}, tradingDay={}, "
                    "adjRiskTime={}, alpha={:.3f}, mark={:.3f}, dayMinutesCount={}, length={}, signalPrice={:.3f}, marketPosition={}, "
                    "targetPosition={}, preTargetPosition=%d", m_policyName.c_str(), m_engineName.c_str(),
                    m_underlyInstrument.data(),
                    static_cast<int>(m_kperiod), m_MV, m_multi, m_tradingDay, m_adjRiskTime, m_alpha,
                    m_mark, m_dayMinutesCount, m_length, m_trendSignal.signalPrice, m_trendSignal.marketPosition,
                    m_targetSignal.targetPosMaps[m_underlyInstrument],
                    m_targetSignal.lastTargetPosMaps[m_underlyInstrument]);
            };

            void _initVulture() {
                for (auto i = 0; i < m_dayKSeries->m_seriesIndex; i++) {
                    auto dayBar = m_dayKSeries->m_KDataVecs[i];
                    if (dayBar->m_tradingday < m_tradingDay) {
                        m_dayHighestVec.emplace_back(dayBar->m_high);
                        m_dayLowestVec.emplace_back(dayBar->m_low);
                        m_dayCloseVec.emplace_back(dayBar->m_close);
                    }
                }

                double highest = 9999.0, lowest = 0.0;
                if (m_dayHighestVec.size() > 0) {
                    if (m_dayHighestVec.size() < m_mark) {
                        highest = *std::max_element(std::begin(m_dayHighestVec), std::end(m_dayHighestVec));
                        lowest = *std::min_element(std::begin(m_dayLowestVec), std::end(m_dayLowestVec));
                    } else {
                        highest = *std::max_element(std::end(m_dayHighestVec) - m_mark, std::end(m_dayHighestVec));
                        lowest = *std::min_element(std::end(m_dayLowestVec) - m_mark, std::end(m_dayLowestVec));
                    }
                    m_band = (highest - lowest) * m_alpha / m_mark;
                    double dayClose = m_dayCloseVec[m_dayCloseVec.size() - 1];
                    //	double dayClose = m_dayKSeries->m_kseries[m_dayKSeries->m_seriesIndex-1]->m_close;
                    m_upBand = dayClose + m_band;
                    m_downBand = dayClose - m_band;
                } else {
                    m_band = (highest + lowest) * 0.5;
                    m_upBand = highest;
                    m_downBand = lowest;
                }

                for (auto i = 0; i < m_underlyKseries->m_seriesIndex; i++) {
                    m_fiveMinCloseVec.emplace_back(m_underlyKseries->m_KDataVecs[i]->m_close);
                }

                int beginI = 0, endI = 0;
                if (m_underlyKseries->m_seriesIndex < m_length) {
                    beginI = 0;
                    endI = m_underlyKseries->m_seriesIndex - 1;
                } else {
                    beginI = m_underlyKseries->m_seriesIndex - m_length;
                    endI = m_underlyKseries->m_seriesIndex - 1;
                }

                m_minsMA = Indicator::MA(m_fiveMinCloseVec, beginI, endI);
            }

            void _updateVultureSignal(const KData::KData *lastUnderlyBar) {
                int beginI = 0, endI = 0;
                if (m_lastUnderlyBarIndex < m_length) {
                    beginI = 0;
                    endI = m_lastUnderlyBarIndex;
                } else {
                    beginI = m_lastUnderlyBarIndex - m_length + 1;
                    endI = m_lastUnderlyBarIndex;
                }
                m_fiveMinCloseVec.emplace_back(lastUnderlyBar->m_close);
                m_minsMA = Indicator::MA(m_fiveMinCloseVec, beginI, endI);
            }

            virtual void runTick(const Types::MarketData *pMD) override {
                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {
                    if (m_lastUnderlyBarIndex == 0 and m_underlyKseries->m_seriesIndex > m_lastUnderlyBarIndex) {
                        if (m_underlyKseries->m_seriesIndex > 0) {
                            auto lastBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex - 1];

                            m_lots = calAdjRiksLots(lastBar->m_close);
                            writePolicyLog(lastBar, pMD);
                        }
                    } else if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex || strcmp(pMD->updateTime.data(), "09:30:58") ==0) {
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex - 1;

                        auto lastBar = m_underlyKseries->m_KDataVecs[m_lastUnderlyBarIndex];
                        _updateVultureSignal(lastBar);

                        m_lots = calAdjRiksLots(lastBar->m_close);

                        if (lastBar->m_close >= m_minsMA) {
                            m_onoff = 1;
                        } else if (lastBar->m_close <= m_minsMA) {
                            m_onoff = -1;
                        }



                        // if (m_trendSignal.marketPosition <0) //long open
                        // {
                        //     ++m_tradeNum;
                        //     m_trendSignal.marketPosition = 1;
                        //     m_targetSignal.targetPosMaps[m_underlyInstrument] = m_lots;
                        //     m_trendSignal.signalPrice = lastBar->m_close;
                        //     m_onoff = 0;
                        // } else if ( m_trendSignal.marketPosition >= 0) //short open
                        // {
                        //     ++m_tradeNum;
                        //     m_trendSignal.marketPosition = -1;
                        //     m_targetSignal.targetPosMaps[m_underlyInstrument] = -m_lots;
                        //     m_trendSignal.signalPrice = lastBar->m_close;
                        //     m_onoff = 0;
                        // }
                        if (strcmp(pMD->updateTime.data(), "10:00:00") ==0 && pMD->milliSeconds == 100) {
                            ++m_tradeNum;
                            m_trendSignal.marketPosition = 1;
                            Types::Instrument_t ins{"MO2403-C-5200"};
                            m_targetSignal.targetPosMaps[ins] = m_lots;
                            m_trendSignal.signalPrice = lastBar->m_close;
                            m_onoff = 0;
                        }



                        if (m_targetSignal.targetPosMaps[m_underlyInstrument] == m_targetSignal.lastTargetPosMaps[
                                m_underlyInstrument] &&
                            m_targetSignal.targetPosMaps[m_underlyInstrument] == 0) {
                            m_trendSignal.signalPrice = 0.0;
                        }

                        if (m_adjRiskTime == Utils::ToPsSeconds(lastBar->m_updateTimeBegin, false)) {
                            if (m_targetSignal.targetPosMaps[m_underlyInstrument] > 0 && m_targetSignal.targetPosMaps[
                                    m_underlyInstrument] != m_lots) {
                                // for risk adjust
                                m_targetSignal.targetPosMaps[m_underlyInstrument] = m_lots;
                            } else if (m_targetSignal.targetPosMaps[m_underlyInstrument] < 0 && m_targetSignal.
                                       targetPosMaps[m_underlyInstrument] != -m_lots) {
                                m_targetSignal.targetPosMaps[m_underlyInstrument] = -m_lots;
                            }
                        }
                        writePolicyLog(lastBar, pMD);
                        m_targetSignal.lastTargetPosMaps.clear();
                        std::copy(m_targetSignal.targetPosMaps.begin(), m_targetSignal.targetPosMaps.end(),
                                  std::inserter(m_targetSignal.lastTargetPosMaps,
                                                m_targetSignal.lastTargetPosMaps.begin()));
                    }
                    m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                }
            };

            virtual void writePolicyLog(const KData::KData *lastUnderlyKB, const Types::MarketData *pMD) override {
                m_configLog->info(
                    "{}, {}, {}, {}, {:03d}, close={:.3f}({:.3f}, {:.3f}), open={:.3f}, band={:.3f}, "
                    "upBand={:.3f}, downBand={:.3f}, minsMA={:.3f}, "
                    "onoff={}, mktPos={}, sgnPrice={:.3f}, "
                    "tgtPos={}, lots={}",
                    lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingday,
                    lastUnderlyKB->m_updateTimeBegin.data(), pMD->updateTime.data(), pMD->milliSeconds,
                    lastUnderlyKB->m_close, pMD->bidPrice[0], pMD->askPrice[0], lastUnderlyKB->m_open, m_band, m_upBand,
                    m_downBand, m_minsMA, m_onoff, m_trendSignal.marketPosition, m_trendSignal.signalPrice,
                    m_targetSignal.targetPosMaps[m_underlyInstrument], m_lots
                );
                m_configLog->flush();
            };
        };
    }
}
#endif //COSMOS_VULTURE_H
