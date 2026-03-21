//
// Created by zhangyw on 1/9/21.
//

#ifndef TESTHFT3_KDATA_H
#define TESTHFT3_KDATA_H


#include <array>
#include <map>
#include <unordered_map>
#include "../Utils/TradingHours.h"
#include <vector>
#include <string>
#include <cassert>
#include "../Utils/cppmysql.h"
#include "../Utils/Utils.h"
//#include "MockBase.h"
#include "../Types/Type.h"
#include "KSeries.h"
#include "../Types/KPeriod.h"
#include "../Driver/RealtimeDriver.h"
#include "UpdateGreeks.h"

namespace TrendFollow {
    namespace KData {
        class KDataManager {
        public:
            Utils::CppMySQL3DB *m_mysql{nullptr};
            std::map<Types::Instrument_t, std::unordered_map<Types::KPeriod, KSeries *> *> m_allKLineSeries;
            //       std::map<std::tuple< Types::Instrument_t,  Types::KPeriod>, std::map<int, CalPutSeries *> *> m_underlyToOptionSeriesMap;
            int m_tradingday{0};
            bool m_isUseUnderlyPrice{true};
            bool m_isDay{true};
            UpdateGreeks *m_updateGreeks{nullptr};

            KDataManager(int tradingDay, bool isDay, bool isUseUnderlyPrice) : m_tradingday(tradingDay),
                                                                               m_isDay(isDay),
                                                                               m_isUseUnderlyPrice(isUseUnderlyPrice) {
                m_updateGreeks = new UpdateGreeks();
            }


            KSeries* KMAddTick(const Types::MarketData *pMD, Types::KPeriod period) {
                // auto itr = this->m_allKLineSeries.find(pMD->instrumentID);
                // if (itr == this->m_allKLineSeries.end()) {
                //     fprintf(stderr, "onEventData %s\n", pMD->instrumentID.data());
                //     assert(false);
                // }


                auto series = this->getSeries(pMD->instrumentID, period);
                int lastSeriesIndex = series->m_seriesIndex;
                series->addTick(pMD);
                if (lastSeriesIndex < series->m_seriesIndex) {
                    this->checkSeriesRecord(series, period, lastSeriesIndex);
                    return series;
                }
                return nullptr;
            }

            KSeries *getSeries(Types::Instrument_t const &instrument, Types::KPeriod period) {
                auto itr = m_allKLineSeries.find(instrument);
                if (itr == m_allKLineSeries.end()) {
                    assert(false && "KLineManager addTick");
                }

                auto peroid_itr = itr->second->find(period);
                if (peroid_itr == itr->second->end()) {
                    assert(false && "KLineManager period addTick");
                }
                return peroid_itr->second;
            }

            void getHisKbars(Types::Instrument_t const &instrument, Types::ProductClass productClass,
                             Types::KPeriod period, std::vector<KData *> &hisKdata,
                             int tradingday, bool isReal, bool isKbarSaver) {
                std::array<char, 400> sql{""};
                int limitValue = 350;
                if (period == Types::KPeriod::Min1) {
                    limitValue = 350;
                }

                std::string tradingdayComp = "<";

                if (isReal == true) {
                    tradingdayComp = "<=";
                }

                decltype( Types::KPeroidToFutureTableMap) *kptmp = &Types::KPeroidToFutureTableMap;
                if (productClass == Types::ProductClass::option) {
                    //  kptmp = & Types::KPeroidToOptionTableMap;
                    return;
                }
                if (period == Types::KPeriod::D1 || period == Types::KPeriod::Min30) {
                    if (isKbarSaver == true) {
                        sprintf(sql.data(),
                                "select * from %s where instrument='%s' and period=%d and tradingDay = %d order by tradingDay DESC,updateTime DESC limit %d",
                                (*kptmp)[period].data(), instrument.data(),
                                Types::KPeroidToIntervalMap[period], tradingday,
                                limitValue);
                    } else {
                        sprintf(sql.data(),
                                "select * from %s where instrument='%s' and period=%d and tradingDay %s %d order by tradingDay DESC,updateTime DESC limit %d",
                                (*kptmp)[period].data(), instrument.data(),
                                Types::KPeroidToIntervalMap[period], tradingdayComp.c_str(), tradingday,
                                limitValue);
                    }
                } else {
                    if (isKbarSaver == true) {
                        sprintf(sql.data(),
                                "select * from %s where instrument='%s' and period=%d and tradingDay = %d order by tradingDay DESC,updateTime DESC limit %d",
                                (*kptmp)[period].data(), instrument.data(),
                                Types::KPeroidToIntervalMap[period], tradingday,
                                limitValue);
                    } else {
                        sprintf(sql.data(),
                                "select * from %s where instrument='%s' and period=%d and tradingDay %s %d order by tradingDay DESC,updateTime DESC limit %d",
                                (*kptmp)[period].data(), instrument.data(),
                                Types::KPeroidToIntervalMap[period], tradingdayComp.c_str(), tradingday,
                                limitValue);
                    }

                    fprintf(stderr, "%s\n", sql.data());
                }


                Utils::CppMySQLQuery rs = m_mysql->querySQL(sql.data());
                while (!rs.eof()) {
                    KData *kData = new KData();
                    if (period == Types::KPeriod::D1) {
                        kData->m_tradingday = atoi(rs.getStringField("tradingDay"));
                        strcpy(kData->m_instrument.data(), rs.getStringField("instrument"));
                        strcpy(kData->m_updateTimeBegin.data(), rs.getStringField("updateTimeBegin"));
                        strcpy(kData->m_updateTimeEnd.data(), rs.getStringField("updateTimeEnd"));
                        strcpy(kData->m_productID.data(), rs.getStringField("productId"));
                        kData->m_open = rs.getFloatField("open");
                        kData->m_high = rs.getFloatField("high");
                        kData->m_low = rs.getFloatField("low");
                        kData->m_close = rs.getFloatField("close");
                        kData->m_volume = rs.getIntField("volume");
                        kData->m_amount = rs.getFloatField("amount");
                        kData->m_oi = rs.getFloatField("position");
                        kData->m_upperLimit = rs.getFloatField("upperLimit");
                        kData->m_lowerLimit = rs.getFloatField("lowerLimit");
                        kData->m_settlement = rs.getFloatField("settlement");
                        kData->m_beginPsTime = Utils::ToPsSeconds(kData->m_updateTimeBegin, false);
                        kData->m_endPsTime = Utils::ToPsSeconds(kData->m_updateTimeEnd, false);
                        hisKdata.emplace_back(kData);
                        rs.nextRow();
                    } else {
                        kData->m_tradingday = atoi(rs.getStringField("tradingDay"));
                        strcpy(kData->m_instrument.data(), rs.getStringField("instrument"));
                        strcpy(kData->m_updateTimeBegin.data(), rs.getStringField("updateTimeBegin"));
                        strcpy(kData->m_updateTimeEnd.data(), rs.getStringField("updateTimeEnd"));
                        strcpy(kData->m_productID.data(), rs.getStringField("productId"));
                        kData->m_open = rs.getFloatField("open");
                        kData->m_high = rs.getFloatField("high");
                        kData->m_low = rs.getFloatField("low");
                        kData->m_close = rs.getFloatField("close");
                        kData->m_volume = rs.getIntField("volume");
                        kData->m_amount = rs.getFloatField("amount");
                        kData->m_oi = rs.getFloatField("position");
                        kData->m_bidPrice = rs.getFloatField("bidPrice");
                        kData->m_askPrice = rs.getFloatField("askPrice");
                        kData->m_bidVolume = rs.getIntField("bidVolume");
                        kData->m_askVolume = rs.getIntField("askVolume");
                        kData->m_beginPsTime = Utils::ToPsSeconds(kData->m_updateTimeBegin, false);
                        kData->m_endPsTime = Utils::ToPsSeconds(kData->m_updateTimeEnd, false);
                        hisKdata.emplace_back(kData);
                        rs.nextRow();
                    }
                }
            }

            void initKSeries(Types::InstrumentInfo const &insInfo, Types::KPeriod period,
                             int tradingday, double riskFreeR, std::vector<KData *> &historyKline, bool isDay) {
                // fprintf(stderr, "initKSeries instrument=%s, peroid=%d, historeKline length=%d, lastBar=%d-%s, firstBar=%d-%s\n",instrument.data(),
                //			                                       Types::KPeroidToIntervalMap[period], historyKline.size(), historyKline[0]->m_tradingday ,historyKline[0]->m_updateTime.data(),
                //							                                     historyKline[historyKline.size()-1]->m_tradingday ,historyKline[historyKline.size()-1]->m_updateTime.data());
                if (historyKline.size() > 0) {
                    spdlog::info(
                        "initKSeries instrument={}, peroid={}, historeKline length={}, lastBar={}-{}, firstBar={}-{}",
                        insInfo.instrumentID.data(),
                        Types::KPeroidToIntervalMap[period], historyKline.size(), historyKline[0]->m_tradingday,
                        historyKline[0]->m_updateTimeBegin.data(),
                        historyKline[historyKline.size() - 1]->m_tradingday,
                        historyKline[historyKline.size() - 1]->m_updateTimeBegin.data());
                }

                m_tradingday = tradingday;
                auto tradingSession = Utils::TradingHours::getTradingSession(insInfo.instrumentID);

                auto itr = m_allKLineSeries.find(insInfo.instrumentID);
                if (itr == m_allKLineSeries.end()) {
                    std::unordered_map<Types::KPeriod, KSeries *> *kseriesTemp =
                            new std::unordered_map<Types::KPeriod, KSeries *>();
                    m_allKLineSeries[insInfo.instrumentID] = kseriesTemp;
                    itr = m_allKLineSeries.find(insInfo.instrumentID);
                }

                auto period_itr = itr->second->find(period);
                if (period_itr == itr->second->end()) {
                    KSeries *kSeries = new KSeries(insInfo, tradingday, riskFreeR, period, *tradingSession, isDay);

                    kSeries->setHistoryKLine(historyKline);
                    itr->second->insert({period, kSeries});

                    if (kSeries->m_insInfo.productIDClass == Types::ProductClass::option) {
                        auto itrUnderlySeriesMap = m_allKLineSeries.find(kSeries->m_insInfo.underly);
                        if (itrUnderlySeriesMap == m_allKLineSeries.end()) {
                            fprintf(stderr, "not find underly seriesMap\n");
                            assert(false);
                        }
                        auto itrUnderlySeries = itrUnderlySeriesMap->second->find(period);
                        if (itrUnderlySeries == itrUnderlySeriesMap->second->end()) {
                            fprintf(stderr, "not find underly series\n");
                            assert(false);
                        }
                        kSeries->setUnderlySeries(itrUnderlySeries->second);

                        _initUnderlyToOptionSeriesMap(kSeries, period);
                        m_updateGreeks->init(kSeries, period, m_tradingday, m_isDay);
                    }
                }
            }

            void _initUnderlyToOptionSeriesMap(KSeries *optionKSeries, Types::KPeriod const &kperiod) {
                // fprintf(stderr, "_initUnderlyToOptionSeriesMap : instrument=%s, period=%d\n",
                //     optionKSeries->m_insInfo.instrumentID.data(), Types::KPeroidToIntervalMap[kperiod]);
                // std::tuple< Types::Instrument_t,  Types::KPeriod> key =
                //                                   std::make_tuple(
                //                                       optionKSeries->m_underlySeries->m_insInfo.instrumentID,
                //                                      kperiod);
                //
                // auto itr = m_underlyToOptionSeriesMap.find(key);
                // if (itr == m_underlyToOptionSeriesMap.end()) {
                //     auto temp =new std::map<int, CalPutSeries *>();
                //     m_underlyToOptionSeriesMap.insert(std::make_pair(key, temp));
                //     itr = m_underlyToOptionSeriesMap.find(key);
                // }
                if (optionKSeries->m_underlySeries->m_callPutSeriesMap == nullptr) {
                    optionKSeries->m_underlySeries->m_callPutSeriesMap = new std::map<int, CallPutSeries *>();
                }
                auto callPutSeriesMap = optionKSeries->m_underlySeries->m_callPutSeriesMap;
                int strikePriceKey = static_cast<int>(optionKSeries->m_insInfo.strikePrice);
                auto strikePriceMapItr = callPutSeriesMap->find(strikePriceKey);
                if (strikePriceMapItr == callPutSeriesMap->end()) {
                    auto temp = new CallPutSeries();
                    (*(callPutSeriesMap))[strikePriceKey] = temp;
                    strikePriceMapItr = callPutSeriesMap->find(strikePriceKey);
                }
                if (optionKSeries->m_insInfo.optionType == 'C') {
                    strikePriceMapItr->second->callSeries = optionKSeries;
                } else if (optionKSeries->m_insInfo.optionType == 'P') {
                    strikePriceMapItr->second->putSeries = optionKSeries;
                }
            }

            double _calForwardPrice(const KSeries *underlySeries, std::map<int, CallPutSeries *> *calPutMap) {
                if (m_isUseUnderlyPrice == true) {
                    return underlySeries->m_lastPMD->midPrice;
                } else {
                    double minSpread = 9999;
                    double forwardPrice = 0.0;

                    for (auto &optionSeriesItr: *calPutMap) {
                        auto callSeries = optionSeriesItr.second->callSeries;
                        auto putSeries = optionSeriesItr.second->putSeries;
                        if (callSeries->m_lastPMD->bidVolume[0] == 0 || callSeries->m_lastPMD->askVolume[0] == 0 ||
                            putSeries->m_lastPMD->bidVolume[0] == 0 || putSeries->m_lastPMD->askVolume[0] == 0) {
                            continue;
                        }
                        double spread = (callSeries->m_lastPMD->askPrice[0] - callSeries->m_lastPMD->bidPrice[0]) +
                                        (putSeries->m_lastPMD->askPrice[0] - putSeries->m_lastPMD->bidPrice[0]);
                        if (spread < minSpread) {
                            minSpread = spread;
                            forwardPrice = callSeries->m_lastPMD->midPrice - putSeries->m_lastPMD->midPrice +
                                           optionSeriesItr.first;
                        }
                    }

                    if (minSpread > underlySeries->m_insInfo.tickSize * 5) {
                        forwardPrice = underlySeries->m_lastPMD->lastPrice;
                    }

                    return forwardPrice;
                }
            };

            void checkSeriesRecord(KSeries *series, Types::KPeriod const &kperiod, int lastSeriesIndex) {
                if (series->m_insInfo.productIDClass == Types::ProductClass::future && series->m_callPutSeriesMap !=
                    nullptr) {
                    auto underlySeries = series;
                    while (lastSeriesIndex < underlySeries->m_seriesIndex) {
                        double forwardPrice = _calForwardPrice(underlySeries, series->m_callPutSeriesMap);
                        m_updateGreeks->updateGreeks(underlySeries, forwardPrice, series->m_callPutSeriesMap,
                                                     lastSeriesIndex);
                        lastSeriesIndex++;
                        //     underlySeries->m_recordIndex += 1;
                    }
                }
            }

            void refreshSeries(int tradingday, bool isDay) {
                for (auto itrSeries = m_allKLineSeries.begin(); itrSeries != m_allKLineSeries.end(); itrSeries++) {
                    for (auto itrSerie = itrSeries->second->begin(); itrSerie != itrSeries->second->end(); itrSerie++) {
                        auto tradingSession = Utils::TradingHours::getTradingSession(itrSeries->first);
                        itrSerie->second->refreshSeries(tradingday, *tradingSession, isDay);
                    }
                }
            }
        };
    }
}
#endif //TESTHFT3_KDATA_H
