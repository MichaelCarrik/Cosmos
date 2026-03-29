//
// Created by Zhangyingwei on 2023/5/10.
//

#include "UnderlyEngine.h"
#include "../Types/QuerySymbol.h"
// //#include "../Policy/SellStrangle.h"
// #include "../Policy/BuyStrangle.h"
// #include "../Policy/LongGammaSAR.h"
// #include "../Policy/LongGammaGod.h"
#include "../Policy/OptionPolicy/LongGammaVulture.h"
// #include "../Policy/Calendar.h"
// #include "../Policy/SellStrangleV2.h"

#include "../Policy/FuturePolicy/VultureTrend.h"


namespace Cosmos {
    namespace Engine {
        void UnderlyEngine::onStart() {
            if (m_isDay == false && (( m_engineParam.productID[0] == 'I' && m_engineParam.productID[1] == 'F')
                                     || (m_engineParam.productID[0] == 'I' && m_engineParam.productID[1]== 'H')
                                     || (m_engineParam.productID[0] == 'I' && m_engineParam.productID[1]== 'M'))) {
                return;
                                     }
            m_kDataManager = new KData::KDataManager(m_tradingDay, m_isDay, m_engineParam.isUseUnderlyPrice, m_mySql);
            setInitLog();

            for (auto &itr: m_underlyInitMap) {
                Types::UnderlyInfo queryUnderInfo;
                strcpy(queryUnderInfo.instrumentID.data(), itr.first.data());
                queryUnderInfo.isWithOption = itr.second;
                m_driver->send(queryUnderInfo);    // get InstrumentInfo
            }

            for (auto & subPolicyParams : *m_subPolicyParamsVec) {
                auto policyName = Utils::getParamMapValue(subPolicyParams, "policyName");
                auto policyType = Utils::getParamMapValue(subPolicyParams, "policyType");

                if (strcmp(policyType.c_str(), "future") ==0) {
                    auto policy= this->createFuturePolicy(policyName, subPolicyParams);
                    m_futurePolicyVec.emplace_back(policy);
                }
                if (strcmp(policyType.c_str(), "option") ==0 ) {
                    auto policy= this->createOptionPolicy(policyName, subPolicyParams);
                    m_optionPolicyVec.emplace_back(policy);
                }

            }

            for (auto &itr: m_underlyInitMap) { //  get underly first , prepared for option
                _querySymbol(itr.first);
                _queryQuote(itr.first);
            }

            for (auto &ins: m_symbolMap) {
                ins.second->m_positionLog = m_positionLog;
                ins.second->order = m_orderList.getNewMemory();
                _querySymbol(ins.first);
                _queryQuote(ins.first);
            }

            Types::SubscribeEngine subscribeEngine;
            subscribeEngine.policyid = m_policyID;
            m_driver->subscribePolicy(subscribeEngine, this);

            for (auto futurePolicy: m_futurePolicyVec) {
                futurePolicy->start(m_symbolMap);
            }

            for (auto optionPolicy: m_optionPolicyVec) {
                optionPolicy->start(m_symbolMap);
            }
        };


        void UnderlyEngine::_queryQuote(Types::Instrument_t const &queryInstrumentID) {
        //    fprintf(stderr, "[%s] QueryQuote, queryInstrumentID=%s\n", m_engineName.c_str(), queryInstrumentID.data());
            Types::SubScribeQuote SubScribeQuote;
            strcpy(SubScribeQuote.instrumentID.data(), queryInstrumentID.data());
            SubScribeQuote.policyID = m_policyID;
            m_driver->subscribeQuote(SubScribeQuote);
        }

        void UnderlyEngine::_querySymbol(Types::Instrument_t const &queryInstrumentID) {
            Types::QuerySymbol queryUnderlySymbol;
            queryUnderlySymbol.id = m_policyID;
            strcpy(queryUnderlySymbol.instrumentID.data(), queryInstrumentID.data());
            m_driver->send(queryUnderlySymbol);
        }

        void UnderlyEngine::_initUnderly(Types::Instrument_t const &underlyID, std::string const& policyType) {

            bool isWithOption = false;
            if (strcmp(policyType.c_str(), "option") ==0 ) {
                isWithOption = true;
            }
            m_underlyInitMap[underlyID] = isWithOption;
        }


        void UnderlyEngine::onInitParams(Types::InitParam const & initParamMap) {
            if (strcmp(initParamMap.engineName.c_str(), m_engineName.c_str()) != 0) {
                assert(false);
            }

            m_engineParam.affiThreadId = stoi(Utils::getParamMapValue(initParamMap.paramMap, "affiThreadId"));
            strcpy(m_engineParam.productID.data(), Utils::getParamMapValue(initParamMap.paramMap, "productId").c_str());
            m_engineParam.putResendTimeout = stoi(Utils::getParamMapValue(initParamMap.paramMap, "putResendTimeout"));
            m_engineParam.futurePreCloseToday = stoi(Utils::getParamMapValue(initParamMap.paramMap, "futurePreCloseToday"));
            m_engineParam.futureMinVolume = stoi(Utils::getParamMapValue(initParamMap.paramMap, "futureMinVolume"));
            m_engineParam.optionMinVolume = stoi(Utils::getParamMapValue(initParamMap.paramMap, "optionMinVolume"));
            m_engineParam.futureMaxPosition = stoi(Utils::getParamMapValue(initParamMap.paramMap, "futureMaxPosition"));
            m_engineParam.optionMaxPosition = stoi(Utils::getParamMapValue(initParamMap.paramMap, "optionMaxPosition"));

            m_engineParam.hedgeType = Types::HedgeType::spec;
            int hedgeType =stoi(Utils::getParamMapValue(initParamMap.paramMap, "hedgeType"));
            if (hedgeType == 1) {
                m_engineParam.hedgeType = Types::HedgeType::hedge;
            }

            for (auto & subPolicyParamMap : initParamMap.subPolicyParamsVec) {
                auto policyType = Utils::getParamMapValue(subPolicyParamMap, "policyType");
                auto underlyAStr = Utils::getParamMapValue(subPolicyParamMap, "underlyA");
                auto underlyBStr = Utils::getParamMapValue(subPolicyParamMap, "underlyB");

                if (strcmp(underlyAStr.c_str(), "") !=0 ) {
                    Types::Instrument_t tempInstrument{""};
                    strcpy(tempInstrument.data(), underlyAStr.c_str());
                    _initUnderly(tempInstrument, policyType);
                }

                if (strcmp(underlyBStr.c_str(), "") !=0 ) {
                    Types::Instrument_t tempInstrument{""};
                    strcpy(tempInstrument.data(), underlyBStr.c_str());
                    _initUnderly(tempInstrument, policyType);
                }
            }
            m_subPolicyParamsVec = &initParamMap.subPolicyParamsVec;
        };


        Policy::IFuturePolicy * UnderlyEngine::createFuturePolicy(std::string const &policyName ,std::map<std::string, std::string> const &paramMap) {
            fprintf(stderr, "[%s] createFuturePolicy policyName = %s\n", m_engineName.c_str(),  policyName.c_str());
            if (policyName.compare("VultureTrend") == 0) {
                Types::Instrument_t underlyInstrument{""};
                strcpy(underlyInstrument.data(), Utils::getParamMapValue(paramMap, "underlyA").c_str());

                auto kpstr = Utils::getParamMapValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                this->setKPtoHisSeriesMap(underlyInstrument, kPeriod, true);
                this->setKPtoHisSeriesMap(underlyInstrument, Types::KPeriod::D1, true);

                std::string siStr = Utils::getParamMapValue(paramMap, "SI");
                Types::SignalIntension si = Types::configParamToSIMap.at(siStr);
                double MV = std::stof(Utils::getParamMapValue(paramMap, "MV").c_str());
                auto adjRiskTimeStr = Utils::getParamMapValue(paramMap, "adjRiskTime");
                auto adjRiskTime = Utils::ToPsSeconds(adjRiskTimeStr, true);
                double alpha = 0.7;
                double mark = 2.0;
              //  double openAtDelta = std::stof(Utils::getParamMapValue(paramMap, "adjRiskTime").c_str());

                Types::InstrumentInfo * futureInsInfo{nullptr};
                getFutureInfoByInstrumentID(underlyInstrument, futureInsInfo);
                return new Policy::VultureTrend(policyName, m_engineName, underlyInstrument, kPeriod,
                                                                           MV, futureInsInfo->multi, si,
                                                                           m_tradingDay, adjRiskTime, alpha, mark);
            }

            assert(false);
            return nullptr;
        }

        Policy::IOptionPolicy *
        UnderlyEngine::createOptionPolicy(std::string const &policyName ,std::map<std::string, std::string> const &paramMap){
            fprintf(stderr, "[%s] createOptionPolicy policyName = %s\n", m_engineName.c_str(),  policyName.c_str());

            //  if (policyName.compare("SellStrangleV2") == 0) {
            //     auto kpstr = Types::Param::getValue(paramMap, "period");
            //     Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
            //     int isRefreshDelta = std::stoi(Types::Param::getValue(paramMap, "isRefreshDelta"));
            //     double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
            //     double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
            //     double baseRation = std::stof(Types::Param::getValue(paramMap, "biasRation").c_str());
            //     if(m_policyKPMap.find(kPeriod) == m_policyKPMap.end()){
            //         m_policyKPMap[kPeriod] = false;
            //     }
            //     return new  Policy::SellStrangleV2(kPeriod, MV * 10000, policyName,
            //                                                  m_engineName, m_underlyInsMap.nearInstrument,  m_multi,
            //                                                  m_tradingDay, m_maxPosition, isRefreshDelta, openAtDelta,baseRation);
            // }else if (policyName.compare("BuyStrangle") == 0) {
            //     auto kpstr = Types::Param::getValue(paramMap, "period");
            //     Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
            //     std::string tt_str = Types::Param::getValue(paramMap, "tradeTime");
            //     int tradeTime = Utils::ToPsSeconds(tt_str);
            //     int isRefreshDelta = std::stoi(Types::Param::getValue(paramMap, "isRefreshDelta"));
            //     double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
            //     double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
            //     double adjCoef = std::stof(Types::Param::getValue(paramMap, "adjCoef").c_str());
            //     if(m_policyKPMap.find(kPeriod) == m_policyKPMap.end()){
            //         m_policyKPMap[kPeriod] = false;
            //     }
            //     return new  Policy::BuyStrangle(kPeriod, MV * 10000, policyName,
            //                                                  m_engineName, m_underlyInsMap.nearInstrument, tradeTime, m_multi,
            //                                                  m_tradingDay, m_maxPosition, isRefreshDelta, openAtDelta,adjCoef);
            // }else if (policyName.compare("Calendar") == 0) {
            //     auto kpstr = Types::Param::getValue(paramMap, "period");
            //     Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
            //     std::string tt_str = Types::Param::getValue(paramMap, "tradeTime");
            //     int tradeTime = Utils::ToPsSeconds(tt_str);
            //     int isRefreshDelta = std::stoi(Types::Param::getValue(paramMap, "isRefreshDelta"));
            //     double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
            //     double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
            //     double adjCoef = std::stof(Types::Param::getValue(paramMap, "adjCoef").c_str());
            //     std::string farUnderlyStr = Types::Param::getValue(paramMap, "farUnderly");
            //     _initUnderly(farUnderlyStr);
            //     strcpy(m_underlyInsMap.farInstrument.data(), farUnderlyStr.c_str());
            //     if(m_policyKPMap.find(kPeriod) == m_policyKPMap.end()){
            //         m_policyKPMap[kPeriod] = false;
            //     }
            //     return new  Policy::Calendar(kPeriod, MV * 10000, policyName,
            //                                                 m_engineName, m_underlyInsMap.nearInstrument, m_underlyInsMap.farInstrument,
            //                                                 tradeTime, m_multi, m_tradingDay, m_maxPosition, isRefreshDelta, openAtDelta,
            //                                                 adjCoef);
            // }else if (policyName.compare("LongGammaSAR") == 0) {
            //     auto kpstr = Types::Param::getValue(paramMap, "period");
            //     Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
            //     m_policyKPMap[kPeriod] = true;
            //     double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
            //     double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
            //     return new  Policy::LongGammaSAR(kPeriod, MV * 10000, policyName,
            //                                                 m_engineName, m_underlyInsMap.nearInstrument,  m_multi,m_tradingDay, m_maxPosition,
            //                                                 openAtDelta);
            // }else
            if (policyName.compare("LongGammaVulture") == 0) {

                Types::Instrument_t underlyInstrument{""};
                strcpy(underlyInstrument.data(), Utils::getParamMapValue(paramMap, "underlyA").c_str());

                auto kpstr = Utils::getParamMapValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                this->setKPtoHisSeriesMap(underlyInstrument, kPeriod, true);
                this->setKPtoHisSeriesMap(underlyInstrument, Types::KPeriod::D1, true);

                std::string siStr = Utils::getParamMapValue(paramMap, "SI");
                Types::SignalIntension si = Types::configParamToSIMap.at(siStr);
                double MV = std::stof(Utils::getParamMapValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Utils::getParamMapValue(paramMap, "openAtDelta").c_str());

                Types::InstrumentInfo * optionInsInfo{nullptr};
                getOptionInfoByUnderly(underlyInstrument, optionInsInfo);

             //  auto underlyToTodayIndex =  m_kDataManager->m_updateGreeks->getUnderlyTodayBeginIndex(underlyInstrument, kPeriod);

                return new Policy::LongGammaVulture( policyName, m_engineName,
                          underlyInstrument, kPeriod, MV,  optionInsInfo->multi,
                          si,  m_tradingDay,optionInsInfo->expireDate,  m_engineParam.optionMaxPosition, openAtDelta,
                          [this](Types::Instrument_t const& instrument, Types::KPeriod period)->int {
                             return  this->m_kDataManager->m_updateGreeks->getUnderlyTodayBeginIndex(instrument, period);
                          });
            } //else if (policyName.compare("LongGammaGod") == 0) {
            //     auto kpstr = Types::Param::getValue(paramMap, "period");
            //     Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
            //     m_policyKPMap[kPeriod] = false;
            //     double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
            //     double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
            //     int godDirection = std::stoi(Types::Param::getValue(paramMap, "godDirection").c_str());
            //     double closeExceedThresh = std::stof(Types::Param::getValue(paramMap, "closeExceedThresh").c_str());
            //     return new  Policy::LongGammaGod(kPeriod, MV * 10000, policyName,
            //                                                 m_engineName, m_underlyInsMap.nearInstrument,  m_multi,m_tradingDay, m_maxPosition,
            //                                                 openAtDelta, godDirection, closeExceedThresh);
            // }
            assert(false);
            return nullptr;
        }

        void UnderlyEngine::setKPtoHisSeriesMap(Types::Instrument_t const& instrument, Types::KPeriod period, bool isHis) {
            auto itr = m_KPtoHisSeriesMap.find(instrument);
            if (itr == m_KPtoHisSeriesMap.end()) {
                std::map<Types::KPeriod ,bool> temp;
                m_KPtoHisSeriesMap[instrument] = temp;
                itr =  m_KPtoHisSeriesMap.find(instrument);
            }

            auto itrIns = itr->second.find(period);
            if (itrIns == itr->second.end()) {
                m_KPtoHisSeriesMap[instrument][period] = isHis;
            }else if (isHis == true) {
                m_KPtoHisSeriesMap[instrument][period] = isHis;
            }


        }

        void UnderlyEngine::onRtnSubScribeQuote(Types::OnSubScribeQuote const &onSubScribeQuote) {
         //   fprintf(stderr,"onRtnSubScribeQuote : instrument=%s\n", onSubScribeQuote.instrumentID.data());
            if (onSubScribeQuote.policyID == m_policyID) {
                auto symbolItr = m_symbolMap.find(onSubScribeQuote.instrumentID);
                if (symbolItr == m_symbolMap.end()) {
                    assert(false && "no itrinsinfo in m_instrumentInfoMap");
                }
                if (m_kDataManager->m_allKLineSeries.find(onSubScribeQuote.instrumentID) ==m_kDataManager->m_allKLineSeries.end()) {\
                    Types::Instrument_t checkKPtoHisInstr =  onSubScribeQuote.instrumentID;
                    if (symbolItr->second->instrumentInfo.productIDClass == Types::ProductClass::option) {
                        checkKPtoHisInstr = symbolItr->second->instrumentInfo.underly;
                    }
                    auto kpInsItr = m_KPtoHisSeriesMap.find(checkKPtoHisInstr);
                    if (kpInsItr != m_KPtoHisSeriesMap.end()) {
                        for (auto & kpItr : kpInsItr->second) {
                            initKSeries(m_tradingDay, kpItr.first, m_isDay, symbolItr->second->instrumentInfo,
                                kpItr.second, m_isReal);
                            symbolItr->second->m_kSeriesMap[kpItr.first] =
                                m_kDataManager->getSeries( symbolItr->second->instrumentInfo.instrumentID,kpItr.first);
                        }
                    }
                }
                //    fprintf(stderr, "onRtnSubScribeQuote : %s, %x\n",symbolItr->second->instrumentInfo.instrumentID.data(), symbolItr);
            }
        };

        void UnderlyEngine::onInstrumentInfo(Types::InstrumentInfo const &instrumentInfo) {

            if (instrumentInfo.productIDClass == Types::ProductClass::future) {
                auto itr = m_underlyInitMap.find(instrumentInfo.instrumentID);
                if (itr != m_underlyInitMap.end()) {
                    auto optionSymbol = new Types::Symbol();
                    optionSymbol->policyID = m_policyID;
                    memcpy(&optionSymbol->instrumentInfo, &instrumentInfo, sizeof(Types::InstrumentInfo));
                    m_symbolMap[optionSymbol->instrumentInfo.instrumentID] = optionSymbol;
                }
            }else if (instrumentInfo.productIDClass == Types::ProductClass::option) {
                auto itr = m_underlyInitMap.find(instrumentInfo.underly);
                if (itr != m_underlyInitMap.end() && itr->second == true) {
                    auto optionSymbol = new Types::Symbol();
                    optionSymbol->policyID = m_policyID;
                    memcpy(&optionSymbol->instrumentInfo, &instrumentInfo, sizeof(Types::InstrumentInfo));
                    m_symbolMap[optionSymbol->instrumentInfo.instrumentID] = optionSymbol;
                }
            }


        };

        void UnderlyEngine::onRtnQuerySymbolPosition(Types::OnQuerySymbol const &onQuerySymbol) {
            if (onQuerySymbol.id != m_policyID) {
                return;
            }
            auto itr = m_symbolMap.find(onQuerySymbol.instrumentID);
            if (itr == m_symbolMap.end()) {
                assert(false);
            }
            memcpy(&itr->second->tradePosition, &onQuerySymbol.tradePosition, sizeof(onQuerySymbol.tradePosition));
            if (itr->second->tradePosition.filledPosition != 0) {
                Types::UpdateTime_t updateTime{""};
                itr->second->posWrite(false, m_tradingDay, updateTime, 0, 0);
            }
        }

        void UnderlyEngine::onEventData(Types::EventData const &eventData) {
            if (eventData.type == 0) {
                auto pMD = (const Types::MarketData *) eventData.point;
                if (strcmp(pMD->instrumentID.data(), "i2405") ==0 ) {
                    // fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, volume=%d, isInit=%d, epoch_time=%ld\n",
                    //  pMD->instrumentID.data(), pMD->updateTime.data(), pMD->milliSeconds, pMD->volume, pMD->isInit,
                    //  pMD->epoch_time);
                    if (strcmp(pMD->updateTime.data(), "22:59:50") == 0) {
                        int a =1;
                    }
                }



                if (strcmp(pMD->updateTime.data(), "09:04:50") == 0) {
                    int a =1;
                }
                m_kDataManager->KMAddTick(pMD);
                if (pMD->isInit == 0) {
                    this->runEvent(pMD, pMD->epoch_time);
                }
            } else if (eventData.type == 1) {
                auto inputOrder = (const Types::OrderField *) eventData.point;
                fprintf(stderr, "onEventData instrument=%s, pOrderID=%d, requestID=%d, orderRef=%s, orderStatus=%s\n",
                        inputOrder->instrumentID.data(),
                        inputOrder->pOrderID, inputOrder->tOrderID, inputOrder->orderRef.data(),
                        Types::orderStatusMap[inputOrder->orderStatus].data());
                auto itr = m_symbolMap.find(inputOrder->instrumentID);
                if (itr == m_symbolMap.end()) {
                    assert(false && "inputOrder not in m_symbolMap");
                }
                auto symbol = itr->second;
                updateMMOrder(inputOrder, symbol);
                Utils::logOrder(inputOrder, m_orderLog, symbol, inputOrder->orderStatus,
                                m_tradingDay, inputOrder->epoch_time);
            }
        };

        void UnderlyEngine::updateMMOrder(const Types::OrderField *inputOrder,
                                                Types::Symbol *symbol) {
            if (inputOrder->policyID != m_policyID) {
                assert(false && "OpenEngine::updateOrder policyID not match");
            }
            auto order = m_orderList.getMemoryById(inputOrder->pOrderID);
            Utils::updateOrder(order, inputOrder);

            if (inputOrder->orderStatus == Types::OrderStatus::failed) {
            } else if (order->orderStatus == Types::OrderStatus::partTraded ||
                       order->orderStatus == Types::OrderStatus::allTraded) {
                symbol->tradePosition.update_filled_position(order->orderSide, order->pet,
                                                             order->orderPrice, order->lastFilledVolume);
                symbol->symbolLastFilledPrice = order->lastFilledPrice;
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;
                symbol->posWrite(true, m_tradingDay, symbol->lastMD->updateTime,
                                 symbol->lastMD->milliSeconds, order->tOrderID);
            }

            if (order->isTerminal == true) {
                // auto itr = m_underlyInsMap.underlyInsMap.find(symbol->instrumentInfo.underly);
                // if (itr == m_underlyInsMap.underlyInsMap.end()) {
                //     assert(false);
                // }
                // if (symbol->instrumentInfo.optionType == 'C' && order->orderSide == Types::OrderSide::buy) {
                //     itr->second.callBuy = false;
                // } else if (symbol->instrumentInfo.optionType == 'C' && order->orderSide == Types::OrderSide::sell) {
                //     itr->second.callSell = false;
                // } else if (symbol->instrumentInfo.optionType == 'P' && order->orderSide == Types::OrderSide::buy) {
                //     itr->second.putBuy = false;
                // } else if (symbol->instrumentInfo.optionType == 'P' && order->orderSide == Types::OrderSide::sell) {
                //     itr->second.putSell = false;
                // }
            }
        }

        void UnderlyEngine::getFutureInfoByInstrumentID(Types::Instrument_t const& instrument, Types::InstrumentInfo* & futureInsInfo) {
            auto itr = m_symbolMap.find(instrument);
            if (itr == m_symbolMap.end()) {
               assert(false);
            }
            futureInsInfo = &itr->second->instrumentInfo;

        }

        void UnderlyEngine::getOptionInfoByUnderly(Types::Instrument_t const& underlyID, Types::InstrumentInfo* & optionInsInfo) {
            for (auto & itr : m_symbolMap) {
                if (strcmp(itr.second->instrumentInfo.underly.data(), underlyID.data()) == 0) {
                    optionInsInfo =  &itr.second->instrumentInfo;
                    break;
                }
            }

            if (optionInsInfo == nullptr) {
                assert(false);
            }

        }


        void UnderlyEngine::runEvent(const Types::MarketData *pMD, const int64_t epoch_time) {
            auto symbolItr = m_symbolMap.find(pMD->instrumentID);
            if (symbolItr == m_symbolMap.end()) {
                assert(false);
            }
            symbolItr->second->lastMD = pMD;

            for (auto i = 0; i < m_futurePolicyVec.size(); i++) {
                m_futurePolicyVec[i]->runTick(pMD);
            }

            for (auto i = 0; i < m_optionPolicyVec.size(); i++) {
                m_optionPolicyVec[i]->runTick(pMD);
            }

            std::map<Types::Instrument_t, int> targetMap;


            for (auto i = 0; i < m_futurePolicyVec.size(); i++) {
                _syncTargetMap(m_futurePolicyVec[i]->m_targetSignal.targetPosMaps, targetMap);
            }

            for (auto i = 0; i < m_optionPolicyVec.size(); i++) {
                //call target

                _syncTargetMap(m_optionPolicyVec[i]->m_callPolicySymbols.targetSignal.targetPosMaps, targetMap);
                _syncTargetMap(m_optionPolicyVec[i]->m_putPolicySymbols.targetSignal.targetPosMaps, targetMap);
            }


            if (strcmp(pMD->instrumentID.data(), pMD->instrumentID.data()) == 0) {
                for (auto &symbolItr: m_symbolMap) {
                    if (true == Utils::TradingHours::isNoTradeBeforeTime(symbolItr.second->instrumentInfo.instrumentID,
                                                                         pMD->psSecond, 5)) {
                        //    cancelOrder(symbolItr.second->order, symbolItr.second, epoch_time);
                    } else if (((pMD->psSecond > 30 && pMD->psSecond < 19900) || (pMD->psSecond > 32430))
                               && symbolItr.second->lastMD != nullptr // && strcmp(m_underlyInsMap.nearInstrument.data(), symbolItr.first.data()) != 0
                               ) {
                        auto targetItr = targetMap.find(symbolItr.first);
                        symbolItr.second->targetPosition = 0;
                        if (targetItr != targetMap.end()) {
                            symbolItr.second->targetPosition = targetItr->second;
                        }

                        syncPosition(symbolItr.second, epoch_time);
                    }
                }
            }
        }

        int UnderlyEngine::_syncTargetMap(std::map<Types::Instrument_t, int> const &optionTargetPosMaps,
                                                std::map<Types::Instrument_t, int> &targetMap) {
            for (auto itr = optionTargetPosMaps.begin(); itr != optionTargetPosMaps.end(); itr++) {
                auto targetItr = targetMap.find(itr->first);
                if (targetItr == targetMap.end()) {
                    targetMap[itr->first] = 0;
                    targetItr = targetMap.find(itr->first);
                }
                targetItr->second += itr->second;
            }
            return 1;
        }

        int UnderlyEngine::syncPosition(Types::Symbol *symbol, const int64_t epoch_time) {
            if (symbol->order->isTerminal == false) {
                //&& isPendingOrderTimeout(symbol, epoch_time) == false) {
                if (isPendingOrderTimeout(symbol, epoch_time) == true) {
                    cancelOrder(symbol->order, symbol, epoch_time);
                }
                return 0;
            }
            if (symbol->lastMD->psSecond - symbol->lastOrderPsTime < 2 &&
                symbol->order->isTerminal == true) {
                return 0;
            }

            int pendingPosition = symbol->getPendingPosition();
            int posDiff = symbol->targetPosition - pendingPosition - symbol->tradePosition.filledPosition;

            if (posDiff != 0) {
                spdlog::info(
                    "syncPosition m_engineName={}, updateTime={}, symbolName={}, posDiff={}, targetPosition={}, "
                    "pendingPosition={}, filledPosition={}, orderIsTerminal={}, ",
                    m_engineName, symbol->lastMD->psSecond, symbol->instrumentInfo.instrumentID.data(), posDiff,
                    symbol->targetPosition, pendingPosition, symbol->tradePosition.filledPosition,
                    symbol->order->isTerminal);

                if (symbol->order->isTerminal == true) {

                    processSignal(symbol, posDiff, epoch_time);
                } else if (symbol->lastMD->psSecond - symbol->lastOrderPsTime > 2) {
                    cancelOrder(symbol->order, symbol, epoch_time);
                } else if (posDiff * pendingPosition < 0) {
                    cancelOrder(symbol->order, symbol, epoch_time);
                }
            } else if (posDiff == 0 && symbol->getPendingPosition() == 0) {
                symbol->sendCount = 0;
            }

            // reset PutHitFlag
            symbol->intension = Types::SignalIntension::put;
            return 1;
        }


        bool UnderlyEngine::isPendingOrderTimeout(const Types::Symbol *symbol, int64_t epoch_time) {
            //            	fprintf(stderr, "isPendingOrderTimeout, symbolName=%s, psSecond=%d, lastOrderPsTime=%d, m_putResendTimeout=%d\n",
            //            			  symbol->instrumentInfo.instrumentID.data(), symbol->lastMD->psSecond,symbol->lastOrderPsTime, m_putResendTimeout);
            if (symbol->lastMD->psSecond - symbol->lastOrderPsTime > m_engineParam.putResendTimeout &&
                symbol->order->isTerminal == false) {
                if (symbol->order->orderSide == Types::OrderSide::buy &&
                    symbol->lastMD->bidPrice[0] > symbol->order->orderPrice + 0.0001) {
                    return true;
                    // cancelOrder(symbol->order, symbol, epoch_time);
                } else if (symbol->order->orderSide == Types::OrderSide::sell &&
                           symbol->lastMD->askPrice[0] < symbol->order->orderPrice - 0.0001) {
                    return true;
                    //cancelOrder(symbol->order, symbol, epoch_time);
                }
            }
            return false;
        }

        void UnderlyEngine::setInitLog() {
            std::string record = "record";
            m_orderLog = Utils::initLogs(record, this->m_engineName, record);

            std::string position = "position";
            m_positionLog =  Utils::initLogs(position, this->m_engineName, position);
        };

        void UnderlyEngine::processSignal(Types::Symbol *symbol, int posDiff, const int64_t epoch_time) {
            Types::Signal signal;
            signal.intension = getIntension(symbol);

            if (posDiff > 0) {
                if (signal.intension == Types::SignalIntension::put) {
                    signal.price = symbol->lastMD->bidPrice[0] - symbol->instrumentInfo.tickSize;
                    signal.orderTimeType = Types::OrderTimeType::COMMON;
                } else if (signal.intension == Types::SignalIntension::hit) {
                    signal.price = symbol->lastMD->askPrice[0];
                    signal.orderTimeType = Types::OrderTimeType::COMMON;
                } else {
                    assert(false);
                }
                signal.volume = std::min(std::min(m_engineParam.futureMinVolume, 90), abs(posDiff)); // abs(posDiff);
                signal.signalSide = Types::OrderSide::buy;
                signal.epoch_time = epoch_time;
                sendSignal(signal, symbol);
                symbol->sendCount += 1;
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;
            } else if (posDiff < 0) {
                if (signal.intension == Types::SignalIntension::put) {
                    signal.price = symbol->lastMD->askPrice[0] + symbol->instrumentInfo.tickSize;
                    //    m_signal.price = symbol->lastMD->upperLimitPrice;
                    signal.orderTimeType = Types::OrderTimeType::COMMON;
                } else if (signal.intension == Types::SignalIntension::hit) {
                    signal.price = symbol->lastMD->bidPrice[0];
                    signal.orderTimeType = Types::OrderTimeType::COMMON;
                } else {
                    assert(false);
                }
                signal.volume = std::min(std::min(m_engineParam.futureMinVolume, 90), abs(posDiff)); //abs(posDiff);
                signal.signalSide = Types::OrderSide::sell;
                signal.epoch_time = epoch_time;
                sendSignal(signal, symbol);
                symbol->sendCount += 1;
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;
            }
        }

        void UnderlyEngine::cancelOrder(const Types::OrderField *pOrder, Types::Symbol *symbol,
                                              const int64_t epoch_time) {
            if (pOrder != nullptr && !pOrder->isTerminal) {
                int64_t sendTime = 0;
                Utils::logOrder(pOrder, m_orderLog, symbol, Types::OrderStatus::cancel, m_tradingDay,
                                epoch_time);
                m_driver->cancelOrder(*pOrder, sendTime);
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;
            }
        }

        Types::SignalIntension UnderlyEngine::getIntension(const Types::Symbol *symbol) {
            //  if (symbol->intension != Types::SignalIntension::put) {
            //      return symbol->intension;
            //    }
            // else if (symbol->sendCount >= m_resendCount) {
            //   return Types::SignalIntension::hit;
            //  }
            return Types::SignalIntension::put;
        }

        void
        UnderlyEngine::sendSignal(Types::Signal const &signal, Types::Symbol *symbol) {
            //            if(signal.price <0.0001){
            //                return;
            //            }
            int assignid = 0;
            auto order = m_orderList.getNewMemory(assignid);
            order->pOrderID = assignid;
            symbol->order = order;
            setOrder(signal, order, symbol);
            Utils::logOrder(order, m_orderLog, symbol, Types::OrderStatus::signal,
                            m_tradingDay, signal.epoch_time);
            m_driver->sendOrder(*order);
        }

        Types::PositionEffectType
        UnderlyEngine::getPet(int &tradeVolume, int T_hold, int Y_hold, int minPosition, bool prioCloseToday) {
            if (prioCloseToday == 1 && T_hold >= tradeVolume && (T_hold + Y_hold - tradeVolume >= minPosition)) {
                return Types::PositionEffectType::T_close;
            } else if (Y_hold >= tradeVolume && (T_hold + Y_hold - tradeVolume >= minPosition)) {
                return Types::PositionEffectType::Y_close;
            } else if (T_hold > 0 && T_hold < tradeVolume) {
                tradeVolume = T_hold;
                return Types::PositionEffectType::T_close;
            } else if (Y_hold > 0 && Y_hold < tradeVolume) {
                tradeVolume = Y_hold;
                return Types::PositionEffectType::Y_close;
            }
            return Types::PositionEffectType::open;
        }
    }
}
