//
// Created by zhangyw on 4/17/20.
//

#ifndef HFT_MM_OPENENGINE_H
#define HFT_MM_OPENENGINE_H



#include "../Driver/RealtimeDriver.h"
#include "../Types/Param.h"
//#include "../KData/KDataManager.h"
#include "../KAmountBarEngine/KAmountManager.h"
#include "../Utils/cppmysql.h"
#include <fstream>
#include <algorithm>
#include <ranges>

namespace Cosmos {
    namespace KAmountBarEngine {
        struct SaveK {
             Types::Instrument_t instrument{""};
             Types::KPeriod period;
             Types::UpdateTime_t updateTimeBegin{""};
             Types::UpdateTime_t updateTimeEnd{""};
            std::array<char, 512> sql{""};
        };

        class KAmountBarReadEngine {
        private:
            Driver::TestDriver *m_driver;
            std::vector< Types::InstrumentInfo> *m_futureInstruments;

            KAmountManager * m_kDataManager{nullptr};
            Utils::CppMySQL3DB *m_mySql{nullptr};
            std::map< Types::Instrument_t, Types::InstrumentInfo> m_instrumentInfoMap;

            std::string m_savePath{""};

        public:
            std::string m_engineName;

            std::vector<SaveK> m_futureDayQueue;
            std::vector<SaveK> m_futureMinutesQueue;
            std::vector<SaveK> m_futureOneMinuteQueue;

            double m_riskFreeR{0.023};

            int m_policyID{-1};
            int m_tradingDay{0};
            int m_isDay;
            bool m_isUseUnderlyPrice{true};

            KAmountBarReadEngine(decltype(m_driver) driver, std::string &_policyName,
                           std::vector< Types::InstrumentInfo> &futureInstruments,
                           int tradingday, int isDay, std::string &savePath, Utils::CppMySQL3DB *mysql) : m_engineName(_policyName),
                                                                               m_tradingDay(tradingday), m_isDay(isDay),
                                                                               m_savePath(savePath), m_mySql(mysql) {

                m_futureInstruments = &futureInstruments;
                m_driver = driver;
                m_driver->add_receiver< Types::OnSubScribeQuote>(
                    m_driver->passn([this]( Types::OnSubScribeQuote const &onSubScribeQuote) {
                        onRtnSubScribeQuote(std::forward<decltype(onSubScribeQuote)>(onSubScribeQuote));
                    }));
            }

            virtual ~KAmountBarReadEngine() {
            }

            void onInstrumentInfo( Types::InstrumentInfo const &instrumentInfo) {
            };

            void onParams( Types::InitParam const &param) {
            };

            void onStart() {

           //     m_kDataManager = new  KData::KDataManager(m_tradingDay, m_isDay, m_isUseUnderlyPrice, nullptr, 0);
                m_kDataManager = new  KAmountManager(m_tradingDay, m_isDay, m_mySql);
                for (auto &ins: *m_futureInstruments) {
                  //  fprintf(stderr, "KBarReadEngine::onStart futureInstruments : instrument=%s\n", ins.instrumentID.data());
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
                if (strcmp(onSubScribeQuote.instrumentID.data(), "IH2612")==0) {
                     int a = 1;
                }
                if (m_kDataManager->m_allKLineSeries.find(onSubScribeQuote.instrumentID) ==
                    m_kDataManager->m_allKLineSeries.end()) {
                    auto itrInsInfo = m_instrumentInfoMap.find(onSubScribeQuote.instrumentID);
                    if (itrInsInfo == m_instrumentInfoMap.end()) {
                        assert(false);
                    }
                    double perDayHisAmount = initHisDayAmount(m_tradingDay, itrInsInfo->first);
                    initKSeries(m_tradingDay, m_isDay, itrInsInfo->second, perDayHisAmount );
                }
            };

            double initHisDayAmount(int m_tradingDay, Types::Instrument_t const &instrument) {
                std::vector< KAmountData *> hisKline;
                m_kDataManager->getHisKbars(instrument, Types::ProductClass::future, Types::KPeriod::D1, hisKline, m_tradingDay,
                                               false, m_isDay);
                double allHisAmount = 0.0;
                int index = 0;
                for (const auto &hisKdata : hisKline ) {
                    allHisAmount += hisKdata->m_amount;
                    index++;
                    if (index > 20) {
                        break;
                    }
                }
                if (index ==0) {
                    return 9999999999999.0;
                }
                return allHisAmount  / index;
            };

            void initKSeries(int tradingday, bool isDay, Types::InstrumentInfo const &insInfo, double perDayHisAmount ) {
                std::vector< KAmountData *> hisKline;
                auto tradingSession =  Utils::TradingHours::getTradingSession(insInfo.productID);
                int tradingMinutes = 0;
                for ( auto& duration  : tradingSession->tradingVec) {
                    tradingMinutes += (duration.endTime - duration.beginTime) / 60;
                }

                for (auto period: Types::m_kperoidVec) {
                    double coef = 0.3;
                    // if (period == Types::KPeriod::Min1 ) {
                    //     coef = 0.3;
                    // }else if (period == Types::KPeriod::Min5 ) {
                    //     coef = 0.3;
                    // }else if (period == Types::KPeriod::Min15 ) {
                    //     coef = 0.3;
                    // }else {
                    //     assert(false);
                    // }

                    double perBarAmountThresh = perDayHisAmount * coef * sqrt(Types::KPeroidToIntervalVec[static_cast<int>(period)]) / tradingMinutes;

                    m_kDataManager->initKSeries(insInfo, period, tradingday, m_riskFreeR,
                     hisKline, isDay, perBarAmountThresh);

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

            void saveFutureKline(KAmountSeries *futureSeries, Types::KPeriod period) {
                if (futureSeries->m_KAmountDataVecs.size()  <1) {
                    return;
                }
                if (period == Types::KPeriod::D1) {
                    if (futureSeries->m_KAmountDataVecs.size() > futureSeries->m_seriesIndex) {
                        this->_saveFutureKline(futureSeries, futureSeries->m_seriesIndex, period);
                    }
                } else {
                    int saveIndex = 0;
                    while (saveIndex <= futureSeries->m_KAmountDataVecs.size()-1) { //not save last
                        this->_saveFutureKline(futureSeries, saveIndex, period);
                        saveIndex++;
                    }
                }
            }

            void _saveFutureKline(KAmountSeries *series, int saveIndex, Types::KPeriod period) {
                auto kline = series->m_KAmountDataVecs[saveIndex];
                if (strcmp(kline->m_instrument.data(), "") == 0) {
                    return;
                }
                SaveK saveK;
                strcpy(saveK.instrument.data(), kline->m_instrument.data());
                strcpy(saveK.updateTimeBegin.data(), kline->m_updateTimeBegin.data());
                strcpy(saveK.updateTimeEnd.data(), kline->m_updateTimeEnd.data());
                saveK.period = period;


                if (period == Cosmos::Types::KPeriod::D1 && m_isDay == true) {
                    sprintf(saveK.sql.data(), "%s,%d,%d,%s,%s,%s,%.1f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f",
                            kline->m_instrument.data(), kline->m_tradingDay,
                           Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(),
                            kline->m_updateTimeEnd.data(),
                            kline->m_productID.data(), series->m_amountThreshold,
                            kline->m_open,
                            kline->m_high, kline->m_low,
                            kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_upperLimit, kline->m_lowerLimit, kline->m_settlement);
                    m_futureDayQueue.emplace_back(saveK);
                } else if (period == Cosmos::Types::KPeriod::Min1 and strcmp(saveK.instrument.data(), "") != 0) {
                    sprintf(saveK.sql.data(), "%s,%d,%d,%s,%s,%s,%.1f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d",
                            kline->m_instrument.data(), kline->m_tradingDay,
                          Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(),
                            kline->m_updateTimeEnd.data(),
                            kline->m_productID.data(), series->m_amountThreshold,
                            kline->m_open, kline->m_high, kline->m_low,
                            kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume);
                    m_futureOneMinuteQueue.emplace_back(saveK);
                } else if (period == Cosmos::Types::KPeriod::Min5 ||
                           period == Cosmos::Types::KPeriod::Min15 ||
                           period == Cosmos::Types::KPeriod::Min30 ) {
                    sprintf(saveK.sql.data(), "%s,%d,%d,%s,%s,%s,%.1f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d",
                            kline->m_instrument.data(), kline->m_tradingDay,
                            Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            kline->m_updateTimeBegin.data(),
                            kline->m_updateTimeEnd.data(),
                            kline->m_productID.data(), series->m_amountThreshold,
                            kline->m_open, kline->m_high, kline->m_low,
                            kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                            kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume);
                    m_futureMinutesQueue.emplace_back(saveK);
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
                // std::sort(std::begin(klineQueue), std::end(klineQueue),
                //           [](auto &a, auto &b) {
                //               if (strcmp(a.instrument.data(), b.instrument.data()) == 0) {
                //                   if (a.period == b.period) {
                //                       return strcmp(a.updateTimeBegin.data(), b.updateTimeBegin.data()) > 0 ? false : true;
                //                   } else {
                //                       return a.period < b.period;
                //                   }
                //               } else {
                //                    Types::Instrument_t tempa{""};
                //                   for (auto i = 0; i < tempa.size(); i++) {
                //                       tempa[i] = std::tolower(a.instrument[i]);
                //                   }
                //
                //                    Types::Instrument_t tempb{""};
                //                   for (auto i = 0; i < tempb.size(); i++) {
                //                       tempb[i] = std::tolower(b.instrument[i]);
                //                   }
                //                   return strcmp(tempa.data(), tempb.data()) > 0 ? false : true;
                //               }
                //           });
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

                    }
                }

                _dumpKline(m_futureDayQueue, std::string("futureDay"), fileName);
                _dumpKline(m_futureMinutesQueue, std::string("futureMinutes"), fileName);
                _dumpKline(m_futureOneMinuteQueue, std::string("futureOneMinute"), fileName);

            }
        };
    }
}


#endif //HFT_MM_OPENENGINE_H
