//
// Created by zhangyw on 4/17/20.
//

#include "../Utils/Utils.h"
#include "KBarSaverEngine.h"

#include <stdlib.h>


namespace Cosmos {
    namespace KBarEngine {
        void KBarSaverEngine::onStart() {

            m_kDataManager = new KData::KDataManager(m_tradingDay, m_isDay, m_isUseUnderlyPrice);

            for(auto& insItr :  m_instrumentInfoMap){
                  if (insItr.second.productIDClass == Types::ProductClass::future) {
                      Types::SubScribeQuote subScribeQuote;
                      memcpy(&subScribeQuote.instrumentID, &insItr.second.instrumentID, sizeof(Types::Instrument_t));
                      subScribeQuote.policyID = m_policyID;
                     // m_instrumentInfoMap[insItr.second.instrumentID] = ins;
                      m_driver->subscribeQuote(subScribeQuote);
                  }
            }
            for(auto& insItr :  m_instrumentInfoMap){
                if (insItr.second.productIDClass == Types::ProductClass::option) {
                    auto itr = m_instrumentInfoMap.find(insItr.second.underly);
                    if (itr == m_instrumentInfoMap.end()) {
                        Types::InstrumentInfo underlyInsInfo;
                        strcpy(underlyInsInfo.instrumentID.data(), insItr.second.underly.data());
                        underlyInsInfo.productIDClass =  Types::ProductClass::future;
                        m_instrumentInfoMap[insItr.second.underly] = underlyInsInfo;
                        Cosmos::Types::SubScribeQuote subScribeUnderly;
                        memcpy(&subScribeUnderly.instrumentID, &insItr.second.underly, sizeof(Types::Instrument_t));
                        subScribeUnderly.policyID = m_policyID;
                        m_driver->subscribeQuote(subScribeUnderly);
                    }
                    Types::SubScribeQuote subScribeQuote;
                    memcpy(&subScribeQuote.instrumentID, &insItr.second.instrumentID, sizeof(Types::Instrument_t));
                    subScribeQuote.policyID = m_policyID;
               //     m_instrumentInfoMap[insItr.second.instrumentID] = insItr.second;
                    m_driver->subscribeQuote(subScribeQuote);
                }
            }

            Types::SubscribeEngine subscribeEngine;
            subscribeEngine.policyid = m_policyID;
            m_driver->subscribePolicy(subscribeEngine, this);

            m_saveThread = std::thread([this](){
                while (true) {
                    auto sql = m_squalQueue.frontPtr();

                    if (sql != nullptr)
                    {
                        m_squalQueue.popFront();
                        try
                        {
                            bool bRetried;
                            int nTried = 0;
                            int retCode = -1;
                    //        fprintf(stderr, "saveKline custom : %s\n", sql->data());
                            do
                            {
                                bRetried = false;
                                retCode = m_mysql->execSQL(sql->data());
                                if (retCode <0)
                                {
                                    const char *errorMsg = mysql_error(m_mysql->getMysql());
                                    int errorNo = mysql_errno(m_mysql->getMysql());
                         //           fprintf(stderr,"insert/update bar into mysql faield, sql:%s, error_no:%d, error_msg:%s\n", sql->data(), errorNo, errorMsg);
                                    spdlog::error("mysql execise failed, sql:{}, error_no:{}, error_msg:{}", sql->data(), errorNo, errorMsg);
                                    if (errorNo == 2006 && !bRetried && m_mysql->reopen() == 0)
                                    {
                                        bRetried = true;
                                    }
                                }
                            } while (nTried++ < 2 && retCode < 0 && bRetried);
                        }
                        catch (...)
                        {
                            //       fprintf(stderr,"exception: insert/update kbar into mysql faield, sql is: %s\n", sql.data());
                            spdlog::error("exception: mysql execise failed, sql is: {}", sql->data());
                        }

                    }
                }
            });
        }


        void KBarSaverEngine::onRtnSubScribeQuote( Types::OnSubScribeQuote const &onSubScribeQuote) {
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

        void KBarSaverEngine::initKSeries(int tradingday, bool isDay, Types::InstrumentInfo const &insInfo) {
            Utils::TradingHours::initInstrumentTradingHours(insInfo.instrumentID);

            std::vector< KData::KData *> hisKline;
            for (auto period: Types::m_kperoidVec) {
                m_kDataManager->initKSeries(insInfo, period, tradingday, m_riskFreeR,
                                           hisKline, isDay);
            }
        }




        void KBarSaverEngine::onParams( Types::Param const &param) {
            fprintf(stderr, "[%s] OnSetParam, %s=%s\n", m_engineName.c_str(), param.name.c_str(), param.value.c_str());
            if(strcmp(param.name.c_str(), "isUseUnderlyPrice") ==0){

                m_isUseUnderlyPrice = atof(param.value.c_str());
            }else if(strcmp(param.name.c_str(), "isUseUnderlyPrice") ==0){
                //  m_ctpTradeConfig = param.value;
                m_isUseUnderlyPrice = atof(param.value.c_str());
            }
        }
        void KBarSaverEngine::onEventData(Types::EventData const &eventData) {
            if (eventData.type == 0) {
                auto pMD = (const Types::MarketData *) eventData.point;
                // if (pMD->instrumentID[4]!='5' ) {
                //     return;
                // }
                // fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, volume=%d, epoch_time=%ld \n",
                //     pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, pMD->epoch_time);
                for (auto period : Types::m_kperoidVec) {
                   auto updateSeries = m_kDataManager->KMAddTick(pMD, period);
                   if (updateSeries != nullptr) {
                       if (updateSeries->m_insInfo.productIDClass == Types::ProductClass::future) {
                           saveKline( updateSeries,  period);
                           for (auto &optionSeriesItr: *updateSeries->m_callPutSeriesMap) {
                               saveKline( optionSeriesItr.second->callSeries,  period);
                               saveKline( optionSeriesItr.second->putSeries,  period);
                           }

                       } else if (updateSeries->m_insInfo.productIDClass == Types::ProductClass::option) {
                           saveKline( updateSeries,  period);
                       }
                   }
                }

                // if (strcmp(pMD->instrumentID.data(),"IM2606") == 0 ) {
                // fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, volume=%d, epoch_time=%ld \n",
                //     pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, pMD->epoch_time);
                // }
            }
        };

        void KBarSaverEngine::saveKline( KData::KSeries * updateSeries, Types::KPeriod period) {
            while(updateSeries->m_recordIndex < updateSeries->m_seriesIndex ){
                               _saveKline(updateSeries, updateSeries->m_recordIndex, period);
                               updateSeries->m_recordIndex +=1;
                           }
        }


        void KBarSaverEngine::_saveKline( KData::KSeries * series, int saveIndex,   Types::KPeriod period){
            auto kline = series->m_KDataVecs[saveIndex];
            if(kline->isInsert == true && period !=  Types::KPeriod::D1){
                return;
            }

            if(strcmp(kline->m_instrument.data(),"")==0 ){
                return;
            }
//            fprintf(stderr,"pmdUpdateTime=%s,%d,%s,%s,%d,%.1f,%.1f,%.1f,%.1f,%d\n",  pMD->updateTime.data(),
//                    kline->m_tradingday, kline->m_instrument.data(), kline->m_updateTime.data(),
//                     Types::KPeroidToSecondsMap[period]/60, kline->m_open,
//                    kline->m_high, kline->m_low, kline->m_close, kline->m_volume  );
            if(period ==  Types::KPeriod::D1){
                auto sql = m_sqlPool.getNewMemory();
                sprintf(sql->data(), "delete from %s where instrument ='%s' and tradingDay = %d ",
                         Types::KPeroidToOptionTableMap[period].data(), kline->m_instrument.data(), kline->m_tradingday);
                //        fprintf(stderr,"%s\n",sql.data());
          //      fprintf(stderr, "delete day Kline : %s\n", sql->data());
                m_squalQueue.write(*sql);
                sql = m_sqlPool.getNewMemory();
                sprintf(sql->data(), "insert into %s (instrument,tradingDay,updateTime,productId,period,open,high,low,close,volume,amount,position,upperLimit,lowerLimit,settlement) "
                                     " values ('%s',%d,'%s','%s',%d,%f,%f,%f,%f,%d,%f,%f,%f,%f,%f)",
                         Types::KPeroidToOptionTableMap[period].data(), kline->m_instrument.data(), kline->m_tradingday,
                        kline->m_updateTimeBegin.data(), kline->m_productID.data(),
                         Types::KPeroidToIntervalMap[period], kline->m_open,
                        kline->m_high, kline->m_low,kline->m_close, kline->m_volume, kline->m_amount, kline->m_oi,
                        kline->m_upperLimit, kline->m_lowerLimit, kline->m_settlement);
                //        fprintf(stderr,"%s\n",sql.data());
                //     fprintf(stderr, "saveKline produce : %s\n", sql->data());
                m_squalQueue.write(*sql);
                kline->isInsert = true;
            }else{
                auto sql = m_sqlPool.getNewMemory();
             //   fprintf(stderr,"instrument=%s, period =%d\n", kline->m_instrument.data(),  Types::KPeroidToIntervalMap[period]);
                sprintf(sql->data(), "insert into %s (instrument,tradingDay,updateTime,productId,period,open,high,low,close,volume,amount,position,bidPrice,askPrice,askVolume,bidVolume)\n "
                                     " values (\'%s\',%d,\'%s\',\'%s\',%d,%f,%f\n,%f,%f,%d,%f,%f,%f,%f,%d,%d)",
                         Types::KPeroidToOptionTableMap[period].data(), kline->m_instrument.data(), kline->m_tradingday,
                        kline->m_updateTimeBegin.data(), kline->m_productID.data(), Types::KPeroidToIntervalMap[period], kline->m_open,
                        kline->m_high, kline->m_low, kline->m_close, kline->m_volume, kline->m_amount, kline->m_oi,
                        kline->m_bidPrice, kline->m_askPrice, kline->m_bidVolume, kline->m_askVolume);
                //        fprintf(stderr,"%s\n",sql->data());
                //     fprintf(stderr, "saveKline produce : %s\n", sql->data());
                m_squalQueue.write(*sql);
                kline->isInsert = true;
            }
        }
    }
}
