//
// Created by Zhangyingwei on 2023/5/10.
//

#ifndef OPTIONTRADING_CALENDAR_H
#define OPTIONTRADING_CALENDAR_H

#include "Strangle.h"
#include "../Types/Type.h"

namespace Cosmos {
    namespace Policy {
        class CalendarSTRD : public Strangle {
        private:
            int m_maxOptionPos{0};
            int m_isRefreshDelta{0};
            double m_openAtDelta{0.25};
            double m_biasRation{0.01};
            double m_basePrice{0.0};
            int m_farExpireDay{0};

            int m_calendarPos{0};
            double m_slopeSqrtT{0.0};
            double m_slopeSqrtTMean{0.0};
            double m_slopeSqrtTSTD{0.0};
            double m_nearT{0.0};
            double m_farT{0.0};
            int m_slopeLength{240};

            //    Types::KPeriod m_kperiod;

            int m_isRefreshFarDelta{0};
            const KData::KSeries *m_underlyFarKseries{nullptr};
            int m_lastUnderlyFarBarIndex{0};
            bool m_isNearFinish{false};

            int m_underlyFarToBeginIndex{0};

            int m_lastNearOptionIndex{0};
            int m_lastFarOptionIndex{0};


            std::vector<double> m_fiveSlopeSqrtTVec;


            PolicySymbolStruct m_callNearPolicySymbols;
            PolicySymbolStruct m_putNearPolicySymbols;

            PolicySymbolStruct m_callFarPolicySymbols;
            PolicySymbolStruct m_putFarPolicySymbols;


            Types::Instrument_t m_underlyFarInstrument{""};

        public:
            CalendarSTRD(std::string const &policyName, std::string const &engineName,
                         Types::Instrument_t &nearInstrument, Types::Instrument_t const &farInstrument,
                         Types::KPeriod kperiod, double mv, double multi,
                         int tradingDay, int nearExpireDay, int farExpireDay, int maxOptionPos,
                         int isRefreshDelta, double openAtDelta, double biasRation,
                         decltype(m_getUnderlyToBeginIndexFunc) getUnderlyToBeginIndexFunc) : Strangle(policyName,
                    engineName,
                    nearInstrument, kperiod, mv, multi, tradingDay, nearExpireDay, getUnderlyToBeginIndexFunc),
                m_maxOptionPos(maxOptionPos), m_isRefreshDelta(isRefreshDelta), m_openAtDelta(openAtDelta),
                m_biasRation(biasRation), m_underlyFarInstrument(farInstrument), m_farExpireDay(farExpireDay) {
                if (m_openAtDelta < 0.1 || m_openAtDelta > 0.5) {
                    assert(false);
                }


                _initPolicyLogger();
            }

            ~CalendarSTRD() {
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
                            char insname[56]{"instr"};
                            _getValueInLine(buf, insname, fileRead.instrumentStr);
                            char tagname[56]{"targetPos"};
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

            virtual void start(std::unordered_map<Types::Instrument_t, Types::Symbol *, Types::InstrumentHash> &
                inputSymbolMap) override {

                auto diff = (Utils::intToSysDays(m_expireDay) - Utils::intToSysDays(m_tradingDay)).count();
                m_nearT = diff / 365.0;

                auto farDiff = (Utils::intToSysDays(m_farExpireDay) - Utils::intToSysDays(m_tradingDay)).count();
                m_farT = farDiff / 365.0;

                m_underlyToBeginIndex = m_getUnderlyToBeginIndexFunc(m_underlyInstrument, m_kperiod);
                m_underlyFarToBeginIndex = m_getUnderlyToBeginIndexFunc(m_underlyFarInstrument, m_kperiod);
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());
                m_configIndex = atoi(this->GetLastValueFromFile(configPath, "configIndex").c_str());
                std::vector<FileRead> fileNearReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_underlyInstrument, m_configIndex, fileNearReadVecs);

                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_callNearPolicySymbols,
                                            fileNearReadVecs,
                                            'C');
                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_putNearPolicySymbols,
                                            fileNearReadVecs, 'P');

                auto symbolNearItr = inputSymbolMap.find(m_underlyInstrument);
                if (symbolNearItr == inputSymbolMap.end()) {
                    assert(false);
                }
                m_underlyKseries = symbolNearItr->second->m_kSeriesMap.at(m_kperiod);
                m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                for (auto fileReadItr: fileNearReadVecs) {
                    if (strcmp(fileReadItr.instrumentStr.c_str(),
                               symbolNearItr->second->instrumentInfo.instrumentID.data()) == 0) {
                        m_basePrice = std::stof(fileReadItr.basePriceStr.c_str());
                    }
                }


                std::vector<FileRead> fileFarReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_underlyFarInstrument, m_configIndex, fileFarReadVecs);

                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyFarInstrument, m_callFarPolicySymbols,
                                            fileFarReadVecs,
                                            'C');
                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyFarInstrument, m_putFarPolicySymbols,
                                            fileFarReadVecs, 'P');

                auto symbolFarItr = inputSymbolMap.find(m_underlyFarInstrument);
                if (symbolFarItr == inputSymbolMap.end()) {
                    assert(false);
                }
                m_underlyFarKseries = symbolFarItr->second->m_kSeriesMap.at(m_kperiod);
                m_lastUnderlyFarBarIndex = m_underlyFarKseries->m_seriesIndex;
                fprintf(stderr,
                        "[%s_%s] start, underlyNearInstrument=%s, underlyFarInstrument=%s, kperiod=%d, MV=%.3f, multi=%.3f, tradingDay=%d, expireDay=%d, "
                        "biasRation=%.3f, maxOptionPos=%d, isRefreshDelta=%d, openAtDelta=%.3f, basePrice=%.3f, configIndex=%d\n",
                        m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(),
                        m_underlyFarInstrument.data(),
                        static_cast<int>(m_kperiod), m_MV, m_multi,
                        m_tradingDay, m_expireDay, m_biasRation, m_maxOptionPos,
                        m_isRefreshDelta,
                        m_openAtDelta, m_basePrice, m_configIndex);

                spdlog::info(
                    "[{}_{}] start, underlyNearInstrument={}, underlyFarInstrument={}, kperiod={}, MV={:.3f}, multi={:.3f}, tradingDay={}, expireDay={}, "
                    "biasRation={:.3f}, maxOptionPos={}, isRefreshDelta={}, openAtDelta={:.3f}, basePrice={:.3f}, configIndex={}",
                    m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(),
                    m_underlyFarInstrument.data(), static_cast<int>(m_kperiod),
                    m_MV, m_multi,
                    m_tradingDay, m_expireDay, m_biasRation, m_maxOptionPos,
                    m_isRefreshDelta,
                    m_openAtDelta, m_basePrice, m_configIndex);
                m_configIndex++;
            };


            virtual void updateParam(const Types::NetModifyParam *netModifyParam) override {
            };


            double _calATM(KData::KData * kData, double t) {
                auto oneMinusBeta = 1.0 - kData->m_sabrPRMT.beta;
                auto fPow = std::pow(kData->m_forwardPrice, oneMinusBeta);
                auto alphaDivF = kData->m_sabrPRMT.alpha / fPow;
                auto term1 = (std::pow(oneMinusBeta, 2.0) / 24.0) * std::pow(alphaDivF, 2.0);
                auto term2 = 0.25 * ( kData->m_sabrPRMT.rho * kData->m_sabrPRMT.beta * kData->m_sabrPRMT.nu * alphaDivF );
                auto term3 = ((2.0 - 3.0 * std::pow(kData->m_sabrPRMT.rho, 2.0)) / 24.0) * std::pow(kData->m_sabrPRMT.nu, 2.0);
                auto bracket = 1.0 + (term1 + term2 + term3) * t;
                auto atmSigma =  alphaDivF * bracket;
                return atmSigma;
            }



            void _calSlopeSqrtT(KData::KData * nearKData, KData::KData * farKData)  {

                if ( (nearKData->m_tradingDay != farKData->m_tradingDay) || strcmp(nearKData->m_updateTimeBegin.data(), farKData->m_updateTimeBegin.data()) != 0 ) {
                    assert(false);
                }

                auto nearDiff = (Utils::intToSysDays(m_expireDay) - Utils::intToSysDays(nearKData->m_tradingDay)).count();
                auto nearT = nearDiff / 365.0;
                auto nearATMIV = _calATM(nearKData, nearT);

                auto farDiff = (Utils::intToSysDays(m_farExpireDay) - Utils::intToSysDays(farKData->m_tradingDay)).count();
                auto farT = farDiff / 365.0;
                auto farATMIV = _calATM(farKData, farT);

                auto nearW = std::pow(nearATMIV, 2.0) * nearT;
                auto farW = std::pow(farATMIV, 2.0) *farT;
                auto deltaT = farT - nearT;
                auto fwdW = farW - nearW;
                if (fwdW > 0.0 && deltaT > 0.02) {

                    auto fwdATMIV = std::pow( fwdW/deltaT, 0.5);
                    auto slopeSqrtT = fwdATMIV - nearATMIV;
                    m_fiveSlopeSqrtTVec.emplace_back(slopeSqrtT);

                }
            }



            void _initSlopeSqrtT() {

                if ( m_underlyKseries->m_seriesIndex != m_underlyFarKseries->m_seriesIndex ) {
                    assert(false);
                }
                for (auto i = 0; i < m_underlyKseries->m_seriesIndex; i++) {
                    _calSlopeSqrtT(m_underlyKseries->m_KDataVecs[i], m_underlyFarKseries->m_KDataVecs[i]);
                }
                int beginI = 0, endI = 0;
                if (m_underlyKseries->m_seriesIndex < m_slopeLength) {
                    beginI = 0;
                    endI = m_underlyKseries->m_seriesIndex-1;
                } else {
                    beginI = m_underlyKseries->m_seriesIndex - m_slopeLength ;
                    endI = m_underlyKseries->m_seriesIndex -1;
                }
                m_slopeSqrtTMean = Indicator::MA(m_fiveSlopeSqrtTVec, beginI, endI);
                m_slopeSqrtTSTD = Indicator::STDEV(m_fiveSlopeSqrtTVec, beginI, endI);
            }





            void calendarSignal(const Types::MarketData *pMD) {
                if (m_tradingDay == m_expireDay) {
                    return;
                }

                auto lastNearUnderlyKB = m_underlyKseries->m_KDataVecs[m_lastUnderlyBarIndex];
                m_lastNearOptionIndex = m_underlyKseries->m_seriesIndex - 1 - m_underlyToBeginIndex;

                auto lastFarUnderlyKB = m_underlyFarKseries->m_KDataVecs[m_lastUnderlyFarBarIndex];
                m_lastFarOptionIndex = m_underlyFarKseries->m_seriesIndex - 1 - m_underlyFarToBeginIndex;

                if (Utils::TradingHours::isNoTradeAfterAuctionEnd(
                        m_underlyKseries->m_insInfo.productID, pMD->psSecond, 240) == true) {
                    return;
                }

                _calSlopeSqrtT(lastNearUnderlyKB, lastFarUnderlyKB);

                if (m_calendarPos < 1 && m_slopeSqrtT > m_slopeSqrtTMean + 2.2 * m_slopeSqrtTSTD) {
                    // long near short far
                    _setTargetAllTargetPosZero(m_callNearPolicySymbols.targetSignal.targetPosMaps);
                    _setTargetAllTargetPosZero(m_putNearPolicySymbols.targetSignal.targetPosMaps);
                    double allNearCallDelta = getAllHoldDelta(m_callNearPolicySymbols);
                    double allNearPutDelta = getAllHoldDelta(m_putNearPolicySymbols);
                    _rebalanceDelta(m_callNearPolicySymbols, m_putNearPolicySymbols, allNearCallDelta, allNearPutDelta,
                                      m_MV *10000 / (lastNearUnderlyKB->m_close * m_multi), m_openAtDelta,
                                      lastNearUnderlyKB->m_close, m_lastNearOptionIndex);

                    _setTargetAllTargetPosZero(m_callFarPolicySymbols.targetSignal.targetPosMaps);
                    _setTargetAllTargetPosZero(m_putFarPolicySymbols.targetSignal.targetPosMaps);
                    double allFarCallDelta = getAllHoldDelta(m_callFarPolicySymbols);
                    double allFarPutDelta = getAllHoldDelta(m_putFarPolicySymbols);
                    _rebalanceDelta(m_callFarPolicySymbols, m_putFarPolicySymbols, allFarCallDelta, allFarPutDelta,
                                      -m_MV *10000 / (lastFarUnderlyKB->m_close * m_multi), m_openAtDelta,
                                      lastFarUnderlyKB->m_close, m_lastFarOptionIndex);
                    _rebalanceCalendarVega(lastNearUnderlyKB, lastFarUnderlyKB);
                    m_calendarPos =1;
                    m_basePrice =   lastNearUnderlyKB->m_close;

                } else if (m_calendarPos > -1 && m_slopeSqrtT > m_slopeSqrtTMean - 2.2 * m_slopeSqrtTSTD) {
                    // short near long far

                    _setTargetAllTargetPosZero(m_callNearPolicySymbols.targetSignal.targetPosMaps);
                    _setTargetAllTargetPosZero(m_putNearPolicySymbols.targetSignal.targetPosMaps);
                    double allNearCallDelta = getAllHoldDelta(m_callNearPolicySymbols);
                    double allNearPutDelta = getAllHoldDelta(m_putNearPolicySymbols);
                    _rebalanceDelta(m_callNearPolicySymbols, m_putNearPolicySymbols, allNearCallDelta, allNearPutDelta,
                                      -m_MV *10000 / (lastNearUnderlyKB->m_close * m_multi), m_openAtDelta,
                                      lastNearUnderlyKB->m_close, m_lastNearOptionIndex);

                    _setTargetAllTargetPosZero(m_callFarPolicySymbols.targetSignal.targetPosMaps);
                    _setTargetAllTargetPosZero(m_putFarPolicySymbols.targetSignal.targetPosMaps);
                    double allFarCallDelta = getAllHoldDelta(m_callFarPolicySymbols);
                    double allFarPutDelta = getAllHoldDelta(m_putFarPolicySymbols);
                    _rebalanceDelta(m_callFarPolicySymbols, m_putFarPolicySymbols, allFarCallDelta, allFarPutDelta,
                                      m_MV *10000 / (lastFarUnderlyKB->m_close * m_multi), m_openAtDelta,
                                      lastFarUnderlyKB->m_close, m_lastFarOptionIndex);

                    _rebalanceCalendarVega(lastNearUnderlyKB, lastFarUnderlyKB);
                    m_calendarPos = -1;
                    m_basePrice =   lastNearUnderlyKB->m_close;
                }

                if (abs(lastNearUnderlyKB->m_close - m_basePrice) > m_basePrice * m_biasRation && m_calendarPos != 0) {
                    _rebalanceCalendarDelta(lastNearUnderlyKB, lastFarUnderlyKB);
                    m_basePrice = lastNearUnderlyKB->m_close;
                }


                auto allNearCallDelta = getAllHoldDelta(m_callNearPolicySymbols);
                auto allNearPutDelta = getAllHoldDelta(m_putNearPolicySymbols);
                auto allFarCallDelta = getAllHoldDelta(m_callFarPolicySymbols);
                auto allFarPutDelta = getAllHoldDelta(m_putFarPolicySymbols);

                _checkMaxPositionRisk(m_callNearPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPos);
                _checkMaxPositionRisk(m_putNearPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPos);
                _checkMaxPositionRisk(m_callFarPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPos);
                _checkMaxPositionRisk(m_putFarPolicySymbols.targetSignal.targetPosMaps, 0, m_maxOptionPos);

                m_configLog->info("configIndex={}, instr={}, {}, {}, {}, close={:.3f}, delta={:.3f}, targetPos={}, "
                                  "basePrice={:.3f}, seriesIndex={}, nearCallDelta={:.3f}, nearPutDelta={:.3f}, "
                                  "farCallDelta={:.3f}, farPutDelta={:.3f}",
                                  m_configIndex, lastNearUnderlyKB->m_instrument.data(),
                                  lastNearUnderlyKB->m_tradingDay,
                                  lastNearUnderlyKB->m_updateTimeBegin.data(), pMD->updateTime.data(),
                                  lastNearUnderlyKB->m_close,
                                  0.0, 0, m_basePrice, m_underlyKseries->m_seriesIndex - 1, allNearCallDelta,
                                  allNearPutDelta, allFarCallDelta, allFarPutDelta);
                _writeOptionPolicyLog(m_callNearPolicySymbols, m_lastNearOptionIndex, m_configIndex);
                _writeOptionPolicyLog(m_putNearPolicySymbols, m_lastNearOptionIndex, m_configIndex);
                _writeOptionPolicyLog(m_callFarPolicySymbols, m_lastFarOptionIndex, m_configIndex);
                _writeOptionPolicyLog(m_putFarPolicySymbols,m_lastFarOptionIndex, m_configIndex);

                m_configLog->flush();
                m_configIndex++;
            }


            virtual void runTick(const Types::MarketData *pMD) override {
                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {
                    if (m_lastUnderlyBarIndex == 0 && m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                    } else if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        m_isNearFinish = true;
                    }

                    if (m_isNearFinish == true) {
                        m_lastUnderlyFarBarIndex = m_underlyFarKseries->m_seriesIndex;
                        if (m_lastUnderlyFarBarIndex == m_lastUnderlyFarBarIndex) {
                            //near and far all finish
                            this->calendarSignal(pMD);

                            m_isNearFinish = false;
                        }
                    }

                    m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                    m_lastUnderlyFarBarIndex = m_underlyFarKseries->m_seriesIndex;
                }
            }


            void _rebalanceCalendarDelta(KData::KData *nearUnderlyKData, KData::KData *farUnderlyKData) {

                auto allNearCallDelta = getAllHoldDelta(m_callNearPolicySymbols);
                auto allNearPutDelta = getAllHoldDelta(m_putNearPolicySymbols);
                auto allFarCallDelta = getAllHoldDelta(m_callFarPolicySymbols);
                auto allFarPutDelta = getAllHoldDelta(m_putFarPolicySymbols);
                auto biasDelta = - (allFarCallDelta + allFarPutDelta);

                auto targetDelta = m_MV * 10000 / (nearUnderlyKData->m_close * m_multi);

                if (m_calendarPos == -1) {
                    _adjustHoldDelta(m_callNearPolicySymbols, targetDelta + biasDelta *0.5, allNearCallDelta, m_openAtDelta, 'C', nearUnderlyKData->m_close, m_lastNearOptionIndex);
                    _adjustHoldDelta(m_putNearPolicySymbols, -targetDelta + biasDelta*0.5, allNearPutDelta, m_openAtDelta, 'P',  nearUnderlyKData->m_close, m_lastNearOptionIndex);
                }else if (m_calendarPos == 1) {
                    _adjustHoldDelta(m_callNearPolicySymbols, -targetDelta + biasDelta *0.5, allNearCallDelta, m_openAtDelta, 'C', nearUnderlyKData->m_close, m_lastNearOptionIndex);
                    _adjustHoldDelta(m_putNearPolicySymbols, targetDelta + biasDelta*0.5, allNearPutDelta, m_openAtDelta, 'P',  nearUnderlyKData->m_close, m_lastNearOptionIndex);
                }

                _rebalanceCalendarVega(nearUnderlyKData, farUnderlyKData);
            };

            void _rebalanceCalendarVega(KData::KData *nearUnderlyKData, KData::KData *farUnderlyKData) {
                double nearCallVega = getAllHoldVega(m_callNearPolicySymbols);
                double nearPutVega = getAllHoldVega(m_putNearPolicySymbols);
                double farCallVega = getAllHoldVega(m_callFarPolicySymbols);
                double farPutVega = getAllHoldVega(m_putFarPolicySymbols);
                auto targetVega = nearCallVega + nearPutVega;
                targetVega = targetVega * std::pow(m_farT / m_nearT, 0.5);
                auto holdVega = farCallVega + farPutVega;

                _adjustHoldVega(m_callFarPolicySymbols, m_putFarPolicySymbols,
                           -targetVega, holdVega, m_openAtDelta, farUnderlyKData->m_close, m_lastFarOptionIndex);
            }

            void _setTargetAllTargetPosZero(decltype(m_callPolicySymbols.targetSignal.targetPosMaps) & optionTargetPosMaps){
                for (auto itr = optionTargetPosMaps.begin(); itr != optionTargetPosMaps.end(); itr++) {
                    itr->second = 0;
                }
            }

            int _syncTargetMap(std::map<Types::Instrument_t, int> const &optionTargetPosMaps,
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
        };
    }
}

#endif //OPTIONTRADING_CALENDAR_H
