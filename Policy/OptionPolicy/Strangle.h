//
// Created by zhangyingwei on 2024/10/31.
//

#ifndef OPTIONTRADING_STRANGLE_H
#define OPTIONTRADING_STRANGLE_H
#include "IOptionPolicy.h"
#include "../Types/Type.h"

namespace Cosmos {
    namespace Policy {


        class Strangle : public IOptionPolicy {
        private:


        public:
            Strangle(std::string const &policyName, std::string const &engineName, Types::Instrument_t &instrument,
                        Types::KPeriod kperiod, double mv, double multi, int tradingDay, int expireDay) :
                                IOptionPolicy(policyName, engineName, instrument,
                                                kperiod, mv, multi, tradingDay, expireDay) {

            }

            ~Strangle() {

            }

            virtual void start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) =0;

            virtual void runTick(const  Types::MarketData *pMD) =0;

            double getAllHoldDelta(PolicySymbolStruct const& policySymbolStruct){
                double allHoldDelta = 0.0;
                for(auto  symbolItr : policySymbolStruct.optionSymbolVecs){
                    auto targetPosition = getTargetPos(symbolItr->instrumentInfo.instrumentID,  policySymbolStruct.targetSignal.targetPosMaps);
                 //   fprintf(stderr," getAllHoldDelta instrument=%s %x %d\n", symbolItr->instrumentInfo.instrumentID.data(), symbolItr, Types::KPeroidToIntervalMap[m_kperiod]);
                    auto itr = symbolItr->m_kSeriesMap.find(m_kperiod);
                    if(itr == symbolItr->m_kSeriesMap.end()){
                        assert(false);
                    }
                    auto series = itr->second;
                    auto lastIndex = series->m_seriesIndex > 0 ?  series->m_seriesIndex-1 : 0;
                    auto lastOptionK = series->m_KDataVecs[lastIndex];
                    if(targetPosition !=0 && lastOptionK != nullptr ){
                        allHoldDelta += targetPosition * lastOptionK->delta;
                    }
                }
                return allHoldDelta;
            }

            double getAllHoldVega(PolicySymbolStruct const& policySymbolStruct) {
                double allHoldVega = 0.0;
                for (auto symbolItr: policySymbolStruct.optionSymbolVecs) {
                    auto targetPosition = getTargetPos(symbolItr->instrumentInfo.instrumentID, policySymbolStruct.targetSignal.targetPosMaps);
                    auto series = symbolItr->m_kSeriesMap.at(m_kperiod);
                    auto lastIndex = series->m_seriesIndex > 0 ? series->m_seriesIndex - 1 : 0;
                    auto lastOptionK = series->m_KDataVecs[lastIndex];
                    if (targetPosition != 0 && lastOptionK != nullptr) {
                        allHoldVega += targetPosition * lastOptionK->vega;
                    }
                }
                return allHoldVega;
            }

            void _GetValueFromFileByConfigIndex(char *filename, int inputConfigIndex, std::vector<FileRead> &fileReadVecs) {
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
                            fileReadVecs.emplace_back(fileRead);
                        }
                    }
                }
                fclose(fp);

            }

            void _adjustHoldDelta(PolicySymbolStruct & policySymbolStruct,
                                  double targetDelta, double holdDelta, double openAtDelta, char optionType, double underlyClose){
                double diffDelta = targetDelta - holdDelta;
                auto optionKBarIndex = m_underlyKseries->m_seriesIndex - 1 - m_underlyToBeginIndex;
                if(diffDelta * holdDelta >=0){  //same direction add
                    auto openAtmItr = getApproxiDeltaSymbol(policySymbolStruct.optionSymbolVecs , openAtDelta, optionType,  underlyClose, optionKBarIndex);
                    if(openAtmItr != nullptr ){
                        auto openAtmKSeries = openAtmItr->m_kSeriesMap.at(m_kperiod);
                        auto openAtDelta =   openAtmKSeries->m_KDataVecs[openAtmKSeries->m_seriesIndex - 1]->delta;

                        addPositionByGreeks(openAtmItr->instrumentInfo.instrumentID, policySymbolStruct.targetSignal.targetPosMaps,
                                            openAtDelta, diffDelta);
                    }
                }
                else if(diffDelta * holdDelta <0){
                    for(auto & symbolItr : policySymbolStruct.optionSymbolVecs){
                        if(symbolItr->m_kSeriesMap.at(m_kperiod)->m_seriesIndex ==0){
                            continue;
                        }
                        auto targetPosition = 0;
                        auto itrTGPos = policySymbolStruct.targetSignal.targetPosMaps.find(symbolItr->instrumentInfo.instrumentID);
                        if (itrTGPos != policySymbolStruct.targetSignal.targetPosMaps.end()) {
                            targetPosition = itrTGPos->second;
                        }

                        auto lastOptionK = symbolItr->m_kSeriesMap.at(m_kperiod)->m_KDataVecs[symbolItr->m_kSeriesMap.at(m_kperiod)->m_seriesIndex -1];
                        double instrumentDeltaCash =  targetPosition * lastOptionK->delta;

                        if(targetPosition ==0 || diffDelta * instrumentDeltaCash >0){
                            continue;
                        }

                        if( (diffDelta + instrumentDeltaCash) * instrumentDeltaCash  < 0){
                            diffDelta += instrumentDeltaCash;
                            itrTGPos->second = 0;
                        }else if ((diffDelta + instrumentDeltaCash) * instrumentDeltaCash > 0){
                            itrTGPos->second = round((instrumentDeltaCash + diffDelta) / lastOptionK->delta);
                            break;
                        }
                    }
                }
            }

            void _adjustHoldVega(PolicySymbolStruct & callPolicySymbols, PolicySymbolStruct & putPolicySymbols,
                                 double targetVega, double holdVega, double openAtDelta,  double underlyClose) {
                double diffVega = targetVega - holdVega;
                auto optionKBarIndex = m_underlyKseries->m_seriesIndex - 1 - m_underlyToBeginIndex;
                if (diffVega * targetVega >= 0) {  //same direction add
                    auto callAtmItr = getApproxiDeltaSymbol(callPolicySymbols.optionSymbolVecs , openAtDelta, 'C',  underlyClose, optionKBarIndex);
                    auto putAtmItr = getApproxiDeltaSymbol(putPolicySymbols.optionSymbolVecs , openAtDelta, 'P',  underlyClose, optionKBarIndex);

                    if(callAtmItr != nullptr && putAtmItr != nullptr){

                        auto callAtmKSeries = callAtmItr->m_kSeriesMap.at(m_kperiod);
                        auto putAtmKSeries = putAtmItr->m_kSeriesMap.at(m_kperiod);
                        auto callDelta =   callAtmKSeries->m_KDataVecs[callAtmKSeries->m_seriesIndex - 1]->delta;
                        auto putDelta =   putAtmKSeries->m_KDataVecs[putAtmKSeries->m_seriesIndex - 1]->delta;
                        auto callDiffVega = diffVega * std::abs(callDelta)/ (std::abs(callDelta) + std::abs(putDelta));
                        auto putDiffVega = diffVega * std::abs(putDelta)/ (std::abs(callDelta) + std::abs(putDelta));

                        auto ckSeries = callAtmItr->m_kSeriesMap.at(m_kperiod);
                        addPositionByGreeks(callAtmItr->instrumentInfo.instrumentID, callPolicySymbols.targetSignal.targetPosMaps,
                                            ckSeries->m_KDataVecs[ckSeries->m_seriesIndex - 1]->vega, callDiffVega);

                        auto pkSeries = putAtmItr->m_kSeriesMap.at(m_kperiod);
                        addPositionByGreeks(putAtmItr->instrumentInfo.instrumentID, putPolicySymbols.targetSignal.targetPosMaps,
                                            pkSeries->m_KDataVecs[pkSeries->m_seriesIndex - 1]->vega, putDiffVega);
                    }
                } else if (diffVega * targetVega < 0) {

                    double callHoldDelta = getAllHoldDelta(callPolicySymbols);
                    double putHoldDelta = getAllHoldDelta(putPolicySymbols);
                    double stepMinusDelta = callHoldDelta *0.05;

                    for(int i=0; i<20; i++){
                        _adjustHoldDelta(callPolicySymbols, callHoldDelta - stepMinusDelta, callHoldDelta, openAtDelta,'C', underlyClose);
                        _adjustHoldDelta(putPolicySymbols, putHoldDelta + stepMinusDelta, putHoldDelta, openAtDelta,'P', underlyClose);

                        double callHoldVega = getAllHoldVega(callPolicySymbols);
                        double putHoldVega = getAllHoldVega(putPolicySymbols);
                        double diffRealVega = targetVega - callHoldVega -putHoldVega;
                        if (diffVega * diffRealVega <0 && std::abs(diffRealVega) < std::abs(diffVega) * 0.1 ){
                            break;
                        }
                    }
                }
            }

            void _rebalanceDelta(PolicySymbolStruct & callPolicySymbols,   PolicySymbolStruct & putPolicySymbols,
                                    double allCallDelta, double allPutDelta, double posDelta, double openAtDelta, double underlyClose) {

                _adjustHoldDelta(callPolicySymbols, posDelta, allCallDelta, openAtDelta, 'C', underlyClose);
                _adjustHoldDelta(putPolicySymbols, -posDelta, allPutDelta, openAtDelta, 'P', underlyClose);

            };

        };
    }
}
#endif //OPTIONTRADING_STRANGLE_H
