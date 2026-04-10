//
// Created by zhangyw on 4/17/20.
//

#include "../Utils/Utils.h"
#include "KBarSaverEngine.h"

#include <stdlib.h>


namespace Cosmos {
    namespace KBarEngine {
        void KBarSaverEngine::onStart() {
            m_kDataManager = new KData::KDataManager(m_tradingDay, m_isDay, m_isUseUnderlyPrice, m_mysql,0);

            for (auto &insInfo: *m_tradeInsInfoVec) {
                m_instrumentInfoMap[insInfo->instrumentID] = insInfo;
            }

            for (auto &insItr: m_instrumentInfoMap) {
                if (insItr.second->productIDClass == Types::ProductClass::future) {
                    Types::SubScribeQuote subScribeQuote;
                    memcpy(&subScribeQuote.instrumentID, &insItr.second->instrumentID, sizeof(Types::Instrument_t));
                    subScribeQuote.policyID = m_policyID;
                    // m_instrumentInfoMap[insItr.second.instrumentID] = ins;
                    m_driver->subscribeQuote(subScribeQuote);
                }
            }
            for (auto &insItr: m_instrumentInfoMap) {
                if (insItr.second->productIDClass == Types::ProductClass::option) {
                    auto itr = m_instrumentInfoMap.find(insItr.second->underly);
                    if (itr == m_instrumentInfoMap.end()) {
                        Types::InstrumentInfo *underlyInsInfo = new Types::InstrumentInfo();
                        strcpy(underlyInsInfo->instrumentID.data(), insItr.second->underly.data());
                        underlyInsInfo->productIDClass = Types::ProductClass::future;
                        m_instrumentInfoMap[insItr.second->underly] = underlyInsInfo;
                        Cosmos::Types::SubScribeQuote subScribeUnderly;
                        memcpy(&subScribeUnderly.instrumentID, &insItr.second->underly, sizeof(Types::Instrument_t));
                        subScribeUnderly.policyID = m_policyID;
                        m_driver->subscribeQuote(subScribeUnderly);
                    }
                    Types::SubScribeQuote subScribeQuote;
                    memcpy(&subScribeQuote.instrumentID, &insItr.second->instrumentID, sizeof(Types::Instrument_t));
                    subScribeQuote.policyID = m_policyID;
                    //     m_instrumentInfoMap[insItr.second->instrumentID] = insItr.second;
                    m_driver->subscribeQuote(subScribeQuote);
                }
            }

            Types::SubscribeEngine subscribeEngine;
            subscribeEngine.policyid = m_policyID;
            m_driver->subscribePolicy(subscribeEngine, this);

            for (int i =0 ; i<6; i++) {
                TableInsertBuffer tableInsertBuffer;
                m_TableInsertBufferVec.emplace_back(tableInsertBuffer);
            }

            sprintf(m_TableInsertBufferVec[0].insertHead.data(), "%s",
                "REPLACE INTO futureDay_bars (instrument,tradingDay,period,updateTimeBegin,updateTimeEnd,productId,open,high,low,close,volume,amount,position,upperLimit,lowerLimit,settlement) "
                        "VALUES");
            initTableInsertBuffer(m_TableInsertBufferVec[0]);

            sprintf(m_TableInsertBufferVec[1].insertHead.data(), "%s",
                "INSERT INTO futureMinutes_bars (instrument,tradingDay,period,updateTimeBegin,updateTimeEnd,productId,open,high,low,close,volume,amount,position,bidPrice,askPrice,bidVolume,askVolume) "
                        "VALUES");
            initTableInsertBuffer(m_TableInsertBufferVec[1]);


            sprintf(m_TableInsertBufferVec[2].insertHead.data(), "%s",
                "INSERT INTO futureOneMinute_bars (instrument,tradingDay,period,updateTimeBegin,updateTimeEnd,productId,open,high,low,close,volume,amount,position,bidPrice,askPrice,bidVolume,askVolume) "
                        "VALUES");
            initTableInsertBuffer(m_TableInsertBufferVec[2]);


            sprintf(m_TableInsertBufferVec[3].insertHead.data(), "%s",
              "REPLACE INTO optionDay_bars (instrument,underly,optionType,strikePrice,tradingDay,expireDate,period,updateTimeBegin,updateTimeEnd,productId,"
              "open,high,low,close,forwardPrice,volume,amount,position,settlement,IV,delta,gamma,vega,theta) "
                      "VALUES");
            initTableInsertBuffer(m_TableInsertBufferVec[3]);


            sprintf(m_TableInsertBufferVec[4].insertHead.data(), "%s",
              "INSERT INTO optionMinutes_bars (instrument,underly,optionType,strikePrice,tradingDay,expireDate,period,updateTimeBegin,updateTimeEnd,productId,"
              "open,high,low,close,forwardPrice,volume,amount,position,bidPrice,askPrice,bidVolume,askVolume,IV,IVBid,IVAsk,delta,gamma,vega,theta) "
                      "VALUES");
            initTableInsertBuffer(m_TableInsertBufferVec[4]);


            sprintf(m_TableInsertBufferVec[5].insertHead.data(), "%s",
              "INSERT INTO optionOneMinute_bars (instrument,underly,optionType,strikePrice,tradingDay,expireDate,period,updateTimeBegin,updateTimeEnd,productId,"
              "open,high,low,close,forwardPrice,volume,amount,position,bidPrice,askPrice,bidVolume,askVolume,IV,IVBid,IVAsk,delta,gamma,vega,theta) "
                      "VALUES");
            initTableInsertBuffer(m_TableInsertBufferVec[5]);



            m_saveThread = std::thread([this]() {
                while (true) {
                    auto kbarsRows = m_sqlThreadQueue.frontPtr();

                    if (kbarsRows != nullptr) {
                        m_sqlThreadQueue.popFront();

                        addBatch(kbarsRows, &m_TableInsertBufferVec[static_cast<int>(kbarsRows->table)]);

                    }
                    auto time = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    for (auto& tableInsertBuffer : m_TableInsertBufferVec) {
                        if ( (tableInsertBuffer.numb >500 ) || (tableInsertBuffer.numb  >0 && time - tableInsertBuffer.lastWriteSeconds >30) ) {
                            insertSql(tableInsertBuffer.insertBuffer);
                            initTableInsertBuffer(tableInsertBuffer);
                        }
                    }

                }
            });
        }

        void KBarSaverEngine::initTableInsertBuffer(TableInsertBuffer & tableInsertBuffer) {
            auto time = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            tableInsertBuffer.insertBuffer.fill('\0');
            sprintf(tableInsertBuffer.insertBuffer.data(), "%s",tableInsertBuffer.insertHead.data());
            tableInsertBuffer.lastWriteSeconds = time;
            tableInsertBuffer.numb=0;
            tableInsertBuffer.insertBeginIdx = strlen(tableInsertBuffer.insertBuffer.data());
        }

       void  KBarSaverEngine::addBatch(const KBarsRows * kbarsRows, TableInsertBuffer * tableInsertBuffer) {
            int i=0;
            if (tableInsertBuffer->numb>0) {
                tableInsertBuffer->insertBuffer[i + tableInsertBuffer->insertBeginIdx] = ',';
                tableInsertBuffer->insertBeginIdx++;
            }
            for (; i < kbarsRows->sql.size(); ) {
                if (kbarsRows->sql[i] =='\0') {
                    break;
                }
                tableInsertBuffer->insertBuffer[i + tableInsertBuffer->insertBeginIdx] = kbarsRows->sql[i];
                i++;

            }
            tableInsertBuffer->insertBeginIdx += i;
            tableInsertBuffer->numb +=1;

        }

        void KBarSaverEngine::insertSql(  std::array<char,512*512> const& insertSql) {
            try {
                bool bRetried;
                int nTried = 0;
                int retCode = -1;
                //        fprintf(stderr, "saveKline custom : %s\n", sql->data());
                do {
                    bRetried = false;
                    retCode = m_mysql->execSQL(insertSql.data());
                    if (retCode < 0) {
                        const char *errorMsg = mysql_error(m_mysql->getMysql());
                        int errorNo = mysql_errno(m_mysql->getMysql());
                     //   fprintf(stderr,"insert/update bar into mysql faield, sql:%s, error_no:%d, error_msg:%s\n", insertSql.data(), errorNo, errorMsg);
                        fprintf(stderr,"insert/update bar into mysql faield, sql size:%d, error_no:%d, error_msg:%s\n", insertSql.size(), errorNo, errorMsg);
                        spdlog::error("mysql execise failed, sql size:{}, error_no:{}, error_msg:{}",
                                      insertSql.size(), errorNo, errorMsg);
                        if (errorNo == 2006 && !bRetried && m_mysql->reopen() == 0) {
                            bRetried = true;
                        }
                    }
                } while (nTried++ < 2 && retCode < 0 && bRetried);
            } catch (...) {
                //       fprintf(stderr,"exception: insert/update kbar into mysql faield, sql is: %s\n", sql.data());
                spdlog::error("exception: mysql execise failed, sql is: {}", insertSql.data());
            }
        }

        void KBarSaverEngine::onRtnSubScribeQuote(Types::OnSubScribeQuote const &onSubScribeQuote) {
            //  fprintf(stderr, "onRtnSubScribeQuote : instrument=%s\n", onSubScribeQuote.instrumentID.data());
            if (strcmp(onSubScribeQuote.instrumentID.data(), "CJ605C12400") == 0) {
                int a = 1;
            }
            if (m_kDataManager->m_allKLineSeries.find(onSubScribeQuote.instrumentID) ==
                m_kDataManager->m_allKLineSeries.end()) {
                auto itrInsInfo = m_instrumentInfoMap.find(onSubScribeQuote.instrumentID);
                if (itrInsInfo == m_instrumentInfoMap.end()) {
                    assert(false);
                }
                initKSeries(m_tradingDay, m_isDay, *(itrInsInfo->second));
            }
        };

        void KBarSaverEngine::initKSeries(int tradingday, bool isDay, Types::InstrumentInfo const &insInfo) {
          //  Utils::TradingHours::initInstrumentTradingHours(insInfo.instrumentID);

            std::vector<KData::KData *> hisKline;
            for (auto period: Types::m_kperoidVec) {
                m_kDataManager->initKSeries(insInfo, period, tradingday, m_riskFreeR,
                                            hisKline, isDay);
            }
        }


        void KBarSaverEngine::onInitParams(Types::InitParam const &param) {
            // fprintf(stderr, "[%s] OnSetParam, %s=%s\n", m_engineName.c_str(), param.name.c_str(), param.value.c_str());
            // if (strcmp(param.name.c_str(), "isUseUnderlyPrice") == 0) {
            //     m_isUseUnderlyPrice = atof(param.value.c_str());
            // } else if (strcmp(param.name.c_str(), "isUseUnderlyPrice") == 0) {
            //     //  m_ctpTradeConfig = param.value;
            //     m_isUseUnderlyPrice = atof(param.value.c_str());
            // }
        }

        void KBarSaverEngine::onEventData(Types::EventData const &eventData) {
            if (eventData.eventType == Types::EventType::marketEvent) {
                auto pMD = (const Types::MarketData *) eventData.point;
                // if (strcmp(pMD->instrumentID.data(), "lc2605") !=0 ) {
                //     return;
                // }
                fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, volume=%d, isInit=%d, epoch_time=%ld\n",
                        pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, pMD->isInit,
                        pMD->epoch_time);
                auto startTime = std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                for (auto period: Types::m_kperoidVec) {

                    auto updateSeries = m_kDataManager->KMAddTick(pMD, period);


                    if (updateSeries != nullptr) {
                        if (updateSeries->m_insInfo.productIDClass == Types::ProductClass::future) {
                               saveKline(updateSeries, period);
                            if (updateSeries->m_callPutSeriesMap != nullptr) {
                                for (auto &optionSeriesItr: *updateSeries->m_callPutSeriesMap) {
                                    saveKline(optionSeriesItr.second->callSeries, period);
                                    saveKline(optionSeriesItr.second->putSeries, period);
                                }
                            }
                        } else if (updateSeries->m_insInfo.productIDClass == Types::ProductClass::option) {
                            saveKline(updateSeries, period);
                        }
                    }
                }
                auto timeConsume =  std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now().time_since_epoch()).count() - startTime;
              //  fprintf(stderr,"saveEngine start consume : timeConsume=%d\n", timeConsume);
                // if (strcmp(pMD->instrumentID.data(),"IM2606") == 0 ) {
                // fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, volume=%d, epoch_time=%ld \n",
                //     pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, pMD->epoch_time);
                // }
            }
        };

        void KBarSaverEngine::saveKline(KData::KSeries *updateSeries, Types::KPeriod period) {
            while (updateSeries->m_recordIndex < updateSeries->m_seriesIndex) {
                _saveKline(updateSeries, updateSeries->m_recordIndex, period);
                updateSeries->m_recordIndex += 1;
            }
        }


        void KBarSaverEngine::_saveKline(KData::KSeries *series, int saveIndex, Types::KPeriod period) {
            auto kline = series->m_KDataVecs[saveIndex];
            if (kline->isInsert == true && period != Types::KPeriod::D1) {
                return;
            }

            if (strcmp(kline->m_instrument.data(), "") == 0) {
                return;
            }

            if (series->m_insInfo.productIDClass == Types::ProductClass::future) {
                if(period == Types::KPeriod::D1) {
                    auto kbarsRows = m_kbarRowsPool.getNewMemory();
                    sprintf(kbarsRows->sql.data(),"(\'%s\',%d,%d,\'%s\',\'%s\',\'%s\',%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f)",
                        kline->m_instrument.data(), kline->m_tradingday,
                        Cosmos::Types::KPeroidToIntervalVec[static_cast<int>(period)],
                        kline->m_updateTimeBegin.data(),
                        kline->m_updateTimeEnd.data(),
                        kline->m_productID.data(), kline->m_open,
                        kline->m_high, kline->m_low,
                        kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                        kline->m_upperLimit, kline->m_lowerLimit, kline->m_settlement);
                    kbarsRows->table = KBars_T::futureDay;
                    m_sqlThreadQueue.write(*kbarsRows);
                    kline->isInsert = true;
                }else  {
                    auto kbarsRows = m_kbarRowsPool.getNewMemory();
                    sprintf(kbarsRows->sql.data(),"(\'%s\',\'%d\',%d,\'%s\',\'%s\',\'%s\',%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d)",
                    kline->m_instrument.data(), kline->m_tradingday,
                         Cosmos::Types::KPeroidToIntervalVec[static_cast<int>(period)],
                         kline->m_updateTimeBegin.data(),
                         kline->m_updateTimeEnd.data(),
                         kline->m_productID.data(), kline->m_open,
                         kline->m_high, kline->m_low,
                         kline->m_close, (double) kline->m_volume, kline->m_amount, kline->m_oi,
                         kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume);
                    kbarsRows->table =  period == Types::KPeriod::Min1 ? KBars_T::futureOneMinute : KBars_T::futureMinutes;
                    m_sqlThreadQueue.write(*kbarsRows);
                    kline->isInsert = true;
                }

            }else if (series->m_insInfo.productIDClass == Types::ProductClass::option) {
                if(period == Types::KPeriod::D1) {
                    auto kbarsRows = m_kbarRowsPool.getNewMemory();
                    sprintf(kbarsRows->sql.data(),"(\'%s\',\'%s\',\'%c\',%.1f,%d,%d,%d,\'%s\',\'%s\',\'%s\',%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.5f,%.5f,%.5f,%.5f,%.5f)",
                    kline->m_instrument.data(), series->m_insInfo.underly.data(), series->m_insInfo.optionType, series->m_insInfo.strikePrice,
                         kline->m_tradingday,series->m_insInfo.expireDate,Types::KPeroidToIntervalVec[static_cast<int>(period)],
                         kline->m_updateTimeBegin.data(),   kline->m_updateTimeEnd.data(), kline->m_productID.data(),
                         kline->m_open, kline->m_high, kline->m_low,
                         kline->m_close, kline->m_forwardPrice,  (double) kline->m_volume, kline->m_amount, kline->m_oi,
                          kline->m_settlement, kline->IV,
                          kline->delta, kline->gamma, kline->vega, kline->theta);
                    kbarsRows->table = KBars_T::optionDay;
                    m_sqlThreadQueue.write(*kbarsRows);
                    kline->isInsert = true;
                }else  {
                    auto kbarsRows = m_kbarRowsPool.getNewMemory();
                    sprintf(kbarsRows->sql.data(),"(\'%s\',\'%s\',\'%c\',%.1f,%d,%d,%d,\'%s\',\'%s\',\'%s\',%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.3f,%.3f,%d,%d,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f)",
                    kline->m_instrument.data(),series->m_insInfo.underly.data(), series->m_insInfo.optionType, series->m_insInfo.strikePrice, kline->m_tradingday,
                                  series->m_insInfo.expireDate,
                                  Types::KPeroidToIntervalVec[static_cast<int>(period)],
                                       kline->m_updateTimeBegin.data(), kline->m_updateTimeEnd.data(), kline->m_productID.data(),
                                  kline->m_open, kline->m_high, kline->m_low,
                                  kline->m_close,  kline->m_forwardPrice,(double) kline->m_volume, kline->m_amount, kline->m_oi,
                                  kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume,
                                  kline->IV, kline->bidIV,  kline->askIV,  kline->delta, kline->gamma, kline->vega, kline->theta);
                    kbarsRows->table =  period == Types::KPeriod::Min1 ? KBars_T::optionOneMinute : KBars_T::optionMinutes;
                  //  if (kbarsRows->table == KBars_T::optionOneMinute) {
                        m_sqlThreadQueue.write(*kbarsRows);
                        kline->isInsert = true;
                  //  }

                }
            }
        }
    }
}
