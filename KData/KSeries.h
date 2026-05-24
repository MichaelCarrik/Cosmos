//
// Created by zhangyw on 1/11/21.
//

#ifndef Cosmos_KSERIES_H
#define Cosmos_KSERIES_H

#include <iostream>
#include "KSeriesTime.h"
#include "../OptionModel/BSModelQuantLib.h"
#include "../OptionModel/LetsBeRationalModel.h"
#include "KData.h"
//#include "CallPutSeries.h"
//#include "../OptionModel/SARBModelQuantLib.h"
#include "Common.h"

namespace Cosmos {
    namespace KData {
        class KSeries {
        public:
            std::vector<KData *> m_KDataVecs;
            int m_seriesIndex{0};
            int m_recordIndex{0};
            int m_tradingday{0};
            Types::InstrumentInfo m_insInfo;
            KSeriesTime * m_kseriesTime{nullptr};
            KTime * m_kTime{nullptr};
            Types::KPeriod m_Period;
            Types::MarketData * m_lastPMD{nullptr};
            double m_lastDelta{0.0};
            int m_biasSeconds{0};
            KSeries *m_underlySeries{nullptr};
            std::map<int, CallPutSeries *> * m_callPutSeriesMap{nullptr};

         //   OptionModel::BSModelQuantLib *m_BSModelQuantLib{nullptr};
            OptionModel::LetsBeRationalModel *m_BSModelQuantLib{nullptr};
            OptionModel::SARBModelQuantLib * m_sabrModelQuantLib{nullptr};

            KSeries( Types::InstrumentInfo const &insInfo,
                    int tradingday, double r,
                     Types::KPeriod period,
                     Utils::TradingSession &tradingSession, bool isDay, int biasSeconds) :
                    m_insInfo(insInfo), m_tradingday(tradingday), m_Period(period), m_biasSeconds(biasSeconds){

                if (m_insInfo.productIDClass == Types::ProductClass::option) {
                     // m_BSModelQuantLib = new OptionModel::BSModelQuantLib(m_insInfo.optionType, m_insInfo.strikePrice, tradingday,
                     //                                        m_insInfo.expireDate, r);
                   m_BSModelQuantLib = new OptionModel::LetsBeRationalModel(m_insInfo.optionType, m_insInfo.strikePrice, tradingday,
                                                           m_insInfo.expireDate, r);

                }else if (m_insInfo.productIDClass == Types::ProductClass::future) {
                      m_sabrModelQuantLib = new OptionModel::SARBModelQuantLib( tradingday,
                                                          m_insInfo.expireDate);
                }
				
				int secondsBias{0};
                if (period ==  Types::KPeriod::Min1 ) {
                    secondsBias = -10 + m_biasSeconds;
                }else if (period == Types::KPeriod::Min5) {
                    secondsBias = -20 + m_biasSeconds;
                }
                else if(period ==  Types::KPeriod::Min15){
                    secondsBias = -90 + m_biasSeconds;
                }else if(period ==  Types::KPeriod::Min30 ) {
                    secondsBias = -220 + m_biasSeconds;
                }
				
				m_kseriesTime = new KSeriesTime(tradingday, period, secondsBias);
                m_kseriesTime->initKTime(tradingSession, isDay);
            }

            void setUnderlySeries(KSeries *inputSeries) {
                m_underlySeries = inputSeries;
            }



            void calGreeks(int optionUpdateIndex, double optionFairPrice, double forwardPrice, double bidPrioPrice, double askPrioPrice) {

                 auto optionKData = m_KDataVecs[optionUpdateIndex];
                optionKData->m_forwardPrice = forwardPrice;
                // optionKData->m_forwardPrice = forwardPrice;
                // auto optionFairPrice = getKFairClose(optionKData);
                //
                // auto underlyKData = this->m_underlySeries->m_KDataVecs[updateUnderlyIndex];
                //

                try {
                    this->m_KDataVecs[optionUpdateIndex]->m_greeks.IV = m_BSModelQuantLib->calImpliedVol(    optionKData->m_forwardPrice,
                                                                                          optionFairPrice);

                    if (this->m_KDataVecs[optionUpdateIndex]->m_greeks.IV > 99999) {
                        this->m_KDataVecs[optionUpdateIndex]->m_greeks.IV = 0.0;
                    }


                    if (this->m_KDataVecs[optionUpdateIndex]->m_greeks.IV > Types::g_epsilon) {

                        m_BSModelQuantLib->calGreeks(optionKData->m_forwardPrice, this->m_KDataVecs[optionUpdateIndex]->m_greeks);
                    }
                    if (std::abs(this->m_KDataVecs[optionUpdateIndex]->m_greeks.delta) < 0.00001) {
                        this->m_KDataVecs[optionUpdateIndex]->m_greeks.delta = m_lastDelta;
                    }
                    m_lastDelta = this->m_KDataVecs[optionUpdateIndex]->m_greeks.delta;
                } catch (std::exception &e) {
                //   if ( strcmp(this->m_insInfo.productID.data() , "si")==0 ) {
                        // fprintf(stderr,"%s-%d-%s, %s\n", this->m_KDataVecs[optionUpdateIndex]->m_instrument.data(),Types::KPeroidToIntervalMap[this->m_Period],
                        //      this->m_KDataVecs[optionUpdateIndex]->m_updateTimeBegin.data() ,e.what());
                      //  std::cerr << e.what() << std::endl;
                        // fprintf(stderr, "calGreeks exception underly=%s %s (%.3f) , optionKD=%s %s (%.3f)\n",
                        //         underlyKData->m_instrument.data(),
                        //         underlyKData->m_updateTimeBegin.data(),
                        //         optionKData->m_forwardPrice,
                        //         optionKData->m_instrument.data(),
                        //         optionKData->m_updateTimeBegin.data(),
                        //         optionFairPrice);

              //      }
                    if (std::abs(this->m_KDataVecs[optionUpdateIndex]->m_greeks.delta) < 0.00001) {
                        this->m_KDataVecs[optionUpdateIndex]->m_greeks.delta = m_lastDelta;
                    }
                    m_lastDelta = this->m_KDataVecs[optionUpdateIndex]->m_greeks.delta;
                    //  return 1;
                } catch (...) {
                    //    std::cerr << "unknown error" << std::endl;
                    //   return 1;

                }

                // try {
                //     this->m_KDataVecs[optionUpdateIndex]->bidIV = m_BSModelQuantLib->calImpliedVol(    optionKData->m_forwardPrice,
                //                                                         bidPrioPrice);
                // }catch (...) {
                //     //    std::cerr << "unknown error" << std::endl;
                //     //   return 1;
                //
                // }
                // try {
                //     this->m_KDataVecs[optionUpdateIndex]->askIV = m_BSModelQuantLib->calImpliedVol(    optionKData->m_forwardPrice,
                //                                                        askPrioPrice);
                // }catch (...) {
                //     //    std::cerr << "unknown error" << std::endl;
                //     //   return 1;
                //
                // }
            }

            void ffill( int psTime ) {
                while (m_kseriesTime->isLast() == false and psTime >= m_kTime->endPstime) {  //qianzhi bu
                    if (m_lastPMD ==nullptr) {
                        int a = 1;
                    }
                    if (strcmp(m_KDataVecs[m_seriesIndex]->m_instrument.data(), "") == 0) {
                        //                        fprintf(stderr, "init k pmd 1 : instrumentid=%s, updateTime=%s, millisec=%d, volume=%d, m_seriesIndex=%d, m_kseries.size()=%d\n",
                        //                                pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, m_seriesIndex, m_kseries.size());
                        m_KDataVecs[m_seriesIndex]->initKBar(m_insInfo.instrumentID, std::max(m_kTime->beginPstime, 0),
                                                 m_kTime->endPstime, m_tradingday, m_lastPMD);

                    }

                    m_seriesIndex += 1;
                    m_kTime = m_kseriesTime->getNext();
                    if (psTime >= m_kTime->endPstime) {
                        //                        fprintf(stderr, "init k pmd 2 : instrumentid=%s, updateTime=%s, millisec=%d, volume=%d, m_seriesIndex=%d, m_kseries.size()=%d\n",
                        //                                pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, m_seriesIndex, m_kseries.size());

                        m_KDataVecs[m_seriesIndex]->initKBar(m_insInfo.instrumentID, std::max(m_kTime->beginPstime, 0), m_kTime->endPstime,
                                                    m_tradingday,m_lastPMD);
                    }
                }

            }

            void addTick(const  Types::MarketData *pMD, bool isDay) {

                 // if (strcmp(pMD->instrumentID.data(), "i2502P890") ==0 && pMD->settlementPrice > 0.0 && pMD->settlementPrice < 99999999.0 ) {
                 //     int a = 1;
                 // }

                int psTime = pMD->psSecond;
                if ( Utils::TradingHours::getProductTrait(pMD->productID, pMD->psSecond, isDay) ==
                     Utils::FTTrait::FT_AUCTION) {
                    if (psTime < 0) {
                        psTime = 0;
                    }else {
                        psTime +=61;
                    }
                //    psTime = (psTime + 300) / (60 * 30);
                } else if (pMD->settlementPrice > 0.0 && pMD->settlementPrice < 99999999.0 )
                {
                    psTime = std::min(psTime, 15 * 60 * 60);

                }

                if(m_lastPMD == nullptr){
                    m_lastPMD = new Types::MarketData();
                    memcpy(m_lastPMD, pMD, sizeof(Types::MarketData));
                }

                ffill(psTime );

                if (psTime <= m_kTime->endPstime and psTime >= m_kTime->beginPstime) {
                    if (abs(m_KDataVecs[m_seriesIndex]->m_open - 0.0) < 0.001) {
//                        fprintf(stderr, "init k pmd 3: instrumentid=%s, updateTime=%s, millisec=%d, volume=%d, m_seriesIndex=%d, m_kseries.size()=%d\n",
//                                pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, m_seriesIndex, m_kseries.size());
                        if (m_Period ==  Types::KPeriod::D1) {
                            m_KDataVecs[m_seriesIndex]->initKDayKBar(m_insInfo.instrumentID, std::max(m_kTime->beginPstime, 0),
                                m_kTime->endPstime, m_tradingday, pMD);
                        } else {
                            m_KDataVecs[m_seriesIndex]->initKBar(m_insInfo.instrumentID, std::max(m_kTime->beginPstime, 0),
                                m_kTime->endPstime,m_tradingday, pMD);
                        }
                    }
                    if (m_Period ==  Types::KPeriod::D1) {
                        m_KDataVecs[m_seriesIndex]->updateDayKBar(pMD->lastPrice, pMD->highestPrice, pMD->lowestPrice,
                                                                       pMD->openPrice, pMD->volume,
                                                                       pMD->amount, pMD->oi, pMD->settlementPrice);
                    } else {
                        m_KDataVecs[m_seriesIndex]->update(pMD);
                    }


                    memcpy(m_lastPMD, pMD, sizeof(Types::MarketData));
                }
            }

            void setHistoryKLine(std::vector<KData *> &ktimeVec) {

                for (int i = ktimeVec.size() - 1; i >= 0; i--) {
                    ktimeVec[i]->m_beginPsTime += m_biasSeconds;
                    ktimeVec[i]->m_endPsTime += m_biasSeconds;
                    m_KDataVecs.emplace_back(ktimeVec[i]);
                    m_seriesIndex++;

                }
                //    m_seriesIndex = ktimeVec.size()-1 >0 ? ktimeVec.size()-1 : 0;
                if (ktimeVec.size() > 0) {
                    int lastTradingday = ktimeVec[0]->m_tradingDay;
                    int lastKBeginTime = ktimeVec[0]->m_beginPsTime; // Utils::ToPsSeconds(ktimeVec[0]->m_updateTimeBegin, false);
                    m_kseriesTime->align(lastTradingday, lastKBeginTime);
                }
                int remainToday = 0;
                m_kTime = m_kseriesTime->getCurrent(remainToday);
                for (auto i = 0; i < remainToday; i++) {
                    KData *kline = new KData();
                    m_KDataVecs.emplace_back(kline);
                    //   m_seriesIndex++;
                }
            }

        };


    }
}


#endif //Cosmos_KSERIES_H
