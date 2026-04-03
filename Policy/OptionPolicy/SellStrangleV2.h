//
// Created by Zhangyingwei on 2023/5/10.
//

#ifndef OPTIONTRADING_SELLSTRANGLEV2_H
#define OPTIONTRADING_SELLSTRANGLEV2_H
#include "Strangle.h"
#include "../Types/Type.h"

namespace Cosmos {
    namespace Policy {
        class SellStrangleV2 : public Strangle {
        private:
            int m_maxOptionPos{0};
            int m_isRefreshDelta{0};
            double m_openAtDelta{0.25};
            double m_biasRation{0.01};
            double m_basePrice{0.0};

        public:
            SellStrangleV2(std::string const &policyName, std::string const &engineName,
                          Types::Instrument_t &instrument, Types::KPeriod kperiod, double mv, double multi,
                              int tradingDay, int expireDay, int maxOptionPos,
                           int isRefreshDelta, double openAtDelta, double biasRation) : Strangle(policyName, engineName,
                    instrument, kperiod, mv, multi,  tradingDay, expireDay),
                m_maxOptionPos(maxOptionPos), m_isRefreshDelta(isRefreshDelta), m_openAtDelta(openAtDelta),
                m_biasRation(biasRation) {
                if (m_openAtDelta < 0.1 || m_openAtDelta > 0.5) {
                    assert(false);
                }


                _initPolicyLogger();
            }

            ~SellStrangleV2() {
            }


            void _GetValueFromFileByConfigIndex(char *filename, Types::Instrument_t const &underlyInstrument,
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

                            if (strcmp(fileRead.instrumentStr.c_str(), underlyInstrument.data()) == 0) {
                                char stpname[56]{"basePrice"};
                                _getValueInLine(buf, stpname, fileRead.basePriceStr);
                            }
                            fileReadVecs.emplace_back(fileRead);
                        }
                    }
                }
                fclose(fp);
            }

            virtual void updateParam(const Types::NetModifyParam *netModifyParam) override {
                if(strcmp(netModifyParam->paramName.c_str(), "isRefreshDelta") == 0) {
                    m_isRefreshDelta = stoi(netModifyParam->paramValue);
                    fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, isRefreshDelta=%d\n",
                    m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                    m_isRefreshDelta);
                }else if(strcmp(netModifyParam->paramName.c_str(), "openAtDelta") == 0) {
                    m_openAtDelta = stof(netModifyParam->paramValue);
                    fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, openAtDelta=%.3f\n",
                 m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                 m_openAtDelta);
                }else if(strcmp(netModifyParam->paramName.c_str(), "MV") == 0) {
                    m_MV = stof(netModifyParam->paramValue);

                    fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, MV=%.3f\n",
                 m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                 m_MV);
                }
                else if (strcmp(netModifyParam->paramName.c_str(), "targetPos") == 0) {
                    bool isFind =false;
                    auto callItr = m_callPolicySymbols.targetSignal.targetPosMaps.find(netModifyParam->symbolName);
                    if (callItr != m_callPolicySymbols.targetSignal.targetPosMaps.end()) {
                        fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, targetPosition=%s\n",
                                m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                                netModifyParam->paramValue.c_str());
                        m_callPolicySymbols.targetSignal.targetPosMaps[netModifyParam->symbolName] = stoi(netModifyParam->paramValue);
                        isFind = true;

                    }else {
                        auto putItr = m_putPolicySymbols.targetSignal.targetPosMaps.find(netModifyParam->symbolName);
                        if (putItr != m_putPolicySymbols.targetSignal.targetPosMaps.end()) {
                            fprintf(stderr, "updateParam engineName=%s, policyName=%s, instrument=%s, targetPosition=%s\n",
                                    m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data(),
                                    netModifyParam->paramValue.c_str());
                            m_putPolicySymbols.targetSignal.targetPosMaps[netModifyParam->symbolName] = stoi(netModifyParam->paramValue);
                            isFind = true;
                        }

                        if (isFind==true) {
                            auto lastUnderlyKB = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex - 1];

                            double allCallDelta = getAllHoldDelta(m_callPolicySymbols);
                            double allPutDelta = getAllHoldDelta(m_putPolicySymbols);
                              m_configLog->info("configIndex={}, instrument={}, {}, {}, {}, close={:.3f}, delta={:.3f}, "
                                          "targetPosition={}, basePrice={:.3f}, seriesIndex={}, "
                                          "allCallDelta={:.3f}, allPutDelta={:.3f}",
                                          m_configIndex, lastUnderlyKB->m_instrument.data(),
                                          lastUnderlyKB->m_tradingday,
                                          lastUnderlyKB->m_updateTimeBegin.data(), lastUnderlyKB->m_endPsTime,
                                          lastUnderlyKB->m_close,
                                          0.0, 0, m_basePrice, m_underlyKseries->m_seriesIndex - 1, allCallDelta,
                                          allPutDelta);
                        _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                        _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);

                        m_configLog->flush();
                        m_configIndex++;
                        }
                    }
                }
            };

            virtual void start (
                std::unordered_map<Types::Instrument_t, Types::Symbol *, Types::InstrumentHash> &
                inputSymbolMap) override {
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());
                m_configIndex = atoi(this->GetLastValueFromFile(configPath, "configIndex").c_str());
                std::vector<FileRead> fileReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_underlyInstrument, m_configIndex, fileReadVecs);

                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_callPolicySymbols, fileReadVecs,
                                            'C');
                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_putPolicySymbols, fileReadVecs, 'P');

                auto symbolItr = inputSymbolMap.find(m_underlyInstrument);
                if (symbolItr == inputSymbolMap.end()) {
                    assert(false);
                }
                m_underlyKseries = symbolItr->second->m_kSeriesMap.at(m_kperiod);
                m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;


                for (auto fileReadItr: fileReadVecs) {
                    if (strcmp(fileReadItr.instrumentStr.c_str(),
                               symbolItr->second->instrumentInfo.instrumentID.data()) == 0) {
                        m_basePrice = std::stof(fileReadItr.basePriceStr.c_str());
                    }
                }
                fprintf(stderr,
                        "[%s_%s] start, underlyInstrument=%s, kperiod=%d, MV=%.3f, multi=%.3f, tradingDay=%d, expireDay=%d, "
                        "biasRation=%.3f, maxOptionPos=%d, isRefreshDelta=%d, openAtDelta=%.3f, basePrice=%.3f, configIndex=%d\n",
                        m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(),
                        static_cast<int>(m_kperiod), m_MV, m_multi,
                        m_tradingDay, m_expireDay, m_biasRation, m_maxOptionPos,
                        m_isRefreshDelta,
                        m_openAtDelta, m_basePrice, m_configIndex);

                spdlog::info(
                    "[{}_{}] start, underlyInstrument={}, kperiod={}, MV={:.3f}, multi={:.3f}, tradingDay={}, expireDay={}, "
                    "biasRation={:.3f}, maxOptionPos={}, isRefreshDelta={}, openAtDelta={:.3f}, basePrice={:.3f}, configIndex={}",
                    m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(), static_cast<int>(m_kperiod),
                    m_MV, m_multi,
                    m_tradingDay, m_expireDay, m_biasRation, m_maxOptionPos,
                    m_isRefreshDelta,
                    m_openAtDelta, m_basePrice, m_configIndex);
                m_configIndex++;
            };


            virtual void runTick(const Types::MarketData *pMD) override {
                //                if(strcmp(pMD->instrumentID.data(), "IO2305-C-4000") ==0 ){
                //                    fprintf(stderr,"runTick instrument=%s, updateTime=%s, psSecond=%d, m_lastPsTime=%d, "
                //                                   "m_lastUnderlyBarIndex=%d, m_seriesIndex=%d\n",
                //                            pMD->instrumentID.data(), pMD->updateTime.data(), pMD->psSecond, m_lastPsTime,
                //                            m_lastUnderlyBarIndex, m_underlyKseries->m_seriesIndex);
                //                }

                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {
                    if (m_lastUnderlyBarIndex == 0 && m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                    } else if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        //    fprintf(stderr,"instrument=%s, updateTime=%s\n", pMD->instrumentID.data(), pMD->updateTime.data());
                        auto lastUnderlyKB = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex - 1];

                        double allCallDelta = getAllHoldDelta(m_callPolicySymbols);
                        double allPutDelta = getAllHoldDelta(m_putPolicySymbols);

                        bool isZeroCall = isHaveZeroDeltaPos(m_callPolicySymbols);
                        bool isZeroPut = isHaveZeroDeltaPos(m_putPolicySymbols);

                        if (isZeroCall == true || isZeroPut == true) {
                            spdlog::info("policyName=%s, updateTime=%s, isZeroCall=%d, isZeroPut=%d\n",
                                         m_policyName.c_str(), pMD->updateTime.data(), isZeroCall, isZeroPut);
                        }

                        if (m_tradingDay != m_expireDay && (
                                abs(lastUnderlyKB->m_close - m_basePrice) > m_basePrice * m_biasRation
                                || (allCallDelta == 0 && allPutDelta == 0) || m_isRefreshDelta == 1) && isZeroCall ==
                            false && isZeroPut == false) {
                            //rebalance;
                            _rebalanceDelta(m_callPolicySymbols, m_putPolicySymbols, allCallDelta, allPutDelta,
                                            -m_MV *10000 / (lastUnderlyKB->m_close * m_multi), m_openAtDelta,
                                            lastUnderlyKB->m_close);
                            m_isRefreshDelta = 0;
                            m_basePrice = lastUnderlyKB->m_close;
                        }

                        allCallDelta = getAllHoldDelta(m_callPolicySymbols);
                        allPutDelta = getAllHoldDelta(m_putPolicySymbols);

                        _checkMaxPositionRisk(m_callPolicySymbols.targetSignal.targetPosMaps, -1, m_maxOptionPos);
                        _checkMaxPositionRisk(m_putPolicySymbols.targetSignal.targetPosMaps, -1, m_maxOptionPos);

                        m_configLog->info("configIndex={}, instrument={}, {}, {}, {}, close={:.3f}, delta={:.3f}, "
                                          "targetPosition={}, basePrice={:.3f}, seriesIndex={}, "
                                          "allCallDelta={:.3f}, allPutDelta={:.3f}",
                                          m_configIndex, lastUnderlyKB->m_instrument.data(),
                                          lastUnderlyKB->m_tradingday,
                                          lastUnderlyKB->m_updateTimeBegin.data(), lastUnderlyKB->m_endPsTime,
                                          lastUnderlyKB->m_close,
                                          0.0, 0, m_basePrice, m_underlyKseries->m_seriesIndex - 1, allCallDelta,
                                          allPutDelta);
                        _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                        _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);

                        m_configLog->flush();
                        m_configIndex++;
                    }
                    m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                }
            }
        };
    }
}

#endif //OPTIONTRADING_SELLSTRANGLEV2_H
