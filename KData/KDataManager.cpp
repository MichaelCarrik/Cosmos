//
// Created by zhangyingwei on 2026/5/15.
//

#include "KDataManager.h"


namespace Cosmos {
    namespace KData {
        KSeries *KDataManager::KMAddTick(const Types::MarketData *pMD, Types::KPeriod period) {
            // auto itr = this->m_allKLineSeries.find(pMD->instrumentID);
            // if (itr == this->m_allKLineSeries.end()) {
            //     fprintf(stderr, "onEventData %s\n", pMD->instrumentID.data());
            //     assert(false);
            // }


            auto series = this->getSeries(pMD->instrumentID, period);
            int lastSeriesIndex = series->m_seriesIndex;

            if (pMD->isInit == true) {
                Types::Product_t product{""};
                Utils::InstrumentToProduct(pMD->instrumentID, product);
                auto fTTrait  = Utils::TradingHours::getProductTrait(product, pMD->psSecond, m_isDay);
                if (fTTrait ==  Utils::FTTrait::FT_AUCTION || fTTrait ==  Utils::FTTrait::FT_TRADING) {
                    series->addTick(pMD, m_isDay);
                }
                lastSeriesIndex = series->m_seriesIndex;
                //  series->m_recordIndex = series->m_seriesIndex;
            } else {
                series->addTick(pMD, m_isDay);
                if (lastSeriesIndex < series->m_seriesIndex or
                    (series->m_Period == Types::KPeriod::D1 && pMD->settlementPrice > 0.0 && pMD->settlementPrice <
                     99999999.0)) {
                    this->checkSeriesRecord(series, lastSeriesIndex);
                    return series;
                }
            }

            return nullptr;
        }

        void KDataManager::KMAddTick(const Types::MarketData *pMD) {
            auto itr = m_allKLineSeries.find(pMD->instrumentID);
            if (itr == m_allKLineSeries.end()) {
                assert(false && "KLineManager addTick ");
            }
            for (auto &itrIns: *(itr->second)) {
                auto series = itrIns.second;
                int lastSeriesIndex = series->m_seriesIndex;

                if (pMD->isInit == true) {
                    Types::Product_t product{""};
                    Utils::InstrumentToProduct(pMD->instrumentID, product);
                    auto fTTrait = Utils::TradingHours::getProductTrait(product, pMD->psSecond, m_isDay);
                    if (fTTrait == Utils::FTTrait::FT_AUCTION || fTTrait == Utils::FTTrait::FT_TRADING) {
                        series->addTick(pMD, m_isDay);
                    }

                    lastSeriesIndex = series->m_seriesIndex;
                    //   series->m_recordIndex = series->m_seriesIndex;
                } else {
                    series->addTick(pMD, m_isDay);
                    if (lastSeriesIndex < series->m_seriesIndex) {
                        this->checkSeriesRecord(series, lastSeriesIndex);
                    }
                }
            }
        }

        KSeries *KDataManager::getSeries(Types::Instrument_t const &instrument, Types::KPeriod period) {
            auto itr = m_allKLineSeries.find(instrument);
            if (itr == m_allKLineSeries.end()) {
                assert(false && "KLineManager getSeries");
            }

            auto peroid_itr = itr->second->find(period);
            if (peroid_itr == itr->second->end()) {
                assert(false && "KLineManager period getSeries");
            }
            return peroid_itr->second;
        }

        void KDataManager::getHisKbars(Types::Instrument_t const &instrument, Types::ProductClass productClass,
                                       Types::KPeriod period, std::vector<KData *> &hisKdata,
                                       int tradingday, bool isReal, bool isDay) {
            std::array<char, 400> sql{""};
            int limitValue = 350;
            if (period == Types::KPeriod::Min1 || period == Types::KPeriod::Min5) {
                limitValue = 1950;
            }


            decltype( Types::KPeroidToFutureTableVec) *kptmp = &Types::KPeroidToFutureTableVec;
            if (productClass == Types::ProductClass::option) {
                //  kptmp = & Types::KPeroidToOptionTableMap;
                return;
            }

            if (isReal == true) {
                if (period == Types::KPeriod::D1) {
                    sprintf(sql.data(),
                            "select * from %s where instrument='%s' and period=%d and tradingDay < %d  order by tradingDay DESC,updateTimeBegin DESC limit %d",
                            (*kptmp)[static_cast<int>(period)].data(), instrument.data(),
                            Types::KPeroidToIntervalVec[static_cast<int>(period)], tradingday, limitValue);
                } else {
                    sprintf(sql.data(),
                            "select * from %s where instrument='%s' and period=%d and tradingDay <= %d  order by tradingDay DESC,updateTimeBegin DESC limit %d",
                            (*kptmp)[static_cast<int>(period)].data(), instrument.data(),
                            Types::KPeroidToIntervalVec[static_cast<int>(period)], tradingday, limitValue);
                }
            } else if (isReal == false && isDay == false) {
                if (period == Types::KPeriod::D1) {
                    sprintf(sql.data(),
                            "select * from %s where instrument='%s' and period=%d and tradingDay < %d  order by tradingDay DESC,updateTimeBegin DESC limit %d",
                            (*kptmp)[static_cast<int>(period)].data(), instrument.data(),
                            Types::KPeroidToIntervalVec[static_cast<int>(period)], tradingday, limitValue);
                } else {
                    sprintf(sql.data(),
                            "select * from %s where instrument='%s' and period=%d and tradingDay < %d order by tradingDay DESC,updateTimeBegin DESC limit %d",
                            (*kptmp)[static_cast<int>(period)].data(), instrument.data(),
                            Types::KPeroidToIntervalVec[static_cast<int>(period)], tradingday, limitValue);
                }
            } else if (isReal == false && isDay == true) {
                if (period == Types::KPeriod::D1) {
                    sprintf(sql.data(),
                            "select * from %s where instrument='%s' and period=%d and tradingDay < %d  order by tradingDay DESC,updateTimeBegin DESC limit %d",
                            (*kptmp)[static_cast<int>(period)].data(), instrument.data(),
                            Types::KPeroidToIntervalVec[static_cast<int>(period)], tradingday, limitValue);
                } else {
                    sprintf(sql.data(),
                            "select * from %s where instrument='%s' and period=%d and ( (tradingDay < %d) or (tradingDay = %d  and  updateTimeBegin <= '08:45:00')) "
                            "order by tradingDay DESC,updateTimeBegin DESC limit %d",
                            (*kptmp)[static_cast<int>(period)].data(), instrument.data(),
                            Types::KPeroidToIntervalVec[static_cast<int>(period)],
                            tradingday, tradingday, limitValue);
                }
            }
            fprintf(stderr, "%s\n", sql.data());

            Utils::CppMySQLQuery rs = m_mysql->querySQL(sql.data());
            while (!rs.eof()) {
                KData *kData = new KData();
                if (period == Types::KPeriod::D1) {
                    kData->m_tradingDay = atoi(rs.getStringField("tradingDay"));
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
                    kData->m_tradingDay = atoi(rs.getStringField("tradingDay"));
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
                    kData->m_sabrPRMT.alpha = rs.getFloatField("alpha");
                    kData->m_sabrPRMT.beta = rs.getFloatField("beta");
                    kData->m_sabrPRMT.rho = rs.getFloatField("rho");
                    kData->m_sabrPRMT.nu = rs.getFloatField("nu");
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

        void KDataManager::initKSeries(Types::InstrumentInfo const &insInfo, Types::KPeriod period,
                                       int tradingDay, double riskFreeR, std::vector<KData *> &historyKline,
                                       bool isDay) {
            // fprintf(stderr, "initKSeries instrument=%s, peroid=%d, historeKline length=%d, lastBar=%d-%s, firstBar=%d-%s\n",instrument.data(),
            //			                                       Types::KPeroidToIntervalMap[period], historyKline.size(), historyKline[0]->m_tradingday ,historyKline[0]->m_updateTime.data(),
            //							                                     historyKline[historyKline.size()-1]->m_tradingday ,historyKline[historyKline.size()-1]->m_updateTime.data());
            if (historyKline.size() > 0) {
                spdlog::info(
                    "initKSeries instrument={}, peroid={}, historeKline length={}, lastBar={}-{}, firstBar={}-{}",
                    insInfo.instrumentID.data(),
                    Types::KPeroidToIntervalVec[static_cast<int>(period)], historyKline.size(),
                    historyKline[0]->m_tradingDay,
                    historyKline[0]->m_updateTimeBegin.data(),
                    historyKline[historyKline.size() - 1]->m_tradingDay,
                    historyKline[historyKline.size() - 1]->m_updateTimeBegin.data());
            }

            m_tradingDay = tradingDay;
            auto tradingSession = Utils::TradingHours::getTradingSession(insInfo.productID);


            auto itr = m_allKLineSeries.find(insInfo.instrumentID);
            if (itr == m_allKLineSeries.end()) {
                std::unordered_map<Types::KPeriod, KSeries *> *kseriesTemp =
                        new std::unordered_map<Types::KPeriod, KSeries *>();
                m_allKLineSeries[insInfo.instrumentID] = kseriesTemp;
                itr = m_allKLineSeries.find(insInfo.instrumentID);
            }

            auto period_itr = itr->second->find(period);
            if (period_itr == itr->second->end()) {
                KSeries *kSeries = new KSeries(insInfo, tradingDay, riskFreeR, period, *tradingSession, isDay,
                                               m_biasSeconds);

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
                    m_updateOptionModelPamt->init(kSeries, period, m_tradingDay, m_isDay);
                }
            }
        }

        void KDataManager::_initUnderlyToOptionSeriesMap(KSeries *optionKSeries, Types::KPeriod const &kperiod) {
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

        double KDataManager::_calForwardPrice(const KSeries *underlySeries) {
            auto calPutMap = underlySeries->m_callPutSeriesMap;
            if (m_isUseUnderlyPrice == true && underlySeries->m_lastPMD->bidVolume[0] > 0 && underlySeries->m_lastPMD->
                askVolume[0] > 0) {
                return underlySeries->m_lastPMD->midPrice;
            } else {
                double minSpread = 9999;
                double forwardPrice = 0.0;

                for (auto &optionSeriesItr: *calPutMap) {
                    auto callSeries = optionSeriesItr.second->callSeries;
                    auto putSeries = optionSeriesItr.second->putSeries;
                    if (callSeries->m_lastPMD == nullptr || callSeries->m_lastPMD->bidVolume[0] == 0 || callSeries->
                        m_lastPMD->askVolume[0] == 0 ||
                        putSeries->m_lastPMD == nullptr || putSeries->m_lastPMD->bidVolume[0] == 0 || putSeries->
                        m_lastPMD->askVolume[0] == 0) {
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

                if (minSpread > underlySeries->m_insInfo.tickSize * 5 && underlySeries->m_lastPMD->bidVolume[0] > 0 &&
                    underlySeries->m_lastPMD->askVolume[0] > 0) {
                    forwardPrice = underlySeries->m_lastPMD->lastPrice;
                }

                return forwardPrice;
            }
        };

        void KDataManager::checkSeriesRecord(KSeries *series, int lastSeriesIndex) {
            if (series->m_insInfo.productIDClass == Types::ProductClass::future && series->m_callPutSeriesMap !=
                nullptr) {
                auto underlySeries = series;

                while (lastSeriesIndex < underlySeries->m_seriesIndex) {
                    double forwardPrice = _calForwardPrice(underlySeries);
                    m_updateOptionModelPamt->updateGreeks(underlySeries, forwardPrice, lastSeriesIndex);
                    m_updateOptionModelPamt->updateSabr(underlySeries, forwardPrice, lastSeriesIndex);
                    lastSeriesIndex++;
                    //     underlySeries->m_recordIndex += 1;
                }
            }
        }
    }
}
