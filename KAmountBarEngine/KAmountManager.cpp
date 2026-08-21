//
// Created by zhangyingwei on 2026/5/15.
//

#include "KAmountManager.h"


namespace Cosmos {
    namespace KAmountBarEngine {
        KAmountSeries *KAmountManager::KMAddTick(const Types::MarketData *pMD, Types::KPeriod period) {
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
                if ( fTTrait ==  Utils::FTTrait::FT_TRADING) {
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

        void KAmountManager::KMAddTick(const Types::MarketData *pMD) {
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
                    if ( fTTrait == Utils::FTTrait::FT_TRADING) {
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

        KAmountSeries * KAmountManager::getSeries(Types::Instrument_t const &instrument, Types::KPeriod period) {
            auto itr = m_allKLineSeries.find(instrument);
            if (itr == m_allKLineSeries.end()) {
                fprintf(stderr, "KLineManager getSeries %s not find\n", instrument.data());
                assert(false && "KLineManager getSeries");
            }

            auto peroid_itr = itr->second->find(period);
            if (peroid_itr == itr->second->end()) {
                fprintf(stderr, "KLineManager getSeries period= %d not find\n", Types::KPeroidToIntervalVec[static_cast<int>(period)]);
                assert(false && "KLineManager period getSeries");
            }
            return peroid_itr->second;
        }

        void KAmountManager::getHisKbars(Types::Instrument_t const &instrument, Types::ProductClass productClass,
                                       Types::KPeriod period, std::vector<KAmountData *> &hisKdata,
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
        //    fprintf(stderr, "%s\n", sql.data());

            Utils::CppMySQLQuery rs = m_mysql->querySQL(sql.data());
            while (!rs.eof()) {
                KAmountData *kData = new KAmountData();
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

        void KAmountManager::initKSeries(Types::InstrumentInfo const &insInfo, Types::KPeriod period,
                                       int tradingDay, double riskFreeR, std::vector<KAmountData *> &historyKline,
                                       bool isDay, double perBarAmountThresh) {
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
                std::unordered_map<Types::KPeriod, KAmountSeries *> *kseriesTemp =
                        new std::unordered_map<Types::KPeriod, KAmountSeries *>();
                m_allKLineSeries[insInfo.instrumentID] = kseriesTemp;
                itr = m_allKLineSeries.find(insInfo.instrumentID);
            }

            auto period_itr = itr->second->find(period);
            if (period_itr == itr->second->end()) {
                KAmountSeries *kSeries = new KAmountSeries(insInfo, tradingDay, riskFreeR, period, *tradingSession, isDay, perBarAmountThresh);

                kSeries->setHistoryKLine(historyKline);
                itr->second->insert({period, kSeries});
            }
        }



        void KAmountManager::checkSeriesRecord(KAmountSeries *series, int lastSeriesIndex) {

        }
    }
}
