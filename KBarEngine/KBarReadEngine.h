//
// Created by zhangyw on 4/17/20.
//

#ifndef HFT_MM_OPENENGINE_H
#define HFT_MM_OPENENGINE_H


#include "../Utils/MemoryList.h"
#include "../Driver/RealtimeDriver.h"
#include "../Types/Param.h"
#include "../Types/KPeriod.h"
#include "../KData/KDataManager.h"
#include "../Utils/cppmysql.h"
#include <set>
#include <fstream>
#include <algorithm>


namespace Cosmos {
    namespace KBarSaverEngine {
        struct SaveK {
             Types::Instrument_t instrument{""};
             Types::KPeriod period;
             Types::UpdateTime_t updateTimeBegin{""};
             Types::UpdateTime_t updateTimeEnd{""};
            std::array<char, 256> sql{""};
        };

        class KBarReadEngine {
        private:
             Driver::TestDriver *m_driver;
            std::vector< Types::InstrumentInfo> *m_optionInstruments;
            std::vector< Types::InstrumentInfo> *m_futureInstruments;
            KData::KDataManager * m_kDataManager{nullptr};

            std::map< Types::Instrument_t, Types::InstrumentInfo> m_instrumentInfoMap;

            std::string m_savePath{""};

        public:
            std::string m_engineName;
            std::vector<SaveK> m_optionDayQueue;
            std::vector<SaveK> m_optionMinutesQueue;
            std::vector<SaveK> m_optionOneMinuteQueue;

            std::vector<SaveK> m_futureDayQueue;
            std::vector<SaveK> m_futureMinutesQueue;
            std::vector<SaveK> m_futureOneMinuteQueue;

            double m_riskFreeR{0.023};

            int m_policyID{-1};
            int m_tradingDay{0};
            int m_isDay;
            bool m_isUseUnderlyPrice{true};

            KBarReadEngine(decltype(m_driver) driver, std::string &_policyName,
                           std::vector< Types::InstrumentInfo> &optionInstruments,
                           std::vector< Types::InstrumentInfo> &futureInstruments,
                           int tradingday, int isDay, std::string &savePath) : m_engineName(_policyName),
                                                                               m_tradingDay(tradingday), m_isDay(isDay),
                                                                               m_savePath(savePath) {

                m_optionInstruments = &optionInstruments;
                m_futureInstruments = &futureInstruments;
                m_driver = driver;
                m_driver->add_receiver< Types::OnSubScribeQuote>(
                    m_driver->passn([this]( Types::OnSubScribeQuote const &onSubScribeQuote) {
                        onRtnSubScribeQuote(std::forward<decltype(onSubScribeQuote)>(onSubScribeQuote));
                    }));
            }

            virtual ~KBarReadEngine() {
            }

            void onInstrumentInfo( Types::InstrumentInfo const &instrumentInfo) {
            };

            void onParams( Types::InitParam const &param) {
            };

            void onStart() {

                m_kDataManager = new  KData::KDataManager(m_tradingDay, m_isDay, m_isUseUnderlyPrice, nullptr, 0);
                for (auto &ins: *m_futureInstruments) {
                  //  fprintf(stderr, "KBarReadEngine::onStart futureInstruments : instrument=%s\n", ins.instrumentID.data());
                    Types::SubScribeQuote subScribeQuote;
                    memcpy(&subScribeQuote.instrumentID, &ins.instrumentID, sizeof(Types::Instrument_t));
                    subScribeQuote.policyID = m_policyID;
                    m_instrumentInfoMap[ins.instrumentID] = ins;
                    m_driver->subscribeQuote(subScribeQuote);
                }

                for (auto &ins: *m_optionInstruments) {

                    auto itr = m_instrumentInfoMap.find(ins.underly);
                    if (itr == m_instrumentInfoMap.end()) {
                        Types::InstrumentInfo underlyInsInfo;
                        strcpy(underlyInsInfo.instrumentID.data(), ins.underly.data());
                        underlyInsInfo.productIDClass =  Types::ProductClass::future;
                        m_instrumentInfoMap[ins.underly] = underlyInsInfo;
                        Cosmos::Types::SubScribeQuote subScribeUnderly;
                        memcpy(&subScribeUnderly.instrumentID, &ins.underly, sizeof(Types::Instrument_t));
                        subScribeUnderly.policyID = m_policyID;
                        m_driver->subscribeQuote(subScribeUnderly);
                    }
                     Types::SubScribeQuote subScribeQuote;
                    memcpy(&subScribeQuote.instrumentID, &ins.instrumentID, sizeof(Types::Instrument_t));
                    subScribeQuote.policyID = m_policyID;
                    m_instrumentInfoMap[ins.instrumentID] = ins;
                    m_driver->subscribeQuote(subScribeQuote);
                }
                Types::SubscribeEngine subscribeEngine;
                subscribeEngine.policyid = m_policyID;
                m_driver->subscribePolicy(subscribeEngine, this);
            };
            
            void onRtnSubScribeQuote( Types::OnSubScribeQuote const &onSubScribeQuote) {
             //   fprintf(stderr, "onRtnSubScribeQuote : instrument=%s\n", onSubScribeQuote.instrumentID.data());
                if (strcmp(onSubScribeQuote.instrumentID.data(), "MA608")==0) {
                     int a = 1;
                }
                if (m_kDataManager->m_allKLineSeries.find(onSubScribeQuote.instrumentID) ==
                    m_kDataManager->m_allKLineSeries.end()) {
                    auto itrInsInfo = m_instrumentInfoMap.find(onSubScribeQuote.instrumentID);
                    if (itrInsInfo == m_instrumentInfoMap.end()) {
                        assert(false);
                    }
                    initKSeries(m_tradingDay, m_isDay, itrInsInfo->second);
                }
            };

            void initKSeries(int tradingday, bool isDay, Types::InstrumentInfo const &insInfo) {
                Utils::TradingHours::initInstrumentTradingHours(insInfo.instrumentID);

                std::vector< KData::KData *> hisKline;
                for (auto period: Types::m_kperoidVec) {
                    m_kDataManager->initKSeries(insInfo, period, tradingday, m_riskFreeR,
                                               hisKline, isDay);
                }
            }

            void onEventData(Types::EventData const &eventData) {
                if (eventData.eventType == Types::EventType::marketEvent) {
                    auto pMD = (const Types::MarketData *) eventData.point;
                     // if (strcmp(pMD->instrumentID.data(), "i2502P890")==-0 ) {
                    // fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, volume=%d, epoch_time=%ld \n",
                    //  pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, pMD->epoch_time);
                     // }

                    for (auto period : Types::m_kperoidVec) {
                        m_kDataManager->KMAddTick(pMD, period);
                    }
                    // if (strcmp(pMD->instrumentID.data(),"IM2606") == 0 ) {
                    // fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, volume=%d, epoch_time=%ld \n",
                    //     pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, pMD->epoch_time);
                    // }
                }
            };

            void saveFutureKline(KData::KSeries *futureSeries, Types::KPeriod period) {
                if (futureSeries->m_KDataVecs.size()  <1) {
                    return;
                }
                if (period == Types::KPeriod::D1) {
                    if (futureSeries->m_KDataVecs.size() > futureSeries->m_seriesIndex) {
                        this->_saveFutureKline(futureSeries, futureSeries->m_seriesIndex, period);
                    }
                } else {
                    int saveIndex = 0;
                    while (saveIndex < futureSeries->m_KDataVecs.size()-1) { //not save last
                        this->_saveFutureKline(futureSeries, saveIndex, period);
                        saveIndex++;
                    }
                }
            }

            void _saveFutureKline(KData::KSeries *series, int saveIndex, Types::KPeriod period) {
                auto kline = series->m_KDataVecs[saveIndex];
                if (strcmp(kline->m_instrument.data(), "") == 0) {
                    return;
                }
                SaveK saveK;
                strcpy(saveK.instrument.data(), kline->m_instrument.data());
                strcpy(saveK.updateTimeBegin.data(), kline->m_updateTimeBegin.data());
                strcpy(saveK.updateTimeEnd.data(), kline->m_updateTimeEnd.data());
                saveK.period = period;


                if (period == Cosmos::Types::KPeriod::D1 && m_isDay == true) {
                    sprintf(saveK.sql.data(), "%s,%d,%d,%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f",
                            kline->m_instrument.data(), kline->m_tradingday,
                           Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(),
                            kline->m_updateTimeEnd.data(),
                            kline->m_productID.data(), kline->m_open,
                            kline->m_high, kline->m_low,
                            kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_upperLimit, kline->m_lowerLimit, kline->m_settlement);
                    m_futureDayQueue.emplace_back(saveK);
                } else if (period == Cosmos::Types::KPeriod::Min1 and strcmp(saveK.instrument.data(), "") != 0) {
                    sprintf(saveK.sql.data(), "%s,%d,%d,%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d",
                            kline->m_instrument.data(), kline->m_tradingday,
                          Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(),
                            kline->m_updateTimeEnd.data(),
                            kline->m_productID.data(), kline->m_open,
                            kline->m_high, kline->m_low,
                            kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume);
                    m_futureOneMinuteQueue.emplace_back(saveK);
                } else if (period == Cosmos::Types::KPeriod::Min5 ||
                           period == Cosmos::Types::KPeriod::Min15 ||
                           period == Cosmos::Types::KPeriod::Min30 ) {
                    sprintf(saveK.sql.data(), "%s,%d,%d,%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d",
                            kline->m_instrument.data(), kline->m_tradingday,
                            Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(),
                            kline->m_updateTimeEnd.data(),
                            kline->m_productID.data(), kline->m_open,
                            kline->m_high, kline->m_low,
                            kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume);
                    m_futureMinutesQueue.emplace_back(saveK);
                }

                kline->isInsert = true;
            };

            void saveOptionKline(KData::KSeries *optionSeries, Types::KPeriod period) {
                if (optionSeries->m_KDataVecs.size()  <1) {
                    return;
                }
               // if (optionSeries->m_KDataVecs.size() > optionSeries->m_seriesIndex) {
               //     auto optionKData = optionSeries->m_KDataVecs[optionSeries->m_KDataVecs.size() - 1];
               //     if (optionKData->m_tradingday != 0 &&  optionSeries->m_underlySeries->m_lastPMD != nullptr) {
               //        optionSeries->calGreeks(optionSeries->m_KDataVecs.size() - 1,
               //                               optionSeries->m_KDataVecs.size() - 1,
               //                               optionSeries->m_underlySeries->m_lastPMD->lastPrice,
               //                                optionSeries->m_underlySeries->m_lastPMD->psSecond);
               //     }
               // }
               if (period == Types::KPeriod::D1) {
                   if (optionSeries->m_KDataVecs.size() > optionSeries->m_seriesIndex) {
                       this->_saveOptionKline(optionSeries, optionSeries->m_seriesIndex, period);
                   }
               } else {
                   int saveIndex = 0;
                   while (saveIndex < optionSeries->m_KDataVecs.size() -1) {  //not save last
                       this->_saveOptionKline(optionSeries, saveIndex, period);
                       saveIndex++;
                   }
               }
           }

            void _saveOptionKline(KData::KSeries *series, int saveIndex, Types::KPeriod period) {

                auto kline = series->m_KDataVecs[saveIndex];
                if (strcmp(kline->m_instrument.data(), "") == 0) {
                    return;
                }
                SaveK saveK;
                strcpy(saveK.instrument.data(), kline->m_instrument.data());
                strcpy(saveK.updateTimeBegin.data(), kline->m_updateTimeBegin.data());
                saveK.period = period;
                //   fprintf(stderr, "%s\n", saveK.sql.data());
                if (period ==  Types::KPeriod::D1 && m_isDay == true) {
                    sprintf(saveK.sql.data(),
                            "%s,%s,%c,%.1f,%d,%d,%d,%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.5f,%.5f,%.5f,%.5f,%.5f",
                            kline->m_instrument.data(), series->m_insInfo.underly.data(), series->m_insInfo.optionType, series->m_insInfo.strikePrice,
                            kline->m_tradingday,series->m_insInfo.expireDate,  Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(),   kline->m_updateTimeEnd.data(), kline->m_productID.data(),
                            kline->m_open, kline->m_high, kline->m_low,
                            kline->m_close, kline->m_forwardPrice,  (double) kline->m_volume, kline->m_amount, kline->m_oi,
                             kline->m_settlement, kline->IV,
                             kline->delta, kline->gamma, kline->vega, kline->theta);
                    m_optionDayQueue.emplace_back(saveK);
                } else if (period ==  Types::KPeriod::Min1 and strcmp(saveK.instrument.data(), "") != 0) {
                    sprintf(saveK.sql.data(),
                            "%s,%s,%c,%.1f,%d,%d,%d,%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f",
                            kline->m_instrument.data(), series->m_insInfo.underly.data(), series->m_insInfo.optionType, series->m_insInfo.strikePrice,
                            kline->m_tradingday,series->m_insInfo.expireDate, Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(), kline->m_updateTimeEnd.data(), kline->m_productID.data(),
                            kline->m_open, kline->m_high, kline->m_low,
                            kline->m_close, kline->m_forwardPrice, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume,
                            kline->IV, kline->bidIV, kline->askIV, kline->delta, kline->gamma, kline->vega, kline->theta);
                    m_optionOneMinuteQueue.emplace_back(saveK);
                } else if (period == Cosmos::Types::KPeriod::Min5 ||
                           period == Cosmos::Types::KPeriod::Min15 ||
                           period == Cosmos::Types::KPeriod::Min30) {
                    sprintf(saveK.sql.data(),
                            "%s,%s,%c,%.1f,%d,%d,%d,%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f",
                            kline->m_instrument.data(),series->m_insInfo.underly.data(), series->m_insInfo.optionType, series->m_insInfo.strikePrice, kline->m_tradingday,
                            series->m_insInfo.expireDate,
                            Types::KPeroidToIntervalVec[static_cast<int>(period)],
                                 kline->m_updateTimeBegin.data(), kline->m_updateTimeEnd.data(), kline->m_productID.data(),
                            kline->m_open, kline->m_high, kline->m_low,
                            kline->m_close,  kline->m_forwardPrice,(double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume,
                            kline->IV, kline->bidIV,  kline->askIV,  kline->delta, kline->gamma, kline->vega, kline->theta);
           //         fprintf(stderr, "%s", saveK.sql.data());
                    m_optionMinutesQueue.emplace_back(saveK);
                }
                kline->isInsert = true;
            };

            void _dumpKline(std::vector<SaveK> &klineQueue, std::string &&name, std::string &fileName) {
                if (klineQueue.size()==0) {
                    return;
                }
                char savePath[256]{""};
                std::ofstream outDayfile;
                sprintf(savePath, "%s/%s_%s.txt", m_savePath.c_str(), name.c_str(), fileName.c_str());
                outDayfile.open(savePath, std::ios::out | std::ios::app);
                std::sort(std::begin(klineQueue), std::end(klineQueue),
                          [](auto &a, auto &b) {
                              if (strcmp(a.instrument.data(), b.instrument.data()) == 0) {
                                  if (a.period == b.period) {
                                      return strcmp(a.updateTimeBegin.data(), b.updateTimeBegin.data()) > 0 ? false : true;
                                  } else {
                                      return a.period < b.period;
                                  }
                              } else {
                                   Types::Instrument_t tempa{""};
                                  for (auto i = 0; i < tempa.size(); i++) {
                                      tempa[i] = std::tolower(a.instrument[i]);
                                  }

                                   Types::Instrument_t tempb{""};
                                  for (auto i = 0; i < tempb.size(); i++) {
                                      tempb[i] = std::tolower(b.instrument[i]);
                                  }
                                  return strcmp(tempa.data(), tempb.data()) > 0 ? false : true;
                              }
                          });
                for (auto &kline: klineQueue) {
                    outDayfile << kline.sql.data() << "\n";
                }
                outDayfile.close();
            }

            void dumpKline(std::string &fileName) {
                for (auto &kv: m_kDataManager->m_allKLineSeries) {
                    for (auto &kse: *kv.second) {
                        if (kse.second->m_insInfo.productIDClass == Types::ProductClass::future) {
                            saveFutureKline(kse.second, kse.first);
                        }
                        else if (kse.second->m_insInfo.productIDClass == Types::ProductClass::option) {
                            saveOptionKline(kse.second, kse.first);
                        }
                    }
                }

                _dumpKline(m_futureDayQueue, std::string("futureDay"), fileName);
                _dumpKline(m_futureMinutesQueue, std::string("futureMinutes"), fileName);
                _dumpKline(m_futureOneMinuteQueue, std::string("futureOneMinute"), fileName);

                _dumpKline(m_optionDayQueue, std::string("optionDay"), fileName);
                _dumpKline(m_optionMinutesQueue, std::string("optionMinutes"), fileName);
                _dumpKline(m_optionOneMinuteQueue, std::string("optionOneMinute"), fileName);
            }
        };
    }
}


#endif //HFT_MM_OPENENGINE_H
