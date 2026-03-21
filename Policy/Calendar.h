//
// Created by Zhangyingwei on 2023/5/10.
//

#ifndef OPTIONTRADING_CALENDAR_H
#define OPTIONTRADING_CALENDAR_H

#include "Strangle.h"
#include "../Types/Type.h"

namespace TrendFollow {
    namespace Policy {

        class Calendar : public Strangle {
        private:

            int m_tradeTime{0};
            double m_mv{0.0};
            double m_multi{1000.0};
            bool m_isCheck{false};
            int m_lastPsTime{0};
            int m_maxPosition{0};
            int m_isRefreshDelta{0};
            double m_openAtDelta{0.25};
            double m_adjCoef{0.35};


         //    Types::KPeriod m_kperiod;

            int m_isRefreshFarDelta{0};
            const KData::KSeries *m_underlyFarKseries{nullptr};
            int m_lastUnderlyFarBarIndex{0};


            PolicySymbolStruct m_callNearPolicySymbols;
            PolicySymbolStruct m_putNearPolicySymbols;

            PolicySymbolStruct m_callFarPolicySymbols;
            PolicySymbolStruct m_putFarPolicySymbols;



             Types::Instrument_t m_underlyFarInstrument{""};

        public:
            Calendar( Types::KPeriod kperiod, double mv,
                     std::string const &policyName, std::string const &engineName,
                     Types::Instrument_t &instrument, Types::Instrument_t & farInstrument,int tradeTime, double multi,
                     int tradingday, int maxPosition, int isRefreshDelta, double openAtDelta, double adjCoef) :
                    m_mv(mv), m_tradeTime(tradeTime),
                    m_multi(multi), m_maxPosition(maxPosition), m_isRefreshDelta(isRefreshDelta),
                    m_openAtDelta(openAtDelta),
                    m_adjCoef(adjCoef) {

                if (m_openAtDelta < 0.1 || m_openAtDelta > 0.5) {
                    assert(false);
                }
                m_kperiod = kperiod;
                m_policyName = policyName;
                m_engineName = engineName;
                m_tradingday = tradingday;
                m_underlyInstrument = instrument;
                m_underlyFarInstrument = farInstrument;
                spdlog::info("createPolicy engineName={}, policyName={}, kperiod={}  mv={}, tradingday={}, "
                             "nearUnderly={}, farUnderly={}, maxPosition={}, isRefreshDelta={}, openAtDelta={}, adjCoef={}",
                             engineName, policyName, (int) kperiod, m_mv, m_tradingday, m_underlyInstrument.data(),
                             m_underlyFarInstrument.data(), m_maxPosition, m_isRefreshDelta, m_openAtDelta, m_adjCoef);

                _initPolicyLogger();
            }

            ~Calendar() {

            }

            virtual void
            start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) override {
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());
                m_configIndex = atoi(this->GetLastValueFromFile(configPath, "configIndex").c_str());
                std::vector<FileRead> fileReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_configIndex, fileReadVecs);

               m_expireday = _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_callNearPolicySymbols, fileReadVecs, 'C' );
                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyInstrument, m_putNearPolicySymbols, fileReadVecs, 'P' );

                auto symbolItr = inputSymbolMap.find(m_underlyInstrument);
                if(symbolItr == inputSymbolMap.end()){
                    assert(false);
                }
                m_underlyKseries = symbolItr->second->m_kSeriesMap.at(m_kperiod);
                m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;


                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyFarInstrument, m_callFarPolicySymbols, fileReadVecs, 'C' );
                _initOptionPolicySymbolVecs(inputSymbolMap, m_underlyFarInstrument, m_putFarPolicySymbols, fileReadVecs, 'P' );

                 symbolItr = inputSymbolMap.find(m_underlyFarInstrument);
                if(symbolItr == inputSymbolMap.end()){
                    assert(false);
                }
                m_underlyFarKseries = symbolItr->second->m_kSeriesMap.at(m_kperiod);
                m_lastUnderlyFarBarIndex = m_underlyFarKseries->m_seriesIndex;

                m_callPolicySymbols.optionTargetPosMaps.clear();
                m_putPolicySymbols.optionTargetPosMaps.clear();
                _syncTargetMap( m_callNearPolicySymbols.optionTargetPosMaps, m_callPolicySymbols.optionTargetPosMaps);
                _syncTargetMap( m_callFarPolicySymbols.optionTargetPosMaps, m_callPolicySymbols.optionTargetPosMaps);

                _syncTargetMap( m_putNearPolicySymbols.optionTargetPosMaps ,m_putPolicySymbols.optionTargetPosMaps);
                _syncTargetMap( m_putFarPolicySymbols.optionTargetPosMaps, m_putPolicySymbols.optionTargetPosMaps);

                fprintf(stderr,
                        "[%s_%s] start, m_kperiod=%dMins, m_mv=%.3f, m_multi=%.3f, m_tradeTime=%d, maxPosition=%d, isRefreshDelta=%d, "
                        "openAtDelta=%.3f, expireday=%d, configIndex=%d\n",
                        m_engineName.c_str(), m_policyName.c_str(), Types::KPeroidToIntervalMap[m_kperiod], m_mv,
                        m_multi, m_tradeTime,
                        m_maxPosition, m_isRefreshDelta, m_openAtDelta,
                        m_expireday, m_configIndex);
                m_configIndex++;

            };





            KData::KData *_calNearSignal(const  Types::MarketData *pMD, const KData::KSeries *underlyseries,
                                         PolicySymbolStruct &  callPolicySymbols, PolicySymbolStruct &  putPolicySymbols,
                                          int &isRefreshDelta, double &allCallDelta, double &allPutDelta, double &allVega
            ) {

                auto lastUnderlyKB = underlyseries->m_KDataVecs[underlyseries->m_seriesIndex - 1];

                allCallDelta = getAllHoldDelta(callPolicySymbols);
                allPutDelta = getAllHoldDelta(putPolicySymbols);

                if (pMD->psSecond - m_tradeTime > 0 && pMD->psSecond - m_tradeTime < 5 * 60 &&
                    m_tradingday != m_expireday
                    && (abs(allCallDelta + allPutDelta) > m_adjCoef * m_mv / (lastUnderlyKB->m_close * m_multi)
                        || (allCallDelta == 0 && allPutDelta == 0) || isRefreshDelta == 1)) {
                    //rebalance;
                    _rebalanceDelta(callPolicySymbols, putPolicySymbols, allCallDelta, allPutDelta, -m_mv / (lastUnderlyKB->m_close * m_multi), m_openAtDelta, lastUnderlyKB->m_close);
                    isRefreshDelta = 0;
                }


                _checkMaxPositionRisk(callPolicySymbols.optionTargetPosMaps, -1, m_maxPosition);
                _checkMaxPositionRisk(putPolicySymbols.optionTargetPosMaps, -1, m_maxPosition);

                allCallDelta = getAllHoldDelta(callPolicySymbols);
                allPutDelta = getAllHoldDelta(putPolicySymbols);

                allVega = 0.0;
                allVega += getAllHoldVega(callPolicySymbols);
                allVega += getAllHoldVega(putPolicySymbols);

                return lastUnderlyKB;

            }

            KData::KData * _calFarSignal(const  Types::MarketData *pMD, const KData::KSeries *underlyseries,
                                         PolicySymbolStruct &  callFarPolicySymbols, PolicySymbolStruct &  putFarPolicySymbols,
                                         int &isRefreshDelta,
                               double &allCallDelta, double &allPutDelta, double &allHoldVega, double targetVega) {
                auto lastUnderlyKB = underlyseries->m_KDataVecs[underlyseries->m_seriesIndex - 1];

                allCallDelta = getAllHoldDelta(callFarPolicySymbols);
                allPutDelta = getAllHoldDelta(putFarPolicySymbols);


                if (pMD->psSecond - m_tradeTime > 0 && pMD->psSecond - m_tradeTime < 5 * 60 &&
                    m_tradingday != m_expireday
                    && (abs(allCallDelta + allPutDelta) > m_adjCoef * m_mv / (lastUnderlyKB->m_close * m_multi)
                        || (allCallDelta == 0 && allPutDelta == 0) || isRefreshDelta == 1)) {
                    //rebalance;
                    _rebalanceDelta(callFarPolicySymbols, putFarPolicySymbols, allCallDelta, allPutDelta,
                                    m_mv / (lastUnderlyKB->m_close * m_multi), m_openAtDelta, lastUnderlyKB->m_close );
                    isRefreshDelta = 0;
                }

                double holdCallVega = getAllHoldVega(callFarPolicySymbols);
                double holdPutVega = getAllHoldVega(putFarPolicySymbols);
                allHoldVega= holdCallVega + holdPutVega;

                if (pMD->psSecond - m_tradeTime > 0 && pMD->psSecond - m_tradeTime < 5 * 60 &&
                    m_tradingday != m_expireday &&  std::abs((targetVega- allHoldVega) * m_multi)>2000  ) {
                    _adjustHoldVega(callFarPolicySymbols, putFarPolicySymbols,
                                    targetVega, allHoldVega, m_openAtDelta, lastUnderlyKB->m_close);
                }

                _checkMaxPositionRisk(callFarPolicySymbols.optionTargetPosMaps, 1, m_maxPosition);
                _checkMaxPositionRisk(putFarPolicySymbols.optionTargetPosMaps, 1, m_maxPosition);


                allCallDelta = getAllHoldDelta(callFarPolicySymbols);
                allPutDelta = getAllHoldDelta(putFarPolicySymbols);

                holdCallVega = getAllHoldVega(callFarPolicySymbols);
                holdPutVega = getAllHoldVega(putFarPolicySymbols);
                allHoldVega= holdCallVega + holdPutVega;

                return lastUnderlyKB;
            }

            virtual void runTick(const  Types::MarketData *pMD) override {

                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {

                    if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        m_isCheck = true;
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                        m_lastPsTime = pMD->psSecond;
                    } else if (m_isCheck == true && m_lastUnderlyBarIndex > 0) {
                        if (pMD->psSecond - m_lastPsTime > 10) {
                            //    fprintf(stderr,"instrument=%s, updateTime=%s\n", pMD->instrumentID.data(), pMD->updateTime.data());
                            double allNearCallDelta = 0.0;
                            double allNearPutDelta = 0.0;
                            double allNearVega = 0.0;
                            auto lastNearUnderlyKB = _calNearSignal(pMD, m_underlyKseries, m_callNearPolicySymbols,
                                                                    m_putNearPolicySymbols,
                                                                    m_isRefreshDelta,
                                                                    allNearCallDelta, allNearPutDelta, allNearVega);

                            double allFarCallDelta = 0.0;
                            double allFarPutDelta = 0.0;
                            double allFarVega = 0.0;

                            auto lastFarUnderlyKB = _calFarSignal(pMD, m_underlyFarKseries, m_callFarPolicySymbols,
                                                                  m_putFarPolicySymbols, m_isRefreshDelta, allFarCallDelta, allFarPutDelta,
                            allFarVega, -allNearVega);


                            m_configLog->info("configIndex={}, instrument={} {}, {}, {}, {}, close={:.3f}  {:.3f}, delta={:.3f}, "
                                              "targetPosition={},  seriesIndex={}, allDelta={:.3f},{:.3f},{:.3f},{:.3f}, "
                                              "allVega={:.3f},{:.3f}",
                                              m_configIndex, lastNearUnderlyKB->m_instrument.data(), lastFarUnderlyKB->m_instrument.data(),
                                              lastNearUnderlyKB->m_tradingday, lastNearUnderlyKB->m_updateTime.data(), lastNearUnderlyKB->m_psTime,
                                              lastNearUnderlyKB->m_close, lastFarUnderlyKB->m_close,
                                              0.0, 0, m_underlyKseries->m_seriesIndex - 1, allNearCallDelta,allNearPutDelta,allFarCallDelta,
                                              allFarPutDelta, allNearVega, allFarVega);
                            _writeOptionPolicyLog(m_callNearPolicySymbols, m_configIndex);
                            _writeOptionPolicyLog(m_putNearPolicySymbols, m_configIndex);

                            _writeOptionPolicyLog(m_callFarPolicySymbols, m_configIndex);
                            _writeOptionPolicyLog(m_putFarPolicySymbols, m_configIndex);

                            m_configLog->flush();
                            m_configIndex++;
                            m_isCheck = false;

                            m_callPolicySymbols.optionTargetPosMaps.clear();
                            m_putPolicySymbols.optionTargetPosMaps.clear();
                            _syncTargetMap( m_callNearPolicySymbols.optionTargetPosMaps, m_callPolicySymbols.optionTargetPosMaps);
                            _syncTargetMap( m_callFarPolicySymbols.optionTargetPosMaps, m_callPolicySymbols.optionTargetPosMaps);

                            _syncTargetMap( m_putNearPolicySymbols.optionTargetPosMaps ,m_putPolicySymbols.optionTargetPosMaps);
                            _syncTargetMap( m_putFarPolicySymbols.optionTargetPosMaps, m_putPolicySymbols.optionTargetPosMaps);
                        }
                    }
                }
            }

            int _syncTargetMap(std::map<Types::Instrument_t, int> const & optionTargetPosMaps,
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

        };
    }
}

#endif //OPTIONTRADING_CALENDAR_H
