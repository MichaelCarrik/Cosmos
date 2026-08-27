//
// Created by zhangyingwei on 2024/10/29.
//

#ifndef OPTIONTRADING_LONGGAMMAGOD_H
#define OPTIONTRADING_LONGGAMMAGOD_H

#include "IOptionPolicy.h"
#include "../Types/Type.h"

namespace Cosmos {
    namespace Policy {



        class OptionGod : public IOptionPolicy {
      private:
            int m_configIndex{0};
            bool m_isCheck{false};

            Types::Instrument_t m_optionInstrumentA{""};
            Types::Instrument_t m_optionInstrumentB{""};
            int m_optionTargetPosA{0};
            int m_optionTargetPosB{0};

            int m_lastPsTime{0};
            int m_maxOptionPosition{0};
            int m_tradeNum{0};
            int m_marketPosition{0};
            int m_preMarketPosition{0};
            double m_holdStrikePrice{0.0};
            double m_signalPrice{0.0};

        public:
            OptionGod( Types::KPeriod kperiod,  std::string const& policyName,
                         std::string &engineName,
                         Types::Instrument_t &underlyInstrument, Types::Instrument_t & optionInstrumentA, Types::Instrument_t& optionInstrumentB,
                         int optionTargetPosA, int optionTargetPosB, double MV, double multi, int tradingDay, int expireDay, int maxOptionPosition,
                        decltype(m_getUnderlyToBeginIndexFunc) getUnderlyToBeginIndexFunc) : IOptionPolicy(policyName, engineName, underlyInstrument,
                                                kperiod, MV, multi,  tradingDay, expireDay, getUnderlyToBeginIndexFunc),
                     m_maxOptionPosition(maxOptionPosition), m_optionInstrumentA(optionInstrumentA), m_optionInstrumentB(optionInstrumentB),
                     m_optionTargetPosA(optionTargetPosA), m_optionTargetPosB(optionTargetPosB){


                _initPolicyLogger();
            }

            ~OptionGod() {}

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
                            char insname[56]{"instrument"};
                            _getValueInLine(buf, insname, fileRead.instrumentStr);
                            char tagname[56]{"targetPosition"};
                            _getValueInLine(buf, tagname, fileRead.targetPositionStr);

                            if(strcmp(fileRead.instrumentStr.c_str(), underlyInstrument.data())==0){
                                char mpname[56]{"marketPosition"};
                                _getValueInLine(buf, mpname, fileRead.marketPositionStr);

                                char preMpname[56]{"preMarketPosition"};
                                _getValueInLine(buf, preMpname, fileRead.preMarketPositionStr);

                                char sgnname[56]{"signalPrice"};
                                _getValueInLine(buf, sgnname, fileRead.signalPriceStr);

                                char stpname[56]{"holdStrikePrice"};
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
                        "optionInstrumentA=%s, optionTargetLotsA=%d, optionInstrumentB=%s, optionTargetLotsB=%d,"
                        "expireday=%d, marketPosition=%d, preMarketPosition=%d, signalPrice=%.3f, holdStrikePrice=%.3f, "
                        "configIndex=%d\n", m_engineName.c_str(), m_policyName.c_str(), static_cast<int>(m_kperiod),
                        m_MV, m_multi, m_maxOptionPosition, m_optionInstrumentA.data(), m_optionTargetLotsA,
                        m_optionInstrumentB.data(),  m_optionTargetLotsB, m_expireDay, m_marketPosition,
                        m_preMarketPosition,m_signalPrice, m_holdStrikePrice, m_configIndex);
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


                        _setTargetAllTargetPosZero(m_callPolicySymbols.targetSignal.targetPosMaps);
                        _setTargetAllTargetPosZero(m_putPolicySymbols.targetSignal.targetPosMaps);

                        _setGodOptionTarget(m_callPolicySymbols.targetSignal.targetPosMaps, m_optionInstrumentA, m_optionTargetPosA);
                        _setGodOptionTarget(m_callPolicySymbols.targetSignal.targetPosMaps, m_optionInstrumentB, m_optionTargetPosB);
                        _setGodOptionTarget(m_putPolicySymbols.targetSignal.targetPosMaps, m_optionInstrumentA, m_optionTargetPosA);
                        _setGodOptionTarget(m_putPolicySymbols.targetSignal.targetPosMaps, m_optionInstrumentB, m_optionTargetPosB);

                        _checkMaxPositionRisk(m_callPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPosition);
                        _checkMaxPositionRisk(m_putPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPosition);




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

                m_configLog->info("configIndex={}, instrument={}, {}, {}, {}, close={:.3f}, "
                                  "marketPosition={}, preMarketPosition={}, signalPrice={:.3f}, "
                                  "holdStrikePrice={:.3f}",
                                  m_configIndex, lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingDay,
                                  lastUnderlyKB->m_updateTimeBegin.data(), lastUnderlyKB->m_endPsTime, lastUnderlyKB->m_close,
                                  m_marketPosition, m_preMarketPosition, m_signalPrice, m_holdStrikePrice);
                _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);
            }





            void _setTargetAllTargetPosZero(decltype(m_callPolicySymbols.targetSignal.targetPosMaps) & optionTargetPosMaps){
                for (auto itr = optionTargetPosMaps.begin(); itr != optionTargetPosMaps.end(); itr++) {
                    itr->second = 0;
                }
            }
            int _setGodOptionTarget(decltype(m_callPolicySymbols.targetSignal.targetPosMaps) & targetPosMaps, Types::Instrument_t const& optionInstrumentID, int targetLots){
                auto itrTGPos = targetPosMaps.find(optionInstrumentID);
                if (itrTGPos == targetPosMaps.end()) {
                    targetPosMaps[optionInstrumentID] = 0;
                    itrTGPos = targetPosMaps.find(optionInstrumentID);
                }
                int preT =     itrTGPos->second;
                if (preT != targetLots) {
                     fprintf(stderr, "setGodOptionTarget optionInstrumentID=%s, diffGreeks=%.3f, preT=%d, greeks=%.3f,targetLots=%d, targetPos=%d\n",
                        optionInstrumentID.data(),  preT, targetLots, itrTGPos->second);
                }
                return 1;
            }


        };
    }
}

#endif //OPTIONTRADING_LONGGAMMAGOD_H
