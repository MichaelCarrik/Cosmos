//
// Created by zhangyw on 2/14/21.
//

#ifndef Cosmos_VULTURE_H
#define Cosmos_VULTURE_H

#include "LPolicy.h"
#include <fstream>


namespace Cosmos {
    namespace Policy {
        class Vulture : public LPolicy {
        private:
             Types::KPeriod m_kperiod;
            int m_length;
            double m_alpha;
            double m_mark;


            int m_lastBarIndex{0};
            int m_marketPosition;
            int m_offset;

            const KData::KSeries *m_mainKSeries{nullptr};
            const KData::KSeries *m_dayKSeries{nullptr};

            int m_lots{0};
            int m_onoff{0};
            int m_tradeNum{0};
            std::vector<double> m_dayHighestVec;
            std::vector<double> m_dayLowestVec;
            std::vector<double> m_dayCloseVec;
            std::vector<double> m_fiveMinCloseVec;
            double m_band;
            double m_upBand;
            double m_downBand;
            double m_minsMA;
            int m_dayMinutesCount{0};



        public:
            Vulture( Types::KPeriod kperiod, int length, double alpha, double mark,
                    std::string &policyName, std::string &engineName, int tradingday, double multi,
                    int adjRiskTime, double riskValue, std::string &riskParam, Types::SignalIntension si) :
                    m_kperiod(kperiod), m_length(length), m_alpha(alpha), m_mark(mark){
                m_policyName = policyName;
                m_engineName = engineName;
                m_tradingday = tradingday;
                m_multi = multi;
                m_adjRiskTime = adjRiskTime;
                m_riskValue = riskValue;
                m_riskParam = riskParam;
                m_signalIntension = si;
                spdlog::info(
                        "createPolicy m_engineName={}, m_policyName={}, kperiod={}  m_length={}, m_alpha={}, m_mark={}, "
                        " tradingday={}, multi={}, adjRiskTime={}, riskValue={}, riskParam={}, SI={}", m_engineName, m_policyName,
                        (int) kperiod, m_length, m_alpha, m_mark, m_tradingday, m_multi, m_adjRiskTime, m_riskValue,  m_riskParam,
                        Types::signalIntensionMap[m_signalIntension].data());
                _initPolicyLogger();
            }

            ~Vulture() {
            }

            virtual void start() override {
                auto itr = m_symbol->m_kSeriesMap.find(m_kperiod);
                m_mainKSeries = itr->second;

                itr = m_symbol->m_kSeriesMap.find(Types::KPeriod::D1);
                m_dayKSeries = itr->second;

                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s.txt", m_engineName.c_str(), m_policyName.c_str());
                m_signalPrice = atof(this->GetLastValueFromFile(configPath, "signalPrice").c_str());
                m_marketPosition = atoi(this->GetLastValueFromFile(configPath, "marketPosition").c_str());
                m_targetPosition = atoi(this->GetLastValueFromFile(configPath, "targetPosition").c_str());
                m_preTargetPosition = m_targetPosition;

                fprintf(stderr, "symbol %s\n", m_engineName.c_str());
                Types::Product_t product{""};
                strcpy(product.data(), m_engineName.c_str());
                m_dayMinutesCount = Utils::TradingHours::getDayMinutesCount(product);
                m_length = int(m_dayMinutesCount / 5 ) * 15;
                fprintf(stderr, "[%s_%s] start, m_kperiod=%d, m_length=%d, m_offset=%d, m_alpha=%.3f, m_mark=%.3f, "
                                "tradingday=%d, multi=%.3f, adjRiskTime=%d, riskValue=%.3f, riskParam=%s, m_signalPrice=%.3f, "
                                "m_marketPosition=%d, m_targetPosition=%d, m_preTargetPosition=%d, m_dayMinutesCount=%d, "
                                "m_length=%d\n",
                        m_engineName.c_str(), m_policyName.c_str(), (int)m_kperiod, m_length, m_offset, m_alpha, m_mark, m_tradingday, m_multi,
                        m_adjRiskTime, m_riskValue,  m_riskParam.c_str(), m_signalPrice, m_marketPosition, m_targetPosition, m_preTargetPosition,
                        m_dayMinutesCount, m_length);
            };


            virtual void runTick(const  Types::MarketData *pMD) override {


                if (m_lastBarIndex == 0 and m_mainKSeries->m_seriesIndex > m_lastBarIndex) {

                    for (auto i = 0; i < m_dayKSeries->m_seriesIndex; i++) {
                        auto dayBar = m_dayKSeries->m_kseries[i];
                        if (dayBar->m_tradingday < m_tradingday) {
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

                    for (auto i = 0; i < m_mainKSeries->m_seriesIndex; i++) {
                        m_fiveMinCloseVec.emplace_back(m_mainKSeries->m_kseries[i]->m_close);
                    }

                    int beginI = 0, endI = 0;
                    if (m_mainKSeries->m_seriesIndex < m_length) {
                        beginI = 0;
                        endI = m_mainKSeries->m_seriesIndex-1;

                    } else {
                        beginI = m_mainKSeries->m_seriesIndex - m_length ;
                        endI = m_mainKSeries->m_seriesIndex -1;
                    }

                    m_minsMA = Indicator::MA(m_fiveMinCloseVec, beginI, endI);

                    if (m_mainKSeries->m_seriesIndex > 0) {
                        auto lastBar = m_mainKSeries->m_kseries[m_mainKSeries->m_seriesIndex - 1];
                        if (strcmp("14:55:00", lastBar->m_updateTime.data()) ==0 && lastBar->m_tradingday == 20250813) {
                            int a =1;
                        }
                        auto barIndex = m_mainKSeries->m_seriesIndex - 1;
                        m_lots = calAdjRiksLots(lastBar->m_close);
                        m_configLog->info(
                                "{}, {}, {}, {}, {}, close={:.3f}({:.3f}, {:.3f}), open={:.3f}, m_band={:.3f}, "
                                "m_upBand={:.3f}, m_downBand={:.3f}, m_ma={:.3f}, "
                                "onoff={}, marketPosition={}, signalPrice={:.3f}, "
                                "targetPosition={}, lots={}",
                                m_symbol->instrumentID.data(), lastBar->m_tradingday,
                                lastBar->m_updateTime.data(), pMD->updateTime.data(), pMD->milliSeconds,
                                lastBar->m_close, pMD->bidPrice[0], pMD->askPrice[0], lastBar->m_open, m_band, m_upBand, m_downBand, m_minsMA,
                                m_onoff, m_marketPosition, m_signalPrice, m_targetPosition, m_lots
                               );
                    }

                } else if (m_lastBarIndex < m_mainKSeries->m_seriesIndex) {

                    auto lastBar = m_mainKSeries->m_kseries[m_lastBarIndex];

                    if (strcmp("14:15:00", lastBar->m_updateTime.data()) ==0 && lastBar->m_tradingday == 20250813 ) {
                        int a = 1;
                    }


                    m_lots = calAdjRiksLots(lastBar->m_close);

                    int beginI = 0, endI = 0;
                    if (m_lastBarIndex < m_length) {
                        beginI = 0;
                        endI = m_lastBarIndex;

                    } else {
                        beginI = m_lastBarIndex - m_length + 1;
                        endI = m_lastBarIndex ;
                    }
                    m_fiveMinCloseVec.emplace_back(lastBar->m_close);
                    m_minsMA = Indicator::MA(m_fiveMinCloseVec, beginI, endI);


                    if (lastBar->m_close >= m_minsMA) {
                        m_onoff = 1;
                    } else if (lastBar->m_close <= m_minsMA) {
                        m_onoff = -1;
                    }


                    if (m_marketPosition > 0 && lastBar->m_close <= m_downBand) {  //long close
                        ++m_tradeNum;
                        m_marketPosition = 0;
                        m_targetPosition = 0;
                        m_signalPrice = lastBar->m_close;
                        m_onoff = 0;
                    } else if (m_marketPosition < 0 && lastBar->m_close >= m_upBand) {  //short close
                        ++m_tradeNum;
                        m_marketPosition = 0;
                        m_targetPosition = 0;
                        m_signalPrice = lastBar->m_close;
                        m_onoff = 0;
                    }

                    if (m_tradeNum < 10 && m_marketPosition == 0) {
                        if (lastBar->m_close >= m_upBand && lastBar->m_close > lastBar->m_open &&
                            m_onoff == 1) //long open
                        {
                            ++m_tradeNum;
                            m_marketPosition = 1;
                            m_targetPosition = m_lots;
                            m_signalPrice = lastBar->m_close;
                            m_onoff = 0;
                        } else if (lastBar->m_close <= m_downBand && lastBar->m_close < lastBar->m_open &&
                                   m_onoff == -1)//short open
                        {
                            ++m_tradeNum;
                            m_marketPosition = -1;
                            m_targetPosition = -m_lots;
                            m_signalPrice = lastBar->m_close;
                            m_onoff = 0;
                        }
                    }


                    if (this->m_targetPosition == this->m_preTargetPosition && this->m_targetPosition == 0) {
                        m_signalPrice = 0.0;
                    }

                    if( std::strcmp(m_riskParam.c_str(), "Lots") == 0 && m_adjRiskTime == Utils::ToPsSeconds(lastBar->m_updateTime, false) ){
                        if (this->m_targetPosition > 0 && this->m_targetPosition != m_lots ) {   // for risk adjust
                            this->m_targetPosition = m_lots;
                        } else if (this->m_targetPosition < 0 && this->m_targetPosition != -m_lots ) {
                            this->m_targetPosition = -m_lots;
                        }
                    }else if( std::strcmp(m_riskParam.c_str(), "MV") == 0 && m_adjRiskTime == Utils::ToPsSeconds(lastBar->m_updateTime, false)){
                         if (this->m_targetPosition > 0 && this->m_targetPosition != m_lots ) {   // for risk adjust
                            this->m_targetPosition = m_lots;
                        } else if (this->m_targetPosition < 0 && this->m_targetPosition != -m_lots ) {
                            this->m_targetPosition = -m_lots;
                        }
                    }





                    m_configLog->info(
                            "{}, {}, {}, {}, {}, close={:.3f}({:.3f}, {:.3f}), open={:.3f}, m_band={:.3f}, "
                            "m_upBand={:.3f}, m_downBand={:.3f}, m_ma={:.3f}, "
                            "onoff={}, marketPosition={}, signalPrice={:.3f}, "
                            "targetPosition={}, lots={}",
                            m_symbol->instrumentID.data(), lastBar->m_tradingday, lastBar->m_updateTime.data(),
                            pMD->updateTime.data(),   pMD->milliSeconds, lastBar->m_close,  pMD->bidPrice[0], pMD->askPrice[0], lastBar->m_open,
                            m_band, m_upBand, m_downBand, m_minsMA, m_onoff, m_marketPosition, m_signalPrice, m_targetPosition, m_lots
                          );
                    m_configLog->flush();
                }
                m_lastBarIndex = m_mainKSeries->m_seriesIndex;
            };




        };
    }
}
#endif //Cosmos_VULTURE_H
