//
// Created by Zhangyingwei on 2023/5/10.
//

#include "UnderlyOptionEngine.h"
#include "../Types/QuerySymbol.h"
//#include "../Policy/SellStrangle.h"
#include "../Policy/BuyStrangle.h"
#include "../Policy/LongGammaSAR.h"
#include "../Policy/LongGammaGod.h"
#include "../Policy/LongGammaVulture.h"
#include "../Policy/Calendar.h"
#include "../Policy/SellStrangleV2.h"

namespace TrendFollow {
    namespace UnderlyOptionEngine {
        void UnderlyOptionEngine::onStart() {
            if(m_isDay == false && ( (m_underlyInsMap.nearInstrument[0] == 'I' && m_underlyInsMap.nearInstrument[1] == 'F')
                                     || (m_underlyInsMap.nearInstrument[0] == 'I' && m_underlyInsMap.nearInstrument[1] == 'H')
                                     || (m_underlyInsMap.nearInstrument[0] == 'I' && m_underlyInsMap.nearInstrument[1] == 'M')
            ) ){
                return;
            }

            for(auto& itr: m_underlyInsMap.underlyInsMap){
                 Types::UnderlyInfo queryUnderInfo;
                strcpy(queryUnderInfo.instrumentID.data(), itr.first.data());
                Utils::InstrumentToProduct(queryUnderInfo.instrumentID, queryUnderInfo.productID);
                m_driver->send(queryUnderInfo);
                _queryQuote(itr.first);
            }

            if(m_symbolMap.size() <=1){  //may by option is expired
                return;
            }

            for (auto &ins: m_symbolMap) {
                _queryQuote(ins.first);
                ins.second->m_positionLog = m_positionLog;
                ins.second->order = m_orderList.getNewMemory();

                _querySymbol(ins.first);
            }
            m_updateGreeks.init(m_kDataManager.m_allKLineSeries, m_tradingDay, m_isDay);

            for(auto& itr: m_underlyInsMap.underlyInsMap){
                _querySymbol(itr.first);
            }

             Types::SubscribeEngine subscribeEngine;
            subscribeEngine.policyid = m_policyID;
            m_driver->subscribePolicy(subscribeEngine, this);

            for (auto policy: m_policyVec) {

                policy->start(m_symbolMap);
            }
        };


        void UnderlyOptionEngine::_queryQuote(Types::Instrument_t const& queryInstrumentID){
             Types::SubScribeQuote SubScribeQuote;
            strcpy(SubScribeQuote.instrumentID.data(), queryInstrumentID.data());
            SubScribeQuote.policyID = m_policyID;
            m_driver->subscribeQuote(SubScribeQuote);
        }

        void UnderlyOptionEngine::_querySymbol(Types::Instrument_t const& queryInstrumentID){
             Types::QuerySymbol queryUnderlySymbol;
            queryUnderlySymbol.id = m_policyID;
            strcpy(queryUnderlySymbol.instrumentID.data(), queryInstrumentID.data());
            m_driver->send(queryUnderlySymbol);
        }

        void UnderlyOptionEngine::_initUnderly(std::string const& underlyIDStr){
            Types::Instrument_t underlyID{""};
            std::copy(std::begin(underlyIDStr), std::end(underlyIDStr), std::begin(underlyID));
            Types::InstrumentInfo underlyInfo;
            strcpy(underlyInfo.instrumentID.data(), underlyID.data());
            underlyInfo.productIDClass = Types::ProductClass::future;
            Utils::InstrumentToProduct(underlyInfo.instrumentID, underlyInfo.productID);
            auto underlySymbol = new Types::Symbol();
            underlySymbol->policyID = m_policyID;
            memcpy(&underlySymbol->instrumentInfo, &underlyInfo, sizeof(Types::InstrumentInfo));
            m_symbolMap[underlyInfo.instrumentID] = underlySymbol;
            Types::RiskOrderControl riskOrderControl;
            m_underlyInsMap.underlyInsMap[underlyID] = riskOrderControl;
        }




        void UnderlyOptionEngine::onParams( Types::Param const &param) {
            fprintf(stderr, "[%s] OnSetParam, %s=%s\n", m_engineName.c_str(), param.name.c_str(), param.value.c_str());

            if (0 == param.name.compare("putResendTimeout")) {
                m_putResendTimeout = stoi(param.value);
            } else if (0 == param.name.compare("resendCount")) {
                m_resendCount = stoi(param.value);
            } else if (0 == param.name.compare("preCloseToday")) {
                m_preCloseToday = stoi(param.value);
            } else if (0 == param.name.compare("rebackTickCounts")) {
                m_rebackTickCounts = stoi(param.value);
            } else if (0 == param.name.compare("minVolume")) {
                m_minVolume = stoi(param.value);
            } else if (0 == param.name.compare("affiThreadId")) {
                m_affiThreadId = stoi(param.value);
            } else if (0 == param.name.compare("tickSize")) {
                m_tickSize = stof(param.value);
            } else if (0 == param.name.compare("multi")) {
                m_multi = stof(param.value);
            } else if (0 == param.name.compare("hedgeType")) {
                m_hedgeType = Types::HedgeType::spec;
                int hedgeType = stoi(param.value);
                if(hedgeType == 1){
                    m_hedgeType = Types::HedgeType::hedge;
                }
            }
            else if (0 == param.name.compare("maxPosition")) {
                m_maxPosition = stoi(param.value);
            }  else if (0 == param.name.compare("underly")) {

                strcpy(m_underlyInsMap.nearInstrument.data(), param.value.c_str());
                _initUnderly(param.value);
            } else if (0 == param.name.compare("subPolicy")) {
                if(m_isDay == false && ( (m_underlyInsMap.nearInstrument[0] == 'I' && m_underlyInsMap.nearInstrument[1] == 'F')
                                         || (m_underlyInsMap.nearInstrument[0] == 'I' && m_underlyInsMap.nearInstrument[1] == 'H')
                                         || (m_underlyInsMap.nearInstrument[0] == 'I' && m_underlyInsMap.nearInstrument[1] == 'M')
                ) ){
                    return;
                }
                auto policyName = Types::Param::getValue(param.paramMap, "policyName");
                auto policy = createPolicy(param.paramMap, policyName);
                if (policy == nullptr) {
                    assert(false && "policy is nullptr");
                }
                m_policyVec.emplace_back(policy);
            }
        };

         Policy::IOptionPolicy *
        UnderlyOptionEngine::createPolicy(std::map<std::string, std::string> const & paramMap, std::string const& policyName) {
            fprintf(stderr, "policyName = %s\n", policyName.c_str());
           /* if (policyName.compare("SellStrangle") == 0) {
                auto kpstr = Types::Param::getValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                std::string tt_str = Types::Param::getValue(paramMap, "tradeTime");
                int tradeTime = Utils::ToPsSeconds(tt_str);
                int isRefreshDelta = std::stoi(Types::Param::getValue(paramMap, "isRefreshDelta"));
                double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
                double adjCoef = std::stof(Types::Param::getValue(paramMap, "adjCoef").c_str());

                if(m_policyKPMap.find(kPeriod) == m_policyKPMap.end()){
                    m_policyKPMap[kPeriod] = false;
                }

                return new  Policy::SellStrangle(kPeriod, MV * 10000, policyName,
                                                             m_engineName, m_underlyInsMap.nearInstrument, tradeTime, m_multi,
                                                             m_tradingDay, m_maxPosition, isRefreshDelta, openAtDelta, adjCoef);
            }else*/ if (policyName.compare("SellStrangleV2") == 0) {
                auto kpstr = Types::Param::getValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                int isRefreshDelta = std::stoi(Types::Param::getValue(paramMap, "isRefreshDelta"));
                double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
                double baseRation = std::stof(Types::Param::getValue(paramMap, "biasRation").c_str());
                if(m_policyKPMap.find(kPeriod) == m_policyKPMap.end()){
                    m_policyKPMap[kPeriod] = false;
                }
                return new  Policy::SellStrangleV2(kPeriod, MV * 10000, policyName,
                                                             m_engineName, m_underlyInsMap.nearInstrument,  m_multi,
                                                             m_tradingDay, m_maxPosition, isRefreshDelta, openAtDelta,baseRation);
            }else if (policyName.compare("BuyStrangle") == 0) {
                auto kpstr = Types::Param::getValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                std::string tt_str = Types::Param::getValue(paramMap, "tradeTime");
                int tradeTime = Utils::ToPsSeconds(tt_str);
                int isRefreshDelta = std::stoi(Types::Param::getValue(paramMap, "isRefreshDelta"));
                double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
                double adjCoef = std::stof(Types::Param::getValue(paramMap, "adjCoef").c_str());
                if(m_policyKPMap.find(kPeriod) == m_policyKPMap.end()){
                    m_policyKPMap[kPeriod] = false;
                }
                return new  Policy::BuyStrangle(kPeriod, MV * 10000, policyName,
                                                             m_engineName, m_underlyInsMap.nearInstrument, tradeTime, m_multi,
                                                             m_tradingDay, m_maxPosition, isRefreshDelta, openAtDelta,adjCoef);
            }else if (policyName.compare("Calendar") == 0) {
                auto kpstr = Types::Param::getValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                std::string tt_str = Types::Param::getValue(paramMap, "tradeTime");
                int tradeTime = Utils::ToPsSeconds(tt_str);
                int isRefreshDelta = std::stoi(Types::Param::getValue(paramMap, "isRefreshDelta"));
                double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
                double adjCoef = std::stof(Types::Param::getValue(paramMap, "adjCoef").c_str());
                std::string farUnderlyStr = Types::Param::getValue(paramMap, "farUnderly");
                _initUnderly(farUnderlyStr);
                strcpy(m_underlyInsMap.farInstrument.data(), farUnderlyStr.c_str());
                if(m_policyKPMap.find(kPeriod) == m_policyKPMap.end()){
                    m_policyKPMap[kPeriod] = false;
                }
                return new  Policy::Calendar(kPeriod, MV * 10000, policyName,
                                                            m_engineName, m_underlyInsMap.nearInstrument, m_underlyInsMap.farInstrument,
                                                            tradeTime, m_multi, m_tradingDay, m_maxPosition, isRefreshDelta, openAtDelta,
                                                            adjCoef);
            }else if (policyName.compare("LongGammaSAR") == 0) {
                auto kpstr = Types::Param::getValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                m_policyKPMap[kPeriod] = true;
                double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
                return new  Policy::LongGammaSAR(kPeriod, MV * 10000, policyName,
                                                            m_engineName, m_underlyInsMap.nearInstrument,  m_multi,m_tradingDay, m_maxPosition,
                                                            openAtDelta);
            }else if (policyName.compare("LongGammaVulture") == 0) {
                auto kpstr = Types::Param::getValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                m_policyKPMap[kPeriod] = true;
                m_policyKPMap[Types::KPeriod::D1] = true;
                double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
                return new  Policy::LongGammaVulture(kPeriod, MV * 10000, policyName,
                                                            m_engineName, m_underlyInsMap.nearInstrument,  m_multi,m_tradingDay, m_maxPosition,
                                                            openAtDelta);
            }else if (policyName.compare("LongGammaGod") == 0) {
                auto kpstr = Types::Param::getValue(paramMap, "period");
                Types::KPeriod kPeriod = Types::configParamToKPeriodMap.at(kpstr);
                m_policyKPMap[kPeriod] = false;
                double MV = std::stof(Types::Param::getValue(paramMap, "MV").c_str());
                double openAtDelta = std::stof(Types::Param::getValue(paramMap, "openAtDelta").c_str());
                int godDirection = std::stoi(Types::Param::getValue(paramMap, "godDirection").c_str());
                double closeExceedThresh = std::stof(Types::Param::getValue(paramMap, "closeExceedThresh").c_str());
                return new  Policy::LongGammaGod(kPeriod, MV * 10000, policyName,
                                                            m_engineName, m_underlyInsMap.nearInstrument,  m_multi,m_tradingDay, m_maxPosition,
                                                            openAtDelta, godDirection, closeExceedThresh);
            }
            return nullptr;

        }

        void UnderlyOptionEngine::onRtnSubScribeQuote( Types::OnSubScribeQuote const &onSubScribeQuote) {

            if (onSubScribeQuote.policyID == m_policyID) {
                auto symbolItr = m_symbolMap.find(onSubScribeQuote.instrumentID);
                if (symbolItr == m_symbolMap.end()) {
                    assert(false && "no itrinsinfo in m_instrumentInfoMap");
                }
                if (m_kDataManager.m_allKLineSeries.find(onSubScribeQuote.instrumentID) ==
                    m_kDataManager.m_allKLineSeries.end()) {
                    for(auto& kpitr : m_policyKPMap){
                        initKSeries(m_tradingDay, kpitr.first, m_isDay, symbolItr->second->instrumentInfo, kpitr.second, m_isReal);
                    }

                    auto temp = new std::unordered_map< Types::KPeriod, int *>();
                    for (auto itr = m_kDataManager.m_allKLineSeries.begin();
                         itr != m_kDataManager.m_allKLineSeries.end(); itr++) {
                        for (auto ksitr = itr->second->begin(); ksitr != itr->second->end(); ksitr++) {
                             if (strcmp(ksitr->second->m_insInfo.instrumentID.data(), onSubScribeQuote.instrumentID.data())==0) {
                                 (*temp)[ksitr->first] = new int(ksitr->second->m_seriesIndex);
                             }
                        }
                    }
                    m_allKLineIndexs[onSubScribeQuote.instrumentID] = temp;
                }
                for(auto& kpitr : m_policyKPMap){
                    symbolItr->second->m_kSeriesMap[kpitr.first] = m_kDataManager.getSeries(symbolItr->second->instrumentInfo.instrumentID,
                                                                                            kpitr.first);
                }

            //    fprintf(stderr, "onRtnSubScribeQuote : %s, %x\n",symbolItr->second->instrumentInfo.instrumentID.data(), symbolItr);
            }
        };

        void UnderlyOptionEngine::onInstrumentInfo( Types::InstrumentInfo const &instrumentInfo) {


            if ( m_underlyInsMap.isIn(instrumentInfo.underly) == true ) {
                auto optionSymbol = new Types::Symbol();
                optionSymbol->policyID = m_policyID;
                memcpy(&optionSymbol->instrumentInfo, &instrumentInfo, sizeof(Types::InstrumentInfo));
                m_symbolMap[optionSymbol->instrumentInfo.instrumentID] = optionSymbol;
            }
        };

        void UnderlyOptionEngine::onRtnQuerySymbolPosition( Types::OnQuerySymbol const &onQuerySymbol) {
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

        void UnderlyOptionEngine::onEventData( Types::EventData const &eventData) {
            if (eventData.type == 0) {
                auto pMD = (const  Types::MarketData *) eventData.point;
                if(pMD->psSecond < m_lastPS-30){
                    return;
                }
                m_lastPS = pMD->psSecond;
//                fprintf(stderr, "onEventData instrumentid=%s, updateTime=%s.%d, price=%.3f\n",pMD->instrumentID.data(),
//                        pMD->updateTime.data(), pMD->milliSeconds, pMD->lastPrice);
                auto itr = m_kDataManager.m_allKLineSeries.find(pMD->instrumentID);
                if (itr == m_kDataManager.m_allKLineSeries.end()) {
                    fprintf(stderr, "onEventData %s\n", pMD->instrumentID.data());
                    assert(false);
                }

               for(auto & kpitr : m_policyKPMap){
                   auto series = m_kDataManager.getSeries(pMD->instrumentID, kpitr.first);
                   series->addTick(pMD);
               }

                auto seMap = itr->second;
                for (auto &sitr: *seMap) {
                    auto series = sitr.second;
                    auto itrindexMap = m_allKLineIndexs.find(pMD->instrumentID);
                    if (itrindexMap == m_allKLineIndexs.end()) {
                        assert(false && "no find instrument indexMap");
                    }
                    auto itrindex = itrindexMap->second->find(series->m_Period);
                    if (itrindex == itrindexMap->second->end()) {
                        assert(false && "no find peorid indexMap");
                    }

                    while (*(itrindex->second) < series->m_seriesIndex) {
                        int saveIndex = *(itrindex->second);
                        *(itrindex->second) = *(itrindex->second) + 1;
                        //     fprintf(stderr, "updateKLine %s %s\n", pMD->instrumentID.data(), pMD->updateTime.data());
                        m_updateGreeks.updateGreeks(series->m_insInfo.instrumentID, sitr.first, series);
                    }
                }
                 if(pMD->isInit == 0 ) {
                   this->runEvent(pMD, pMD->epoch_time);
		}
            } else if (eventData.type == 1) {
                auto inputOrder = (const  Types::OrderField *) eventData.point;
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

        void UnderlyOptionEngine::updateMMOrder(const  Types::OrderField *inputOrder,
                                                 Types::Symbol *symbol) {


            if (inputOrder->policyID != m_policyID) {
                assert(false && "OpenEngine::updateOrder policyID not match");
            }
            auto order = m_orderList.getMemoryById(inputOrder->pOrderID);
             Utils::updateOrder(order, inputOrder);

            if(inputOrder->orderStatus == Types::OrderStatus::failed){

            }
            else if (order->orderStatus ==  Types::OrderStatus::partTraded ||
                order->orderStatus ==  Types::OrderStatus::allTraded) {
                symbol->tradePosition.update_filled_position(order->orderSide, order->pet,
                                                             order->orderPrice, order->lastFilledVolume);
                symbol->symbolLastFilledPrice = order->lastFilledPrice;
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;
                symbol->posWrite(true, m_tradingDay, symbol->lastMD->updateTime,
                                 symbol->lastMD->milliSeconds, order->tOrderID);
            }

            if (order->isTerminal == true) {
                auto itr  = m_underlyInsMap.underlyInsMap.find(symbol->instrumentInfo.underly);
                if (itr ==  m_underlyInsMap.underlyInsMap.end()){
                    assert(false);
                }
                if(symbol->instrumentInfo.optionType =='C' && order->orderSide==Types::OrderSide::buy){
                    itr->second.callBuy = false;
                }else if(symbol->instrumentInfo.optionType =='C' && order->orderSide==Types::OrderSide::sell){
                    itr->second.callSell = false;
                }else if(symbol->instrumentInfo.optionType =='P' && order->orderSide==Types::OrderSide::buy){
                    itr->second.putBuy = false;
                }else if(symbol->instrumentInfo.optionType =='P' && order->orderSide==Types::OrderSide::sell){
                    itr->second.putSell = false;
                }
            }
        }


        void UnderlyOptionEngine::runEvent(const  Types::MarketData *pMD, const int64_t epoch_time) {
            auto symbolItr = m_symbolMap.find(pMD->instrumentID);
            if (symbolItr == m_symbolMap.end()) {
                assert(false);
            }
            symbolItr->second->lastMD = pMD;

            for (auto i = 0; i < m_policyVec.size(); i++) {
                m_policyVec[i]->runTick(pMD);
            }

            std::map<Types::Instrument_t, int> targetMap;
            for (auto i = 0; i < m_policyVec.size(); i++) { //call target

                _syncTargetMap(m_policyVec[i]->m_callPolicySymbols.optionTargetPosMaps, targetMap);
                _syncTargetMap(m_policyVec[i]->m_putPolicySymbols.optionTargetPosMaps, targetMap);

            }


            if (strcmp(pMD->instrumentID.data(), pMD->instrumentID.data()) == 0) {
                for (auto& symbolItr: m_symbolMap) {
                    if (true == Utils::TradingHours::isNoTradeBeforeTime(symbolItr.second->instrumentInfo.instrumentID,
                                                                         pMD->psSecond, 5)) {
                        //    cancelOrder(symbolItr.second->order, symbolItr.second, epoch_time);
                    } else if (((pMD->psSecond > 30 && pMD->psSecond < 19900) || (pMD->psSecond > 32430))
                               && symbolItr.second->lastMD != nullptr &&
                               strcmp(m_underlyInsMap.nearInstrument.data(), symbolItr.first.data())!=0 ) {

                        auto targetItr = targetMap.find(symbolItr.first);
                        symbolItr.second->targetPosition = 0;
                        if(targetItr != targetMap.end()){
                            symbolItr.second->targetPosition = targetItr->second;
                        }

                        syncPosition(symbolItr.second, epoch_time);
                    }
                }
            }
        }

        int UnderlyOptionEngine::_syncTargetMap(std::map<Types::Instrument_t, int> const & optionTargetPosMaps,
                                                    std::map<Types::Instrument_t, int> & targetMap){
            for(auto itr = optionTargetPosMaps.begin(); itr!=optionTargetPosMaps.end(); itr++){
                auto targetItr = targetMap.find(itr->first);
                if(targetItr == targetMap.end()){
                    targetMap[itr->first] = 0;
                    targetItr = targetMap.find(itr->first);
                }
                targetItr->second += itr->second;
            }
            return 1;
        }

        int UnderlyOptionEngine::syncPosition( Types::Symbol *symbol,const int64_t epoch_time) {

            if ( symbol->order->isTerminal == false ){//&& isPendingOrderTimeout(symbol, epoch_time) == false) {
                 if(isPendingOrderTimeout(symbol, epoch_time) == true) {
                     cancelOrder(symbol->order, symbol, epoch_time);
                 }
                return 0 ;
            }
            if (symbol->lastMD->psSecond - symbol->lastOrderPsTime < 2 &&
                symbol->order->isTerminal == true){
                return 0 ;
            }

            int pendingPosition = symbol->getPendingPosition();
            int posDiff = symbol->targetPosition - pendingPosition - symbol->tradePosition.filledPosition;

            if ( posDiff !=0) {
                auto itr  = m_underlyInsMap.underlyInsMap.find(symbol->instrumentInfo.underly);
                spdlog::info(
                        "syncPosition m_engineName={}, updateTime={}, symbolName={}, posDiff={}, targetPosition={}, "
                        "pendingPosition={}, filledPosition={}, orderIsTerminal={}, callBuy={}, callSell={}, putBuy={}, "
                        "putSell={}",
                        m_engineName, symbol->lastMD->psSecond, symbol->instrumentInfo.instrumentID.data(), posDiff,
                        symbol->targetPosition, pendingPosition, symbol->tradePosition.filledPosition, symbol->order->isTerminal,
                        itr->second.callBuy, itr->second.callSell,  itr->second.putBuy, itr->second.putSell );

                if (symbol->order->isTerminal == true) {

                    if (itr ==  m_underlyInsMap.underlyInsMap.end()){
                        assert(false);
                    }
                    if(symbol->instrumentInfo.optionType =='C' &&
                            ((posDiff>0 &&  itr->second.callBuy == true) || (posDiff<0 &&  itr->second.callSell == true))){
                        return 0;
                    }
                    if(symbol->instrumentInfo.optionType =='P' &&
                            ((posDiff>0 &&  itr->second.putBuy == true) || (posDiff<0 &&  itr->second.putSell == true))){
                        return 0;
                    }
                    processSignal(symbol, posDiff, epoch_time);
                } else if(symbol->lastMD->psSecond - symbol->lastOrderPsTime > 2) {
                    cancelOrder(symbol->order, symbol, epoch_time);
                }else if(posDiff * pendingPosition <0){
                    cancelOrder(symbol->order, symbol, epoch_time);
                }
            } else if (posDiff == 0 && symbol->getPendingPosition() == 0) {
                symbol->sendCount = 0;
            }

            // reset PutHitFlag
            symbol->intension = Types::SignalIntension::put;
            return 1 ;
        }


        bool UnderlyOptionEngine::isPendingOrderTimeout(const Types::Symbol *symbol, int64_t epoch_time) {

//            	fprintf(stderr, "isPendingOrderTimeout, symbolName=%s, psSecond=%d, lastOrderPsTime=%d, m_putResendTimeout=%d\n",
//            			  symbol->instrumentInfo.instrumentID.data(), symbol->lastMD->psSecond,symbol->lastOrderPsTime, m_putResendTimeout);
            if (symbol->lastMD->psSecond - symbol->lastOrderPsTime > m_putResendTimeout &&
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

        void UnderlyOptionEngine::setInitLog(Types::LogStruct &logStruct) {

            m_orderLog = logStruct.recordLog;
            m_positionLog = logStruct.m_positionLog;
        };

        void UnderlyOptionEngine::processSignal(Types::Symbol *symbol, int posDiff, const int64_t epoch_time) {
             Types::Signal signal;
            signal.intension = getIntension(symbol);
            if (posDiff > 0) {
                if (signal.intension == Types::SignalIntension::put) {
                    signal.price = symbol->lastMD->bidPrice[0] - m_rebackTickCounts * m_tickSize;
                    signal.orderTimeType =  Types::OrderTimeType::COMMON;
                } else if (signal.intension == Types::SignalIntension::hit) {
                    signal.price = symbol->lastMD->askPrice[0];
                    signal.orderTimeType =  Types::OrderTimeType::COMMON;
                } else {
                    assert(false);
                }
                signal.volume = std::min(std::min(m_minVolume, 90), abs(posDiff));// abs(posDiff);
                signal.signalSide =  Types::OrderSide::buy;
                signal.epoch_time = epoch_time;
                sendSignal(signal, symbol);
                symbol->sendCount += 1;
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;
            } else if (posDiff < 0) {
                if (signal.intension == Types::SignalIntension::put) {
                    signal.price = symbol->lastMD->askPrice[0] +  m_rebackTickCounts * m_tickSize;
                    //    m_signal.price = symbol->lastMD->upperLimitPrice;
                    signal.orderTimeType =  Types::OrderTimeType::COMMON;
                } else if (signal.intension == Types::SignalIntension::hit) {
                    signal.price = symbol->lastMD->bidPrice[0];
                    signal.orderTimeType =  Types::OrderTimeType::COMMON;
                } else {
                    assert(false);
                }
                signal.volume = std::min(std::min(m_minVolume, 90), abs(posDiff));//abs(posDiff);
                signal.signalSide =  Types::OrderSide::sell;
                signal.epoch_time = epoch_time;
                sendSignal(signal, symbol);
                symbol->sendCount += 1;
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;
            }
        }

        void UnderlyOptionEngine::cancelOrder(const  Types::OrderField *pOrder,  Types::Symbol *symbol,
                                              const int64_t epoch_time) {
            if (pOrder != nullptr && !pOrder->isTerminal) {
                int64_t sendTime = 0;
                 Utils::logOrder(pOrder, m_orderLog, symbol, Types::OrderStatus::cancel, m_tradingDay,
                                             epoch_time);
                m_driver->cancelOrder(*pOrder, sendTime);
                symbol->lastOrderPsTime = symbol->lastMD->psSecond;

            }
        }

        Types::SignalIntension UnderlyOptionEngine::getIntension(const Types::Symbol *symbol) {
            //  if (symbol->intension != Types::SignalIntension::put) {
            //      return symbol->intension;
            //    }
            // else if (symbol->sendCount >= m_resendCount) {
            //   return Types::SignalIntension::hit;
            //  }
            return Types::SignalIntension::put;
        }

        void
        UnderlyOptionEngine::sendSignal( Types::Signal const &signal,  Types::Symbol *symbol) {
//            if(signal.price <0.0001){
//                return;
//            }
            int assignid = 0;
            auto order = m_orderList.getNewMemory(assignid);
            order->pOrderID = assignid;
            symbol->order = order;
            setOrder(signal, order, symbol);
             Utils::logOrder(order, m_orderLog, symbol,  Types::OrderStatus::signal,
                                         m_tradingDay, signal.epoch_time);
            m_driver->sendOrder(*order);

            auto itr  = m_underlyInsMap.underlyInsMap.find(symbol->instrumentInfo.underly);
            if (itr ==  m_underlyInsMap.underlyInsMap.end()){
                assert(false);
            }

            if(symbol->instrumentInfo.optionType =='C' && order->orderSide==Types::OrderSide::buy){
                itr->second.callBuy = true;
            }else if(symbol->instrumentInfo.optionType =='C' && order->orderSide==Types::OrderSide::sell){
                itr->second.callSell = true;
            }else if(symbol->instrumentInfo.optionType =='P' && order->orderSide==Types::OrderSide::buy){
                itr->second.putBuy = true;
            }else if(symbol->instrumentInfo.optionType =='P' && order->orderSide==Types::OrderSide::sell){
                itr->second.putSell = true;
            }

        }

        Types::PositionEffectType
        UnderlyOptionEngine::getPet(int &tradeVolume, int T_hold, int Y_hold, int minPosition, bool prioCloseToday) {
            if (prioCloseToday == 1 && T_hold >= tradeVolume && (T_hold + Y_hold - tradeVolume >= minPosition)) {
                return  Types::PositionEffectType::T_close;
            } else if (Y_hold >= tradeVolume && (T_hold + Y_hold - tradeVolume >= minPosition)) {
                return  Types::PositionEffectType::Y_close;
            } else if (T_hold > 0 && T_hold < tradeVolume) {
                tradeVolume = T_hold;
                return  Types::PositionEffectType::T_close;
            } else if (Y_hold > 0 && Y_hold < tradeVolume) {
                tradeVolume = Y_hold;
                return  Types::PositionEffectType::Y_close;
            }
            return  Types::PositionEffectType::open;
        }


    }
}
