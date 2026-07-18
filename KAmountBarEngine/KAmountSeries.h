//
// Created by zhangyw on 1/11/21.
//

#ifndef Cosmos_KSERIES_H
#define Cosmos_KSERIES_H


#include "KAmountData.h"
#include "../Types/KPeriod.h"
#include "../Utils/TradingHours.h"

namespace Cosmos {
    namespace KAmountBarEngine {
        class KAmountSeries {
        public:
            std::vector<KAmountData *> m_KAmountDataVecs;
            int m_seriesIndex{0};
            int m_recordIndex{0};
            int m_tradingday{0};
            Types::InstrumentInfo m_insInfo;

            int m_currentBarVolume{0};
            double m_currentBarTurnover{0.0};
            double m_amountThreshold{99999999999999.0};


            Types::KPeriod m_Period;
            Types::MarketData *m_lastPMD{nullptr};

            KAmountSeries(Types::InstrumentInfo const &insInfo,int tradingday, double r,Types::KPeriod period,
                          Utils::TradingSession &tradingSession, bool isDay, double perBarAmountThresh) :
            m_insInfo(insInfo), m_tradingday(tradingday), m_Period(period), m_amountThreshold(perBarAmountThresh)
           {
               m_KAmountDataVecs.emplace_back(new KAmountData());
            }


            void addTick(const Types::MarketData *pMD, bool isDay) {

                // if (strcmp(pMD->instrumentID.data(), "IM2606") != 0 || m_Period != Types::KPeriod::Min1) {
                //         return;
                // }
                int diffVolume = pMD->volume;
                double diffTurnover = pMD->amount;

                if(m_lastPMD == nullptr){
                    m_lastPMD = new Types::MarketData();
                    memcpy(m_lastPMD, pMD, sizeof(Types::MarketData));
                    return;
                }
                diffTurnover = pMD->amount - m_lastPMD->amount;
                diffVolume = pMD->volume - m_lastPMD->volume;
                double tradePrice = diffVolume > 0 ? diffTurnover/ (diffVolume * m_insInfo.multi) : (m_lastPMD->bidPrice[0] + m_lastPMD->askPrice[0]) * 0.5;
                double imbalanceAmount = 0.0;
                if (diffVolume > 0 && tradePrice > m_lastPMD->midPrice + 0.1 * m_insInfo.tickSize) {
                    imbalanceAmount = diffTurnover;
                }else if (diffVolume > 0 && tradePrice < m_lastPMD->midPrice - m_insInfo.tickSize *0.1) {
                    imbalanceAmount = -diffTurnover;
                }



                    if (abs(m_KAmountDataVecs[m_seriesIndex]->m_open - 0.0) < 0.001) {
                        //                        fprintf(stderr, "init k pmd 3: instrumentid=%s, updateTime=%s, millisec=%d, volume=%d, m_seriesIndex=%d, m_kseries.size()=%d\n",
                        //                                pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, m_seriesIndex, m_kseries.size());
                        if (m_Period ==  Types::KPeriod::D1) {
                            m_KAmountDataVecs[m_seriesIndex]->initKDayKBar(m_insInfo.instrumentID, std::max(pMD->psSecond, 0),
                            m_tradingday, pMD, diffVolume, diffTurnover);
                        } else {
                            m_KAmountDataVecs[m_seriesIndex]->initKBar(m_insInfo.instrumentID, std::max(pMD->psSecond, 0),
                            m_tradingday, pMD,  diffVolume, diffTurnover);
                        }
                    }

                    if (m_Period ==  Types::KPeriod::D1) {
                        m_KAmountDataVecs[m_seriesIndex]->updateDayKBar(pMD->psSecond, pMD->lastPrice, pMD->highestPrice, pMD->lowestPrice,
                                                                       pMD->openPrice, diffVolume,
                                                                      diffTurnover, pMD->oi, pMD->settlementPrice);
                    } else {
                        m_KAmountDataVecs[m_seriesIndex]->update(pMD, diffVolume, diffTurnover);
                    }

                    m_currentBarVolume += diffVolume;
                    m_currentBarTurnover += imbalanceAmount;
                    if (std::abs(m_currentBarTurnover) > m_amountThreshold) {
                        m_currentBarVolume = 0;
                        m_currentBarTurnover = 0.0;
                        m_KAmountDataVecs.emplace_back(new KAmountData());
                        m_seriesIndex += 1;
                    }
                    memcpy(m_lastPMD, pMD, sizeof(Types::MarketData));
            }

            void setHistoryKLine(std::vector<KAmountData *> &hisBarVec) {
                for (int i = hisBarVec.size() - 1; i >= 0; i--) {
                    m_KAmountDataVecs.emplace_back(hisBarVec[i]);
                    m_seriesIndex++;
                }
            }
        };
    }
}


#endif //Cosmos_KSERIES_H
