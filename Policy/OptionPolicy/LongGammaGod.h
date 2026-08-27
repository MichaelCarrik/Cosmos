//
// Created by zhangyingwei on 2024/10/29.
//

#ifndef OPTIONTRADING_LONGGAMMAGOD_H
#define OPTIONTRADING_LONGGAMMAGOD_H

#include "IOptionPolicy.h"
#include "../Types/Type.h"

namespace Cosmos {
    namespace Policy {

        class LongGammaGod : public IOptionPolicy {
        private:
            int m_configIndex{0};
            bool m_isCheck{false};

            int m_lastPsTime{0};
            int m_maxOptionPosition{0};
            int m_godDirection{0};
            double m_closeExceedThresh{0.0};
            double m_openAtDelta{0.25};
            int m_isRefresh{0};
            int m_tradeNum{0};
            int m_marketPosition{0};
            int m_preMarketPosition{0};
            double m_holdStrikePrice{0.0};
            double m_signalPrice{0.0};

        public:
            LongGammaGod( Types::KPeriod kperiod,  std::string const& policyName,
                         std::string &engineName,
                         Types::Instrument_t &instrument, double MV, double multi, int tradingDay, int expireDay, int maxOptionPosition , int isRefresh,
                         double openAtDelta, int godDirection, double closeExceedThresh, decltype(m_getUnderlyToBeginIndexFunc) getUnderlyToBeginIndexFunc) : IOptionPolicy(policyName, engineName, instrument,
                                                kperiod, MV, multi,  tradingDay, expireDay, getUnderlyToBeginIndexFunc),
                     m_openAtDelta(openAtDelta), m_maxOptionPosition(maxOptionPosition), m_godDirection(godDirection), m_closeExceedThresh(closeExceedThresh), m_isRefresh(isRefresh) {

                m_underlyInstrument = instrument;

                if (m_openAtDelta < 0.1 || m_openAtDelta > 0.5) {
                    assert(false);
                }
                // m_kperiod = kperiod;
                // m_policyName = policyName;
                // m_engineName = engineName;
                // spdlog::info("createPolicy engineName={}, policyName={}, kperiod={}  mv={}, tradingday={}, "
                //              "underly={}, , maxPosition={}, openAtDelta={}, godDirection={}, closeExceedThresh={}",
                //              engineName, policyName, (int) kperiod, m_MV, m_tradingDay, m_underlyInstrument.data(),
                //              m_maxPosition, m_openAtDelta, m_godDirection, m_closeExceedThresh);
                _initPolicyLogger();
            }

            ~LongGammaGod() {}

     void _GetValueFromFileByConfigIndex(char *filename, Types::Instrument_t const& underlyInstrument,
                                                int inputConfigIndex, std::vector<FileRead> &fileReadVecs) {
                char buf[BUFSIZ], *field;

                FILE *fp = NULL;
                fp = fopen(filename, "r");
                FileRead fileRead;
                if (fp == NULL) {
                    printf("OnStarted, Warning: Cannot open file: %s !!!\n", filename);
                    return;
                } else {
                    while (fgets(buf, BUFSIZ, fp) != NULL) {
                        std::string configIndexStr{""};
                        char ciname[56]{"configIndex"};
                        _getValueInLine(buf, ciname, configIndexStr);
                        if (std::stoi(configIndexStr.c_str()) == inputConfigIndex) {
                            FileRead fileRead;
                            char insname[56]{"instr"};
                            _getValueInLine(buf, insname, fileRead.instrumentStr);
                            char tagname[56]{"targetPos"};
                            _getValueInLine(buf, tagname, fileRead.targetPositionStr);

                            if(strcmp(fileRead.instrumentStr.c_str(), underlyInstrument.data())==0){
                                char mpname[56]{"mktPos"};
                                _getValueInLine(buf, mpname, fileRead.marketPositionStr);

                                char preMpname[56]{"preMktPos"};
                                _getValueInLine(buf, preMpname, fileRead.preMarketPositionStr);

                                char sgnname[56]{"sgnPrice"};
                                _getValueInLine(buf, sgnname, fileRead.signalPriceStr);

                                char stpname[56]{"strikePrice"};
                                _getValueInLine(buf, stpname, fileRead.holdStrikePriceStr);

                            }
                            fileReadVecs.emplace_back(fileRead);
                        }
                    }
                }
                fclose(fp);
            }


            void initIndicator() {

            }

            virtual void start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) override {
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());
                m_configIndex = atoi(this->GetLastValueFromFile(configPath, "configIndex").c_str());
                std::vector<FileRead> fileReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_underlyInstrument, m_configIndex, fileReadVecs);
                for(auto fileReadItr : fileReadVecs) {
                    fprintf(stderr, "LongGammaGod %s, targetPos=%s\n", fileReadItr.instrumentStr.c_str(), fileReadItr.targetPositionStr.c_str());
                }


                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_callPolicySymbols, fileReadVecs, 'C' );
                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_putPolicySymbols, fileReadVecs, 'P' );

                auto symbolItr = inputSymbolMap.find(m_underlyInstrument);
                if(symbolItr == inputSymbolMap.end()){
                    assert(false);
                }
                m_underlyKseries = symbolItr->second->m_kSeriesMap.at(m_kperiod);
                m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;

                for(auto fileReadItr : fileReadVecs){
                    if(strcmp(fileReadItr.instrumentStr.c_str(), symbolItr->second->instrumentInfo.instrumentID.data())==0){
                        m_marketPosition = std::stoi(fileReadItr.marketPositionStr.c_str());
                        m_preMarketPosition = std::stoi(fileReadItr.preMarketPositionStr.c_str());
                        m_signalPrice = std::stof(fileReadItr.signalPriceStr.c_str());
                        m_holdStrikePrice = std::stof(fileReadItr.holdStrikePriceStr.c_str());

                  //      fprintf(stderr, "%s, %.3f, %s\n",m_engineName.c_str(), m_holdStrikePrice, fileReadItr.holdStrikePriceStr.c_str());
                    }
                }
                m_configIndex++;
                initIndicator();
                auto lastUnderlyBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];

            //    _writePolicyLog(lastUnderlyBar,lastSAR);

                fprintf(stderr,
                        "[%s_%s] start, m_kperiod=%d, m_MV=%.3f, m_multi=%.3f, m_maxOptionPosition=%d, "
                        "openAtDelta=%.3f, godDirection=%d, closeExceedThresh=%.3f, expireday=%d, "
                        "marketPosition=%d, preMarketPosition=%d, signalPrice=%.3f, holdStrikePrice=%.3f, isRefresh=%d, "
                        "configIndex=%d\n", m_engineName.c_str(), m_policyName.c_str(), static_cast<int>(m_kperiod),
                        m_MV, m_multi, m_maxOptionPosition, m_openAtDelta, m_godDirection, m_closeExceedThresh, m_expireDay, m_marketPosition,
                        m_preMarketPosition,m_signalPrice, m_holdStrikePrice, m_isRefresh, m_configIndex);
                m_configIndex++;
            };

            virtual void runTick(const  Types::MarketData *pMD) override {

                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {

                    if (m_lastUnderlyBarIndex==0 && m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        //   auto lastUnderlyBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];

                        //    _writePolicyLog(lastUnderlyBar, pMD);
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                    }        
					 else if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        auto lastUnderlyBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];
                        m_lastOptionIndex = m_underlyKseries->m_seriesIndex - 1 - m_underlyToBeginIndex;

                        if(m_tradingDay  != m_expireDay){

                         //   fprintf(stderr, "%s, 1 m_isRefresh=%d\n",m_engineName.c_str(), m_isRefresh);
                            if (m_isRefresh == 0) {
                                _isCloseMarketPosition(lastUnderlyBar);
                                _marketPosToOptionPos(lastUnderlyBar, m_marketPosition, m_preMarketPosition);
                                m_preMarketPosition = m_marketPosition;

                                _isOpenMarketPosition(lastUnderlyBar);
                                _marketPosToOptionPos(lastUnderlyBar, m_marketPosition, m_preMarketPosition);
                                m_preMarketPosition = m_marketPosition;

                                _checkMaxPositionRisk(m_callPolicySymbols.targetSignal.targetPosMaps, 1, m_maxOptionPosition);
                                _checkMaxPositionRisk(m_putPolicySymbols.targetSignal.targetPosMaps, 1, m_maxOptionPosition);
                            }else if(m_isRefresh == 1) {
                                m_preMarketPosition = 0;
                           //     fprintf(stderr, "%s, 2 m_isRefresh=%d, preMarketPos=%d, marketPos=%d\n",m_engineName.c_str(), m_isRefresh,
                          //          m_preMarketPosition, m_marketPosition);
                                _marketPosToOptionPos(lastUnderlyBar, m_marketPosition, m_preMarketPosition);
                                m_preMarketPosition = m_marketPosition;
                                m_isRefresh = 0;
                            }

                        }

                        _writePolicyLog(lastUnderlyBar);
                        m_configLog->flush();
                        m_configIndex++;
                        m_isCheck = false;
                  //      m_preMarketPosition = m_marketPosition;
                    }
                    m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                }
            };

            virtual void updateParam(const Types::NetModifyParam *netModifyParam) override {};

            void _writePolicyLog(const KData::KData *lastUnderlyKB) {

                m_configLog->info("configIndex={}, instr={}, {}, {}, {}, close={:.3f}, "
                                  "mktPos={}, preMktPos={}, sgnPrice={:.3f}, "
                                  "strikePrice={:.3f}, godDirection={}, closeExceedThresh={:.3f}",
                                  m_configIndex, lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingDay,
                                  lastUnderlyKB->m_updateTimeBegin.data(), lastUnderlyKB->m_endPsTime, lastUnderlyKB->m_close,
                                  m_marketPosition, m_preMarketPosition, m_signalPrice, m_holdStrikePrice, m_godDirection,
                                  m_closeExceedThresh);
                _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);
            }


            void _isCloseMarketPosition(const KData::KData *lastUnderlyBar) {
                if (m_marketPosition > 0 && lastUnderlyBar->m_close >= m_holdStrikePrice + m_closeExceedThresh) {  //long close
                    ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_holdStrikePrice = 0.0;
                } else if (m_marketPosition < 0 && lastUnderlyBar->m_close <= m_holdStrikePrice - m_closeExceedThresh) {  //short close
                    ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_holdStrikePrice = 0.0;
                }
            }

            void _isOpenMarketPosition(const KData::KData *lastUnderlyBar) {


                //if (m_tradeNum < 10 && m_marketPosition == 0) {
                if (m_tradeNum < 10 ) {
                 //   m_marketPosition = m_godDirection;
                    if ( m_marketPosition != 1 and m_godDirection ==1 ) //long open
                    {
                        ++m_tradeNum;
                        m_marketPosition = 1;
                        m_signalPrice = lastUnderlyBar->m_close;
                    } else if ( m_marketPosition != -1 and m_godDirection ==-1 ) //short open
                    {
                        ++m_tradeNum;
                        m_marketPosition = -1;
                        m_signalPrice = lastUnderlyBar->m_close;
                    }
                }
            }

            void _marketPosToOptionPos(const KData::KData* lastUnderlyBar, int marketPosition, int preMarketPosition) {
                if (preMarketPosition == marketPosition) {
                    return;
                } else if (marketPosition == 1 && preMarketPosition != marketPosition) {

                    _setTargetAllTargetPosZero(m_callPolicySymbols.targetSignal.targetPosMaps);
                    _setTargetAllTargetPosZero(m_putPolicySymbols.targetSignal.targetPosMaps);

                    double targetDelta=  m_MV * 10000 / (lastUnderlyBar->m_close * m_multi);
                    _setOpenPostion(m_callPolicySymbols, targetDelta, 'C',lastUnderlyBar->m_close);
                } else if (marketPosition == -1 && preMarketPosition != marketPosition) {

                    _setTargetAllTargetPosZero(m_callPolicySymbols.targetSignal.targetPosMaps);
                    _setTargetAllTargetPosZero(m_putPolicySymbols.targetSignal.targetPosMaps);

                    double targetDelta=  -m_MV * 10000 / (lastUnderlyBar->m_close * m_multi);
                    _setOpenPostion(m_putPolicySymbols, targetDelta,  'P',lastUnderlyBar->m_close);
                } else if (marketPosition == 0 && preMarketPosition != marketPosition) {// minus delta

                    _setTargetAllTargetPosZero(m_callPolicySymbols.targetSignal.targetPosMaps);
                    _setTargetAllTargetPosZero(m_putPolicySymbols.targetSignal.targetPosMaps);

                }
            }

            void _setOpenPostion(PolicySymbolStruct & policySymbols, double targetDelta, char optionType, double underlyClose) {

                auto openAtSymbol = getApproxiDeltaSymbol(policySymbols.optionSymbolVecs, m_openAtDelta, optionType, underlyClose);
                if(openAtSymbol != nullptr){
                    auto openAtSeries = openAtSymbol->m_kSeriesMap.at(m_kperiod);
                    auto symbolDelta = (openAtSeries->m_KDataVecs[m_lastOptionIndex])->m_greeks.delta;

                    addPositionByGreeks(openAtSymbol->instrumentInfo.instrumentID, policySymbols.targetSignal.targetPosMaps,
                                        symbolDelta, targetDelta);
                    m_holdStrikePrice =  openAtSymbol->instrumentInfo.strikePrice;

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

#endif //OPTIONTRADING_LONGGAMMAGOD_H
