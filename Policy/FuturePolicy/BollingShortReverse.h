//
// Created by zhangyw on 2/12/21.
//

#ifndef TRENDFOLLOW_BOLLINGSHORTREVERSE_H
#define TRENDFOLLOW_BOLLINGSHORTREVERSE_H

#include "IFuturePolicy.h"
#include <fstream>


namespace Cosmos {
    namespace Policy {
        class BollingShortReverse : public IFuturePolicy {
        private:

            int m_length;
            int m_lastBarIndex{0};
            double m_stdCoif{3.0};
            double m_basePrice{0.0};
            double m_stdValue{0.0};
            double m_upBand{999999.0};
            double m_downBand{0.0};
            int m_onOff{0};

            std::vector<double> m_MaxVec;
            std::vector<double> m_MinVec;
            double m_maxValue{0.0};
            double m_minValue{0.0};


            int m_trendFlag{0};

            double m_exitPrice{0.0};

            int m_tradeNum{0};
            std::vector<double> m_closeVec;

        public:
            BollingShortReverse(TrendFollow::Types::KPeriod kperiod, int length, double stdCoif, double riskValue,
                std::string &riskParam, std::string &policyName, std::string &engineName, int tradingday, double tickSize,
                            double multi, Types::SignalIntension si) : m_stdCoif(stdCoif), m_kperiod(kperiod),
                                                                       m_length(length), m_tickSize(tickSize) {
                m_policyName = policyName;
                m_engineName = engineName;
                m_tradingday = tradingday;
                m_riskValue = riskValue;
                m_riskParam = riskParam;
                m_multi = multi;
                m_signalIntension = si;
                spdlog::info("createPolicy m_engineName={}, m_policyName={}, kperiod={}, length={}, tradingday={}, stdCoif={}, "
                             "riskValue={}, riskParam={}, tickSize={}, multi={}, SI={}",
                             engineName, policyName, (int) kperiod, m_length,  m_tradingday, m_stdCoif,  m_riskValue,  m_riskParam, m_tickSize,
                             m_multi, Types::signalIntensionMap[m_signalIntension].data());
                _initPolicyLogger();
            }

            ~BollingShortReverse() {
            }

            virtual void start() override {
                auto itr = m_symbol->m_kSeriesMap.find(m_kperiod);
                m_mainKSeries = itr->second;
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s.txt", m_engineName.c_str(), m_policyName.c_str());
                m_exitPrice = atof(this->GetLastValueFromFile(configPath, "exitPrice").c_str());
                m_signalPrice = atof(this->GetLastValueFromFile(configPath, "signalPrice").c_str());
                m_marketPosition = atoi(this->GetLastValueFromFile(configPath, "marketPosition").c_str());
                m_targetPosition = atoi(this->GetLastValueFromFile(configPath, "targetPosition").c_str());
                m_preTargetPosition = m_targetPosition;

                m_preMarketPosition = m_marketPosition;
                fprintf(stderr,
                        "[%s_%s] start, m_kperiod=%d, m_lots=%d, m_length=%d, m_stdCoif=%.3f, m_exitPrice=%f, m_signalPrice=%f, m_marketPosition=%d, "
                        "m_targetPosition=%d, m_preTargetPosition=%d\n",
                        m_engineName.c_str(), m_policyName.c_str(), m_kperiod, m_lots, m_length, m_stdCoif, m_exitPrice, m_signalPrice,
                        m_marketPosition, m_targetPosition, m_preTargetPosition);
            };


            void initIndicator(const TrendFollow::Types::MarketData *pMD) {
                for (auto i = 0; i < m_mainKSeries->m_seriesIndex; i++) {
                    auto bar = m_mainKSeries->m_kseries[i];
                    m_closeVec.emplace_back(bar->m_close);
                    m_MaxVec.emplace_back(bar->m_high);
                    m_MinVec.emplace_back(bar->m_low);
                }

                if (m_closeVec.size() < m_length) {
                    m_basePrice =m_closeVec[0];

                } else {
                    m_basePrice =m_closeVec[m_closeVec.size() - m_length];

                }

                if (m_closeVec.size() < m_length * 5) {
                    m_stdValue = Indicator::STDEV(m_closeVec, 0, m_closeVec.size() - 1);
                    m_maxValue = Indicator::MAX(m_MaxVec, 0, m_MaxVec.size() - 1);
                    m_minValue = Indicator::MIN(m_MinVec, 0, m_MinVec.size() - 1);
                }else {
                    m_stdValue = Indicator::STDEV(m_closeVec, m_closeVec.size() - m_length * 5, m_closeVec.size() - 1);
                    m_maxValue = Indicator::MAX(m_MaxVec, m_MaxVec.size() - m_length* 5 , m_MaxVec.size() - 1);
                    m_minValue = Indicator::MIN(m_MinVec, m_MinVec.size() - m_length* 5 , m_MinVec.size() - 1);
                }

                if (m_mainKSeries->m_seriesIndex > 0) {
                    this->writeConfigLog(pMD);
                }
            }

            void trendSignal(double midPrice, const TrendFollow::Types::MarketData *pMD) {

                double exitThresh = 0.001 * midPrice *5 ;


                if ( m_maxValue - m_minValue > 0.001 * midPrice *5  ){
                    m_onOff = 1;
                }else{
                    m_onOff =0;
                }


                if (m_downBand > midPrice && m_trendFlag > -1) {
                    m_trendFlag = -1;

                } else if (m_upBand < midPrice && m_trendFlag < 1) {
                    m_trendFlag = 1;

                }else {
                    m_trendFlag = 0;
                }

                if (pMD->psSecond > 14*3600 + 59 *60) {
                    if (m_marketPosition!=0) {
                          this->writeConfigLog(pMD);
                    }
                    this->writeConfigLog(pMD);
                    m_marketPosition = 0;
                    m_targetPosition = 0;
                    m_signalPrice = midPrice;
                    return;
                }


                if (m_trendFlag == -1  and m_marketPosition <= 0 and m_onOff == 1 ) {
                    this->writeConfigLog(pMD);
                    m_marketPosition = 1;
                    m_targetPosition = m_lots;
                    m_signalPrice = midPrice;
                    m_exitPrice = midPrice - exitThresh;
                    m_trendFlag = 0;

                } else if (m_trendFlag == 1 and  m_marketPosition >= 0 and m_onOff == 1 ) {
                    this->writeConfigLog(pMD);
                    m_marketPosition = -1;
                    m_targetPosition = -m_lots;
                    m_signalPrice = midPrice;
                    m_exitPrice = midPrice + exitThresh;
                    m_trendFlag = 0;

                }

                if (m_marketPosition > 0) {
                    if (midPrice <= m_exitPrice) {
                        this->writeConfigLog(pMD);
                        m_marketPosition = 0;
                        m_targetPosition = 0;
                        m_signalPrice = midPrice;
                        m_exitPrice = 0.0;
                        m_trendFlag = 0;
                    } else if (midPrice - exitThresh > m_exitPrice) {
                        m_exitPrice = midPrice - exitThresh;
                    }
                } else if (m_marketPosition < 0) {
                    if (midPrice >= m_exitPrice) {
                        this->writeConfigLog(pMD);
                        m_marketPosition = 0;
                        m_targetPosition = 0;
                        m_signalPrice = midPrice;
                        m_exitPrice = 0.0;
                        m_trendFlag = 0;
                    } else if (midPrice + exitThresh < m_exitPrice) {
                        m_exitPrice = midPrice + exitThresh;
                    }
                }
            }


            void writeConfigLog(const TrendFollow::Types::MarketData *pMD) {
                double midPrice = (pMD->bidPrice[0] + pMD->askPrice[0]) * 0.5;
                auto lastBar = m_mainKSeries->m_kseries[m_mainKSeries->m_seriesIndex - 1];
                m_configLog->info(
                    "{}, {}, {}, {}:{:03d}, close={:.3f}, midPrice={:.3f}, basePrice={:.3f}, {:.3f}({:.3f} {:.3f}), "
                    "onOff={}, ({:.3f} {:.3f}), trendFlag={}, exitPrice={:.3f}, signalPrice={:.3f}, marketPosition={}, targetPosition={}",
                    m_symbol->instrumentID.data(), lastBar->m_tradingday, lastBar->m_updateTime.data(),  pMD->updateTime.data(), pMD->milliSeconds, lastBar->m_close,
                    midPrice, m_basePrice, m_stdValue, m_upBand, m_downBand, m_onOff, m_maxValue, m_minValue,  m_trendFlag, m_exitPrice, m_signalPrice, m_marketPosition,
                    m_targetPosition);
                m_configLog->flush();
            }


            virtual void runTick(const TrendFollow::Types::MarketData *pMD) override {
                double midPrice = (pMD->bidPrice[0] + pMD->askPrice[0]) * 0.5;
                if (m_lastBarIndex == 0 and m_mainKSeries->m_seriesIndex > m_lastBarIndex) {
                    m_lots = calAdjRiksLots(midPrice);
                    this->initIndicator(pMD);
                } else if (m_lastBarIndex < m_mainKSeries->m_seriesIndex) {
                    auto lastBar = m_mainKSeries->m_kseries[m_lastBarIndex];
                    m_lots = calAdjRiksLots(lastBar->m_close);
                    m_closeVec.emplace_back(lastBar->m_close);
                    if (m_closeVec.size() < m_length) {
                        m_basePrice =m_closeVec[0];

                    } else {
                        m_basePrice =m_closeVec[m_closeVec.size() - m_length];

                    }

                    if (m_closeVec.size() < m_length * 5) {
                        m_stdValue = Indicator::STDEV(m_closeVec, 0, m_closeVec.size() - 1);
                        m_maxValue = Indicator::MAX(m_MaxVec, 0, m_MaxVec.size() - 1);
                        m_minValue = Indicator::MIN(m_MinVec, 0, m_MinVec.size() - 1);
                    }else {
                        m_stdValue = Indicator::STDEV(m_closeVec, m_closeVec.size() - m_length * 5, m_closeVec.size() - 1);
                        m_maxValue = Indicator::MAX(m_MaxVec, m_MaxVec.size() - m_length* 5 , m_MaxVec.size() - 1);
                        m_minValue = Indicator::MIN(m_MinVec, m_MinVec.size() - m_length* 5 , m_MinVec.size() - 1);
                    }

                    m_MaxVec.emplace_back(lastBar->m_high);
                    m_MinVec.emplace_back(lastBar->m_low);

                    m_upBand = m_basePrice + m_stdValue * m_stdCoif;
                    m_downBand = m_basePrice - m_stdValue * m_stdCoif;
                    this->writeConfigLog(pMD);
                    if (strcmp(lastBar->m_updateTime.data(), "13:36:00")==0) {
                        int a =1;
                    }
                }

                this->trendSignal(midPrice, pMD);

                if (this->m_targetPosition == this->m_preTargetPosition && this->m_targetPosition == 0) {
                    m_signalPrice = 0.0;
                }

                if (this->m_targetPosition > 0 && this->m_targetPosition != m_lots) {
                    // for risk adjust
                    this->m_targetPosition = m_lots;
                } else if (this->m_targetPosition < 0 && this->m_targetPosition != -m_lots) {
                    this->m_targetPosition = -m_lots;
                }


                if (m_preMarketPosition != m_marketPosition) {
                    this->writeConfigLog(pMD);
                }


                m_preMarketPosition = m_marketPosition;
                m_lastBarIndex = m_mainKSeries->m_seriesIndex;
            };


        };
    }
}
#endif //TRENDFOLLOW_BOLLINGSHORT_H
