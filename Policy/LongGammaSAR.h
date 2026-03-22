//
// Created by zhangyingwei on 2024/10/29.
//

#ifndef OPTIONTRADING_LONGGAMMASAR_H
#define OPTIONTRADING_LONGGAMMASAR_H

#include "IOptionPolicy.h"
#include "../Types/Type.h"

namespace Cosmos {
    namespace Policy {
        struct SARStruct {
            int trend{0};
            double sar{0.0};
            double realSAR{0.0};
            double ep{0.0};
            double af{0.0};
            double high{0.0};
            double low{0.0};
            double close{0.0};
        };

        template<typename T>
        static T Sum(std::vector<T> const &inputVec, int begin, int end) {
            T sumValue = 0;
            for (int i = begin; i <= end; i++) {
                sumValue = sumValue + inputVec[i];
            }
            return sumValue;
        }

        template<typename T>
        static T MA(std::vector<T> const &inputVec, int begin, int end) {
            auto sumValue = Sum(inputVec, begin, end);
            return sumValue / (end - begin + 1);
        }

        class LongGammaSAR : public IOptionPolicy {
        private:
            int m_configIndex{0};
            bool m_isCheck{false};
            double m_mv{0.0};
            double m_multi{1000.0};
            int m_lastPsTime{0};
            int m_maxPosition{0};
            double m_openAtDelta{0.25};
            double m_initial_af{0.02};
            double m_step_af{0.02};
            double m_end_af{0.2};
            int m_length{1000};
            double m_minsMA{0.0};
            int m_tradeNum{0};
            int m_marketPosition{0};
            int m_preMarketPosition{0};
            double m_holdStrikePrice{0.0};
            double m_signalPrice{0.0};

            std::vector<SARStruct> m_mySARVec;
            std::vector<double> m_minCloseVec;


        public:
            LongGammaSAR( Types::KPeriod kperiod, double mv, std::string const& policyName,
                         std::string &engineName,
                         Types::Instrument_t &instrument, double multi, int tradingday, int maxPosition,
                         double openAtDelta) :
                    m_mv(mv),
                    m_multi(multi), m_maxPosition(maxPosition),
                    m_openAtDelta(openAtDelta) {

                m_underlyInstrument = instrument;

                if (m_openAtDelta < 0.1 || m_openAtDelta > 0.5) {
                    assert(false);
                }
                m_kperiod = kperiod;
                m_policyName = policyName;
                m_engineName = engineName;
                m_tradingday = tradingday;
                m_length = 1000 *5 /30;
                spdlog::info("createPolicy engineName={}, policyName={}, kperiod={}  mv={}, tradingday={}, "
                             "underly={}, , maxPosition={}, openAtDelta={}",
                             engineName, policyName, (int) kperiod, m_mv, m_tradingday, m_underlyInstrument.data(),
                             m_maxPosition, m_openAtDelta);
                _initPolicyLogger();
            }

            ~LongGammaSAR() {

            }

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

            void parabolicSAR(const KData::KData *kD) {
                if (m_mySARVec.size() == 0) {
                    SARStruct sarStruct;
                    sarStruct.high = kD->m_high;
                    sarStruct.low = kD->m_low;
                    sarStruct.close = kD->m_close;
                    m_mySARVec.emplace_back(sarStruct);
                } else if (m_mySARVec.size() == 1) {
                    SARStruct sarStruct;
                    const SARStruct *lastSAR = &(m_mySARVec[m_mySARVec.size() - 1]);
                    sarStruct.trend = kD->m_close > lastSAR->close ? 1 : -1;
                    sarStruct.sar = sarStruct.trend > 0 ? lastSAR->high : lastSAR->low;
                    sarStruct.realSAR = sarStruct.sar;
                    sarStruct.ep = sarStruct.trend > 0 ? kD->m_high : kD->m_low;
                    sarStruct.af = m_initial_af;
                    sarStruct.high = kD->m_high;
                    sarStruct.low = kD->m_low;
                    sarStruct.close = kD->m_close;
                    m_mySARVec.emplace_back(sarStruct);
                } else {
                    SARStruct sarStruct;
                    sarStruct.high = kD->m_high;
                    sarStruct.low = kD->m_low;
                    sarStruct.close = kD->m_close;
                    const SARStruct *lastSAR = &(m_mySARVec[m_mySARVec.size() - 1]);
                    const SARStruct *lastLastSAR = &(m_mySARVec[m_mySARVec.size() - 2]);
                    double temp = lastSAR->sar + lastSAR->af * (lastSAR->ep - lastSAR->sar);
                    if (lastSAR->trend < 0) {
                        sarStruct.sar = std::max(temp, std::max(lastSAR->high, lastLastSAR->high));
                        temp = sarStruct.sar < sarStruct.high ? 1 : lastSAR->trend - 1;
                    } else {
                        sarStruct.sar = std::min(temp, std::min(lastSAR->low, lastLastSAR->low));
                        temp = sarStruct.sar > sarStruct.low ? -1 : lastSAR->trend + 1;
                    }
                    sarStruct.trend = temp;

                    if (sarStruct.trend < 0) {
                        temp = sarStruct.trend != -1 ? std::min(sarStruct.low, lastSAR->ep) : sarStruct.low;
                    } else {
                        temp = sarStruct.trend != 1 ? std::max(sarStruct.high, lastSAR->ep) : sarStruct.high;
                    }
                    sarStruct.ep = temp;

                    if (std::abs(sarStruct.trend) == 1) {
                        temp = lastSAR->ep;
                        sarStruct.af = m_initial_af;
                    } else {
                        temp = sarStruct.sar;
                        if (sarStruct.ep == lastSAR->ep) {
                            sarStruct.af = lastSAR->af;
                        } else {
                            sarStruct.af = std::min(m_end_af, lastSAR->af + m_step_af);
                        }
                    }
                    sarStruct.realSAR = temp;
//                    fprintf(stderr, "sar %d %s realSar=%.3f, trend=%d\n",kD->m_tradingday, kD->m_updateTime.data(),
//                            sarStruct.realSAR, sarStruct.trend);
                    m_mySARVec.emplace_back(sarStruct);
                }
            }

            void initIndicator() {
                for (auto i = 0; i < m_underlyKseries->m_seriesIndex; i++) {
                    auto bar = m_underlyKseries->m_KDataVecs[i];
                    parabolicSAR(bar);
                }

                for (auto i = 0; i < m_underlyKseries->m_seriesIndex; i++) {
//                    fprintf(stderr, "%s, %d, %s, close=%.3f, read_sar=%.3f\n", m_underlyKseries->m_KDataVecs[i]->m_instrument.data(),
//                            m_underlyKseries->m_KDataVecs[i]->m_tradingday,    m_underlyKseries->m_KDataVecs[i]->m_updateTime.data(),
//                            m_underlyKseries->m_KDataVecs[i]->m_close,  m_mySARVec[i].realSAR);
                    m_minCloseVec.emplace_back(m_underlyKseries->m_KDataVecs[i]->m_close);
                }

                m_minsMA = _getMean();

                if (m_underlyKseries->m_seriesIndex > 0) {
                    auto lastUnderlyKB = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex - 1];
                    auto barIndex = m_underlyKseries->m_seriesIndex - 1;
                    const SARStruct *sarStruct = &m_mySARVec[barIndex];
                //    _writePolicyLog(lastUnderlyKB, sarStruct);
                }
            }

            virtual void
            start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) override {
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());
                m_configIndex = atoi(this->GetLastValueFromFile(configPath, "configIndex").c_str());
                std::vector<FileRead> fileReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_underlyInstrument, m_configIndex, fileReadVecs);

                m_expireday = _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_callPolicySymbols, fileReadVecs, 'C' );
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
                    }
                }
                m_configIndex++;
                initIndicator();
                auto lastUnderlyBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];
                auto lastSAR = &(m_mySARVec[m_mySARVec.size() - 1]);

            //    _writePolicyLog(lastUnderlyBar,lastSAR);

                fprintf(stderr,
                        "[%s_%s] start, m_kperiod=%dMins, m_mv=%.3f, m_multi=%.3f, maxPosition=%d, "
                        "openAtDelta=%.3f, expireday=%d, marketPosition=%d, preMarketPostion=%d, signalPrice=%.3f, holdStrikePrice=%.3f, "
                        "configIndex=%d\n", m_engineName.c_str(), m_policyName.c_str(), Types::KPeroidToIntervalMap[m_kperiod], m_mv, m_multi,
                        m_maxPosition, m_openAtDelta, m_expireday, m_marketPosition, m_preMarketPosition,
                        m_signalPrice, m_holdStrikePrice ,m_configIndex);
                m_configIndex++;
            };

            virtual void runTick(const  Types::MarketData *pMD) override {

                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {

                    if (m_lastUnderlyBarIndex <
                               m_underlyKseries->m_seriesIndex) {  //waiting all instrtuments finish KData
                        m_isCheck = true;
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                        m_lastPsTime = pMD->psSecond;
                    } else if (m_isCheck == true && pMD->psSecond - m_lastPsTime > 10 && m_lastUnderlyBarIndex > 0 ) {
                        auto lastUnderlyBar = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];
                        m_minCloseVec.emplace_back(lastUnderlyBar->m_close);
                        m_minsMA = _getMean();

                        parabolicSAR(lastUnderlyBar);

                        auto lastSAR = &(m_mySARVec[m_mySARVec.size() - 1]);

                        if(m_tradingday  != m_expireday){
                            _isCloseMarketPosition(lastUnderlyBar, lastSAR);
                            _marketPosToOptionPos(lastUnderlyBar, m_marketPosition, m_preMarketPosition);
                            m_preMarketPosition = m_marketPosition;

                            _isOpenMarketPosition(lastUnderlyBar, lastSAR);
                            _marketPosToOptionPos(lastUnderlyBar, m_marketPosition, m_preMarketPosition);
                            m_preMarketPosition = m_marketPosition;

                            _checkMaxPositionRisk(m_callPolicySymbols.optionTargetPosMaps, 1, m_maxPosition);
                            _checkMaxPositionRisk(m_putPolicySymbols.optionTargetPosMaps, 1, m_maxPosition);

                        }

                        _writePolicyLog(lastUnderlyBar, lastSAR);
                        m_configLog->flush();
                        m_configIndex++;
                        m_isCheck = false;
                        m_preMarketPosition = m_marketPosition;
                    }
                }
            };

            void _writePolicyLog(const KData::KData *lastUnderlyKB, const SARStruct *sarStruct) {

                m_configLog->info("configIndex={}, instrument={}, {}, {}, {}, close={:.3f}, trend={}, "
                                  //"sar={:.3f}, realSAR={:.3f}, ep={:.3f}, af={:.3f}, " 
                                  "high={:.3f}, low={:.3f}, "
                                  "minsMA={:.3f}, marketPosition={}, preMarketPosition={}, signalPrice={:.3f}, "
                                  "holdStrikePrice={:.3f}",
                                  m_configIndex, lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingday,
                                  lastUnderlyKB->m_updateTime.data(), lastUnderlyKB->m_psTime, lastUnderlyKB->m_close,
                                  sarStruct->trend, // sarStruct->sar, sarStruct->realSAR, sarStruct->ep,  sarStruct->af, 
                                  sarStruct->high, sarStruct->low, m_minsMA, m_marketPosition,
                                  m_preMarketPosition, m_signalPrice, m_holdStrikePrice);

                _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);
            }



            double _getMean() {
                int beginI = 0, endI = 0;
                if (m_underlyKseries->m_seriesIndex < m_length) {
                    beginI = 0;
                    endI = m_underlyKseries->m_seriesIndex;
                } else {
                    beginI = m_underlyKseries->m_seriesIndex - m_length;
                    endI = m_underlyKseries->m_seriesIndex;
                }
                return MA(m_minCloseVec, beginI, endI);
            }

            void _isCloseMarketPosition(const KData::KData *lastUnderlyBar, const SARStruct *lastSARBar) {
                if (m_marketPosition > 0 && lastUnderlyBar->m_close >= m_holdStrikePrice) {  //long close
                    ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_holdStrikePrice = 0.0;
                } else if (m_marketPosition < 0 && lastUnderlyBar->m_close <= m_holdStrikePrice) {  //short close
                    ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_holdStrikePrice = 0.0;
                }
            }

            void _isOpenMarketPosition(const KData::KData *lastUnderlyBar, const SARStruct *lastSARBar) {


                //if (m_tradeNum < 10 && m_marketPosition == 0) {
                if (m_tradeNum < 10 ) {
                    if (lastSARBar->realSAR < lastUnderlyBar->m_close && m_marketPosition != 1 &&
                        lastUnderlyBar->m_close > m_minsMA) //long open
                    {
                        ++m_tradeNum;
                        m_marketPosition = 1;
                        m_signalPrice = lastUnderlyBar->m_close;
                    } else if (lastSARBar->realSAR > lastUnderlyBar->m_close && m_marketPosition != -1 &&
                               lastUnderlyBar->m_close < m_minsMA) //short open
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

                    _setTargetAllTargetPosZero(m_putPolicySymbols.optionTargetPosMaps);

                    double targetDelta=  m_mv / (lastUnderlyBar->m_close * m_multi);
                    _setOpenPostion(m_callPolicySymbols, targetDelta, 'C',lastUnderlyBar->m_close);
                } else if (marketPosition == -1 && preMarketPosition != marketPosition) {

                    _setTargetAllTargetPosZero(m_callPolicySymbols.optionTargetPosMaps);

                    double targetDelta=  -m_mv / (lastUnderlyBar->m_close * m_multi);
                    _setOpenPostion(m_putPolicySymbols, targetDelta,  'P',lastUnderlyBar->m_close);
                } else if (marketPosition == 0 && preMarketPosition != marketPosition) {// minus delta

                    _setTargetAllTargetPosZero(m_callPolicySymbols.optionTargetPosMaps);
                    _setTargetAllTargetPosZero(m_putPolicySymbols.optionTargetPosMaps);

                }
            }

            void _setOpenPostion(PolicySymbolStruct & policySymbols, double targetDelta, char optionType, double underlyClose) {

                auto openAtSymbol = getApproxiDeltaSymbol(policySymbols.optionSymbolVecs, m_openAtDelta, optionType, underlyClose);
                if(openAtSymbol != nullptr){
                    auto openAtSeries = openAtSymbol->m_kSeriesMap.at(m_kperiod);
                    auto symbolDelta = (openAtSeries->m_KDataVecs[openAtSeries->m_seriesIndex - 1])->delta;

                    addPositionByGreeks(openAtSymbol->instrumentInfo.instrumentID, policySymbols.optionTargetPosMaps,
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

            void _setTargetAllTargetPosZero(decltype(m_callPolicySymbols.optionTargetPosMaps) & optionTargetPosMaps){
                for (auto itr = optionTargetPosMaps.begin(); itr != optionTargetPosMaps.end(); itr++) {
                    itr->second = 0;
                }
            }

        };
    }
}

#endif //OPTIONTRADING_LONGGAMMASAR_H
