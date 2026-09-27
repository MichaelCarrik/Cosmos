//
// Created by zhangyw on 2026/9/21.
//

#ifndef COSMOS_VANNASABR_H
#define COSMOS_VANNASABR_H

#include "IOptionPolicy.h"
#include "../Types/Type.h"
#include "../Utils/Indicator.h"
#include <vector>

namespace Cosmos {
    namespace Policy {

        struct VannaFileRead {
            std::string instrumentStr{""};
            std::string targetPositionStr{""};
            std::string signalPriceStr{""};
            std::string skewDirectionStr{""};
            std::string skewPositionStr{""};
            std::string callOptionStrikeStr{""};
            std::string putOptionStrikeStr{""};
        };


        class VannaSABR : public IOptionPolicy {
        private:
            int m_configIndex{0};
            bool m_isCheck{false};
            int m_lastPsTime{0};
            int m_maxOptionPosition{0};
            double m_openAtDelta{0.25};
            int m_tradeNum{0};
            int m_skewPosition{0};
            double m_signalPrice{0.0};
            double m_callOptionStrike{0.0};
            double m_putOptionStrike{0.0};
            std::vector<double> m_fiveRhoVec;
            double m_rhoUp;
            double m_rhoDown;
            int m_skewDirection{0};
            int m_rhoLength{0};


        //    std::function<int(Types::Instrument_t const&, Types::KPeriod)> m_getUnderlyToBeginIndexFunc;

        public:
            VannaSABR( std::string const &policyName, std::string const &engineName,
                          Types::Instrument_t &instrument, Types::KPeriod kperiod, double mv, double multi,
                          int tradingDay, int expireDay, int maxOptionPosition,
                         double openAtDelta, int rhoLength, decltype(m_getUnderlyToBeginIndexFunc) getUnderlyToBeginIndexFunc) :  IOptionPolicy(policyName, engineName, instrument,
                                                kperiod, mv, multi,  tradingDay, expireDay, getUnderlyToBeginIndexFunc), m_maxOptionPosition(maxOptionPosition),
                                                m_openAtDelta(openAtDelta), m_rhoLength(rhoLength){
                if (m_openAtDelta < 0.1 || m_openAtDelta > 0.5) {
                    assert(false);
                }

                // spdlog::info("createPolicy engineName={}, policyName={}, kperiod={}  mv={}, tradingday={}, "
                //              "underly={}, , maxPosition={}, openAtDelta={}",
                //              engineName, policyName, (int) kperiod, m_MV, m_tradingday, m_underlyInstrument.data(),
                //              m_maxPosition, m_openAtDelta);
                _initPolicyLogger();
            }

            ~VannaSABR() {}

            void _GetValueFromFileByConfigIndex(char *filename, Types::Instrument_t const& underlyInstrument,
                                                int inputConfigIndex, std::vector<VannaFileRead> & vannaFileReadVecs) {
                char buf[BUFSIZ], *field;

                FILE *fp = NULL;
                fp = fopen(filename, "r");
              //  FileRead fileRead;
                if (fp == NULL) {
                    printf("OnStarted, Warning: Cannot open file: %s !!!\n", filename);
                    return;
                } else {
                    while (fgets(buf, BUFSIZ, fp) != NULL) {
                        std::string configIndexStr{""};
                        char ciname[56]{"configIndex"};
                        _getValueInLine(buf, ciname, configIndexStr);
                        if (std::stoi(configIndexStr.c_str()) == inputConfigIndex) {
                            VannaFileRead vannaFileRead;
                            char insname[56]{"instr"};
                            _getValueInLine(buf, insname, vannaFileRead.instrumentStr);
                            char tagname[56]{"targetPos"};
                            _getValueInLine(buf, tagname, vannaFileRead.targetPositionStr);

                            if(strcmp(vannaFileRead.instrumentStr.c_str(), underlyInstrument.data())==0){
                                char mpname[56]{"skewDirct"};
                                _getValueInLine(buf, mpname, vannaFileRead.skewDirectionStr);

                                char preMpname[56]{"skewPos"};
                                _getValueInLine(buf, preMpname, vannaFileRead.skewPositionStr);

                                char sgnname[56]{"sgnPrice"};
                                _getValueInLine(buf, sgnname, vannaFileRead.signalPriceStr);

                                char cspname[56]{"callStkPrice"};
                                _getValueInLine(buf, cspname, vannaFileRead.callOptionStrikeStr);

                                char pspname[56]{"putStkPrice"};
                                _getValueInLine(buf, pspname, vannaFileRead.putOptionStrikeStr);
                            }
                            vannaFileReadVecs.emplace_back(vannaFileRead);
                        }
                    }
                }
                fclose(fp);
            }

            void initIndicator() {
                _initVannaSABR();
            }

            void _updateVannaSignal(const KData::KData *lastUnderlyBar) {

                int beginI = 0, endI = 0;
                if (m_lastUnderlyBarIndex < m_rhoLength) {
                    beginI = 0;
                    endI = m_lastUnderlyBarIndex;
                } else {
                    beginI = m_lastUnderlyBarIndex - m_rhoLength + 1;
                    endI = m_lastUnderlyBarIndex ;
                }
                m_fiveRhoVec.emplace_back(lastUnderlyBar->m_sabrPRMT.rho);
                m_rhoUp = Indicator::Quartile(m_fiveRhoVec, beginI, endI, 0.75);
                m_rhoDown = Indicator::Quartile(m_fiveRhoVec, beginI, endI, 0.25);

                if (lastUnderlyBar->m_sabrPRMT.rho > m_rhoUp and lastUnderlyBar->m_sabrPRMT.rho >0){
                    m_skewDirection = -1;
                }else if (lastUnderlyBar->m_sabrPRMT.rho < m_rhoDown and lastUnderlyBar->m_sabrPRMT.rho <0) {
                    m_skewDirection = 1;
                }
                else if ((lastUnderlyBar->m_sabrPRMT.rho >0 && m_skewDirection >0 ) || (lastUnderlyBar->m_sabrPRMT.rho <0 && m_skewDirection < 0)){
                    m_skewDirection = 0;
                }
            }

            void _initVannaSABR() {
                    for (auto i = 0; i < m_underlyKseries->m_seriesIndex; i++) {
                        m_fiveRhoVec.emplace_back(m_underlyKseries->m_KDataVecs[i]->m_sabrPRMT.rho);
                    }
                    int beginI = 0, endI = 0;
                    if (m_underlyKseries->m_seriesIndex < m_rhoLength) {
                        beginI = 0;
                        endI = m_underlyKseries->m_seriesIndex-1;
                    } else {
                        beginI = m_underlyKseries->m_seriesIndex - m_rhoLength ;
                        endI = m_underlyKseries->m_seriesIndex -1;
                    }
                    m_rhoUp = Indicator::Quartile(m_fiveRhoVec, beginI, endI, 0.75);
                    m_rhoDown = Indicator::Quartile(m_fiveRhoVec, beginI, endI, 0.25);
            }

            void _thisInitOptionPolicySymbolVecs(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap,
                                       Types::Instrument_t const& underlyInstrument, PolicySymbolStruct & policySymbols,
                                       std::vector<VannaFileRead>const& vannaFileReadVecs, char optionType ){
               // int expireDay =0;
                for(auto symbolItr : inputSymbolMap){
                    if ( symbolItr.second->instrumentInfo.productIDClass == Types::ProductClass::option &&
                          symbolItr.second->instrumentInfo.optionType == optionType &&
                            strcmp(symbolItr.second->instrumentInfo.underly.data(), underlyInstrument.data()) ==0)
                    {
                    //    expireDay = symbolItr.second->instrumentInfo.expireDate;
                        policySymbols.optionSymbolVecs.emplace_back(symbolItr.second);
                        for(auto vannaFileReadItr : vannaFileReadVecs){
                            if(strcmp(vannaFileReadItr.instrumentStr.c_str(), symbolItr.second->instrumentInfo.instrumentID.data())==0){
                                policySymbols.targetSignal.targetPosMaps[symbolItr.second->instrumentInfo.instrumentID] = std::stoi(vannaFileReadItr.targetPositionStr.c_str());
                                policySymbols.targetSignal.lastTargetPosMaps[symbolItr.second->instrumentInfo.instrumentID] = std::stoi(vannaFileReadItr.targetPositionStr.c_str());
                            }
                        }
                    }
                }

                if (optionType =='C'){
                    std::sort(policySymbols.optionSymbolVecs.begin(), policySymbols.optionSymbolVecs.end(),[](auto symbol_a, auto symbol_b){
                        return symbol_a->instrumentInfo.strikePrice < symbol_b->instrumentInfo.strikePrice;
                    });

                }else{
                    std::sort(policySymbols.optionSymbolVecs.begin(), policySymbols.optionSymbolVecs.end(),[](auto symbol_a, auto symbol_b){
                        return symbol_a->instrumentInfo.strikePrice > symbol_b->instrumentInfo.strikePrice;
                    });
                }
             //   return expireDay;
            }

            virtual void updateParam(const Types::NetModifyParam *netModifyParam) override {
                if(strcmp(netModifyParam->paramName.c_str(), "openAtDelta") == 0) {
                    m_openAtDelta = stof(netModifyParam->paramValue);
                    fprintf(stderr, "[%s] updateParam engineName=%s, policyName=%s, instrument=%s, openAtDelta=%.3f\n",
                     m_policyName.c_str(), m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                 m_openAtDelta);
                }else if(strcmp(netModifyParam->paramName.c_str(), "MV") == 0) {
                    m_MV = stof(netModifyParam->paramValue);

                    fprintf(stderr, "[%s] updateParam engineName=%s, policyName=%s, instrument=%s, MV=%.3f\n",
                    m_policyName.c_str(),  m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                 m_MV);
                }
                else if (strcmp(netModifyParam->paramName.c_str(), "targetPos") == 0) {
                    bool isFind =false;
                    auto callItr = m_callPolicySymbols.targetSignal.targetPosMaps.find(netModifyParam->symbolName);
                    if (callItr != m_callPolicySymbols.targetSignal.targetPosMaps.end()) {
                        fprintf(stderr, "[%s] updateParam engineName=%s, policyName=%s, instrument=%s, targetPosition=%s\n",
                               m_policyName.c_str(), m_engineName.c_str(), m_policyName.c_str(), callItr->first.data(),
                                netModifyParam->paramValue.c_str());
                        callItr->second = stoi(netModifyParam->paramValue);
                        isFind = true;

                    }else {
                        auto putItr = m_putPolicySymbols.targetSignal.targetPosMaps.find(netModifyParam->symbolName);
                        if (putItr != m_putPolicySymbols.targetSignal.targetPosMaps.end()) {
                            fprintf(stderr, "[%s] updateParam engineName=%s, policyName=%s, instrument=%s, targetPosition=%s\n",
                                       m_policyName.c_str(),  m_engineName.c_str(), m_policyName.c_str(), putItr->first.data(),
                                    netModifyParam->paramValue.c_str());
                            putItr->second = stoi(netModifyParam->paramValue);
                            isFind = true;
                        }
                    }

                    if (isFind==true) {
                        auto lastUnderlyKB = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex - 1];

                        _writePolicyLog(lastUnderlyKB, m_underlyKseries->m_lastPMD);
                        m_configLog->flush();
                        m_configIndex++;

                    }
                }
            };

            virtual void start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) override {

                m_underlyToBeginIndex =  m_getUnderlyToBeginIndexFunc(m_underlyInstrument, m_kperiod);
                Types::Product_t product{""};
                Utils::InstrumentToProduct(m_underlyInstrument, product);


                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());
                m_configIndex = atoi(this->GetLastValueFromFile(configPath, "configIndex").c_str());
                std::vector<VannaFileRead> vannaFileReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_underlyInstrument, m_configIndex, vannaFileReadVecs);

                _thisInitOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_callPolicySymbols, vannaFileReadVecs, 'C' );
                _thisInitOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_putPolicySymbols, vannaFileReadVecs, 'P' );

                auto symbolItr = inputSymbolMap.find(m_underlyInstrument);
                if(symbolItr == inputSymbolMap.end()){
                    assert(false);
                }
                m_underlyKseries = symbolItr->second->m_kSeriesMap.at(m_kperiod);
            //    m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;

                m_multi = m_underlyKseries->m_insInfo.multi;
                for(auto vannaFileReadItr : vannaFileReadVecs){
                    if(strcmp(vannaFileReadItr.instrumentStr.c_str(), symbolItr->second->instrumentInfo.instrumentID.data())==0){
                        m_skewDirection = std::stoi(vannaFileReadItr.skewDirectionStr.c_str());
                        m_skewPosition = std::stoi(vannaFileReadItr.skewPositionStr.c_str());
                        m_signalPrice = std::stof(vannaFileReadItr.signalPriceStr.c_str());
                        m_callOptionStrike = std::stof(vannaFileReadItr.callOptionStrikeStr.c_str());
                        m_putOptionStrike = std::stof(vannaFileReadItr.putOptionStrikeStr.c_str());
                    }
                }
                m_configIndex++;
                initIndicator();
                fprintf(stderr, "[%s_%s] start, underlyInstrument=%s, kperiod=%d, MV=%.3f, multi=%.3f, tradingDay=%d, expireDay=%d, "
                                "openAtDelta=%.3f, rhoLength=%d, rhoUp=%.3f, rhoDown=%.3f, maxOptionPosition=%d, underlyToBeginIndex=%d, "
                                "skewDirection=%d, skewPosition=%d, signalPrice=%.3f, callStkPrice=%.3f, putStkPrice=%.3f, configIndex=%d\n",
                                m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(), static_cast<int>(m_kperiod), m_MV, m_multi,
                                m_tradingDay, m_expireDay,m_openAtDelta, m_rhoLength, m_rhoUp, m_rhoDown, m_maxOptionPosition, m_underlyToBeginIndex,
                                m_skewDirection, m_skewPosition, m_signalPrice, m_callOptionStrike, m_putOptionStrike ,m_configIndex);
                spdlog::info( "[{}_{}] start, underlyInstrument={}, kperiod={}, MV={:.3f}, multi={:.3f}, tradingDay={}, expireDay={}, "
                "openAtDelta={:.3f}, length={}, upBand={:.3f}, downBand={:.3f}, maxOptionPosition={}, "
                "underlyToBeginIndex={}, skewDirection={}, skewPosition={}, signalPrice={:.3f}, holdStrikePrice={:.3f}, configIndex={}\n",
                m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(), static_cast<int>(m_kperiod), m_MV, m_multi,
                m_tradingDay,m_expireDay,m_openAtDelta, m_rhoLength, m_rhoUp, m_rhoDown, m_maxOptionPosition,
                m_underlyToBeginIndex,  m_skewDirection, m_skewPosition, m_signalPrice, m_callOptionStrike, m_putOptionStrike ,
                m_configIndex);
                m_configIndex++;
            };

            virtual void runTick(const  Types::MarketData *pMD) override {

                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {
                    if (m_lastUnderlyBarIndex==0 && m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                     //   auto lastUnderlyBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];

                    //    _writePolicyLog(lastUnderlyBar, pMD);
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                    }
                    else if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {  //waiting all instrtuments finish KData
                        auto lastUnderlyBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];
                        m_lastOptionIndex = m_underlyKseries->m_seriesIndex - 1 - m_underlyToBeginIndex;
                        if (strcmp(lastUnderlyBar->m_updateTimeBegin.data(),"04:54:40") ==0 && m_tradingDay == 20260716) {
                            int a = 1;
                        }
                        _updateVannaSignal(lastUnderlyBar);
                        if(m_tradingDay  != m_expireDay){
                            if (m_skewDirection > 0 &&  m_skewPosition <=0) {
                                 // close last Position and sellPut buyCall
                                _closeVannaPosition(lastUnderlyBar);
                                _openVannaPosition(lastUnderlyBar, m_skewDirection);
                                m_skewPosition = 1;
                            }else if (m_skewDirection < 0 && m_skewPosition >=0) {
                                // close LastPosition and sellCall buyPut
                                _closeVannaPosition(lastUnderlyBar);
                                _openVannaPosition(lastUnderlyBar, m_skewDirection);
                                m_skewPosition = -1;
                            }else if (m_skewPosition !=0 && (std::max(m_callOptionStrike, m_putOptionStrike) < lastUnderlyBar->m_close ||
                                    std::min(m_callOptionStrike, m_putOptionStrike) > lastUnderlyBar->m_close)) {

                                //close old position and open new
                                _closeVannaPosition(lastUnderlyBar);
                                _openVannaPosition(lastUnderlyBar, m_skewDirection);
                            }

                            if ( (m_skewPosition > 0 && lastUnderlyBar->m_sabrPRMT.rho >0.1) ||
                                (m_skewPosition < 0 && lastUnderlyBar->m_sabrPRMT.rho < -0.1) ) {
                                 //close all
                                _closeVannaPosition(lastUnderlyBar);
                                m_skewPosition=0;
                            }

                          _checkMaxPositionRisk(m_callPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPosition);
                          _checkMaxPositionRisk(m_putPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPosition);
                        }
                        _writePolicyLog(lastUnderlyBar, pMD);
                        m_configLog->flush();
                        m_configIndex++;
                        m_isCheck = false;
                    //    m_preMarketPosition = m_marketPosition;
                    }
                    m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                }

            };

            void _writePolicyLog(const KData::KData *lastUnderlyKB, const Types::MarketData * pMD) {

                m_configLog->info("configIndex={}, instr={}, {}, {}, {}, {:03d}, close={:.3f}, rho={:.5f}, rhoUp={:.5f}, rhoDown={:.5f}, "
                                  "skewDirct={}, skewPos={}, sgnPrice={:.3f}, "
                                  "callStkPrice={:.3f}, putStkPrice={:.3f}, seriesIndex={}",
                                  m_configIndex, lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingDay,
                                  lastUnderlyKB->m_updateTimeBegin.data(), pMD->updateTime.data(), pMD->milliSeconds, lastUnderlyKB->m_close,
                                  lastUnderlyKB->m_sabrPRMT.rho, m_rhoUp, m_rhoDown, m_skewDirection, m_skewPosition,
                                  m_signalPrice, m_callOptionStrike, m_putOptionStrike, m_lastUnderlyBarIndex);
                _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);
            }


            void _closeVannaPosition(const KData::KData *lastUnderlyBar) {
                _setTargetAllTargetPosZero(m_callPolicySymbols.targetSignal.targetPosMaps);
                _setTargetAllTargetPosZero(m_putPolicySymbols.targetSignal.targetPosMaps);
                m_callOptionStrike = 0.0;
                m_putOptionStrike = 0.0;
            }

            void _openVannaPosition(const KData::KData *lastUnderlyBar, int skewDirection) {
                double targetDelta=  m_MV * 10000 / (lastUnderlyBar->m_close * m_multi);
                if (skewDirection >0) {
                    _setOpenPostion(m_callPolicySymbols, targetDelta, 'C',lastUnderlyBar->m_close);
                    _setOpenPostion(m_putPolicySymbols, targetDelta,  'P',lastUnderlyBar->m_close);
                }else if (skewDirection <0) {
                    _setOpenPostion(m_callPolicySymbols, -targetDelta, 'C',lastUnderlyBar->m_close);
                    _setOpenPostion(m_putPolicySymbols,  -targetDelta,  'P',lastUnderlyBar->m_close);
                }
            }



            void _setOpenPostion(PolicySymbolStruct & policySymbols, double targetDelta, char optionType, double underlyClose) {
            //    auto optionKBarIndex = m_underlyKseries->m_seriesIndex - 1 - m_underlyToBeginIndex;

                auto openAtSymbol = getApproxiDeltaSymbol(policySymbols.optionSymbolVecs, m_openAtDelta, optionType, underlyClose);
                if(openAtSymbol != nullptr){
                    auto openAtSeries = openAtSymbol->m_kSeriesMap.at(m_kperiod);
                    auto symbolDelta = (openAtSeries->m_KDataVecs[m_lastOptionIndex])->m_greeks.delta;

                    addPositionByGreeks(openAtSymbol->instrumentInfo.instrumentID, policySymbols.targetSignal.targetPosMaps,
                                        symbolDelta, targetDelta);

                    if (optionType =='C') {
                        m_callOptionStrike = openAtSymbol->instrumentInfo.strikePrice;
                    }else if (optionType =='P') {
                        m_putOptionStrike = openAtSymbol->instrumentInfo.strikePrice;
                    }


                }else {
                    fprintf(stderr,"_setOpenPostion openAtNull %s, symbolLength=%d\n", m_engineName.c_str(), policySymbols.optionSymbolVecs.size());
                         for (auto symbolItr: policySymbols.optionSymbolVecs) {
                                fprintf(stderr,"symbolItr  instrumentID=%s, m_seriesIndex=%d, delta=%.3f \n", symbolItr->m_kSeriesMap.at(m_kperiod)->m_insInfo.instrumentID.data(),
					symbolItr->m_kSeriesMap.at(m_kperiod)->m_seriesIndex, symbolItr->m_kSeriesMap.at(m_kperiod)->m_lastDelta);
                         }
                }
            }

            void _setTargetAllTargetPosZero(decltype(m_callPolicySymbols.targetSignal.targetPosMaps) & optionTargetPosMaps){
                for (auto itr = optionTargetPosMaps.begin(); itr != optionTargetPosMaps.end(); itr++) {
                    itr->second = 0;
                }
            }

        };
    }
}

#endif //COSMOS_VANNASABR_H
