//
// Created by zhangyw on 2/2/21.
//

#ifndef Cosmos_LPOLICY_H
#define Cosmos_LPOLICY_H
#include "../IPolicy.h"
#include "../Types/Symbol.h"
#include "../Utils/LogUtils.h"

namespace Cosmos {
    namespace Policy {

        struct FileRead {
            std::string instrumentStr{""};
            std::string targetPositionStr{""};
            std::string marketPositionStr{""};
            std::string preMarketPositionStr{""};
            std::string signalPriceStr{""};
            std::string holdStrikePriceStr{""};
            std::string basePriceStr{""};
        };

        struct PolicySymbolStruct{
            std::vector<const Types::Symbol *> optionSymbolVecs;
            TargetSignal targetSignal;
        };

        class IOptionPolicy  :  public IPolicy{

        public:

            int m_configIndex{0};
            PolicySymbolStruct m_callPolicySymbols;
            PolicySymbolStruct m_putPolicySymbols;
            int m_expireDay{0};
            int m_underlyToBeginIndex{0};
            IOptionPolicy(std::string const &policyName, std::string const &engineName,
                          Types::Instrument_t &instrument, Types::KPeriod kperiod, double mv, double multi,
                          int tradingDay, int expireDay ) : IPolicy(policyName, engineName,
                                                                     instrument, kperiod, mv, multi,  tradingDay) , m_expireDay(expireDay)
            {

            }


            void _initOptionPolicySymbolVecs(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap,
                                       Types::Instrument_t const& underlyInstrument, PolicySymbolStruct & policySymbols,
                                       std::vector<FileRead>const& fileReadVecs, char optionType ){
               // int expireDay =0;
                for(auto symbolItr : inputSymbolMap){
                    if ( symbolItr.second->instrumentInfo.productIDClass == Types::ProductClass::option &&
                          symbolItr.second->instrumentInfo.optionType == optionType &&
                            strcmp(symbolItr.second->instrumentInfo.underly.data(), underlyInstrument.data()) ==0)
                    {
                    //    expireDay = symbolItr.second->instrumentInfo.expireDate;
                        policySymbols.optionSymbolVecs.emplace_back(symbolItr.second);
                        for(auto fileReadItr : fileReadVecs){
                            if(strcmp(fileReadItr.instrumentStr.c_str(), symbolItr.second->instrumentInfo.instrumentID.data())==0){
                                policySymbols.targetSignal.targetPosMaps[symbolItr.second->instrumentInfo.instrumentID] = std::stoi(fileReadItr.targetPositionStr.c_str());
                                policySymbols.targetSignal.lastTargetPosMaps[symbolItr.second->instrumentInfo.instrumentID] = std::stoi(fileReadItr.targetPositionStr.c_str());
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



            bool isHaveZeroDeltaPos(PolicySymbolStruct & policySymbols) {
                for (auto & optionSymbol: policySymbols.optionSymbolVecs) {
                    int  targetPosition =  getTargetPos(optionSymbol->instrumentInfo.instrumentID, policySymbols.targetSignal.targetPosMaps);
                    if (targetPosition !=0) {
                        auto series = optionSymbol->m_kSeriesMap.at(m_kperiod);
                        auto lastIndex = series->m_seriesIndex > 0 ? series->m_seriesIndex - 1 : 0;
                        auto lastOptionK = series->m_KDataVecs[lastIndex];
                        if ( abs(lastOptionK->delta) < 0.00001 && optionSymbol->lastMD->askPrice[0] > 10* optionSymbol->instrumentInfo.tickSize) {
                            fprintf(stderr, "policyName=%s, instrument=%s, updateTimeBegin=%s, pos=%d,  delta=%.3f\n", m_policyName.c_str(), optionSymbol->instrumentInfo.instrumentID.data(),
                                lastOptionK->m_updateTimeBegin.data(), targetPosition, lastOptionK->delta);
                            return true;
                        }
                    }
                }
                return false;
            }

            void _writeOptionPolicyLog( PolicySymbolStruct & policySymbols,  int configIndex) {
                auto optionIndex = m_underlyKseries->m_seriesIndex - 1 - m_underlyToBeginIndex;
                for (auto & optionSymbol: policySymbols.optionSymbolVecs) {
                    int  targetPosition =  getTargetPos(optionSymbol->instrumentInfo.instrumentID, policySymbols.targetSignal.targetPosMaps);
                    int lastTargetPosition  = getTargetPos(optionSymbol->instrumentInfo.instrumentID, policySymbols.targetSignal.lastTargetPosMaps);
                    if(targetPosition !=0 || (targetPosition ==0 && lastTargetPosition!=0)) {


                        auto series = optionSymbol->m_kSeriesMap.at(m_kperiod);
                 //       auto lastIndex = series->m_seriesIndex > 0 ? series->m_seriesIndex - 1 : 0;
                        auto lastOptionK = series->m_KDataVecs[optionIndex];

//                        if(lastOptionK->m_tradingday ==0){
//                            assert(false);
//                        }down
                        m_configLog->info(
                                "configIndex={}, instr={}, {}, {}, {}, close={:.3f}({:.3f}, {:.3f}), sgnPrice={:.3f}, delta={:.3f}, "
                                "targetPos={}, seriesIndex={}", configIndex, optionSymbol->instrumentInfo.instrumentID.data(), lastOptionK->m_tradingday,
                                lastOptionK->m_updateTimeBegin.data(), lastOptionK->m_endPsTime, lastOptionK->m_close, lastOptionK->m_bidPrice,
                                lastOptionK->m_askPrice, (lastOptionK->m_bidPrice +lastOptionK->m_askPrice) * 0.5, lastOptionK->delta, targetPosition,
                                optionIndex);
                    }
                }
                policySymbols.targetSignal.lastTargetPosMaps.clear();
                std::copy(policySymbols.targetSignal.targetPosMaps.begin(), policySymbols.targetSignal.targetPosMaps.end(),
                          std::inserter(policySymbols.targetSignal.lastTargetPosMaps, policySymbols.targetSignal.lastTargetPosMaps.begin()));
            //    memcpy(&lastTargetPosMaps, &targetPosMaps, sizeof(targetPosMaps));
            }

            int getTargetPos(Types::Instrument_t const instrumentID,
                             decltype(m_callPolicySymbols.targetSignal.targetPosMaps) const& targetPosMaps) {
                int targetPosition = 0;
                auto itrTGPos = targetPosMaps.find(instrumentID);
                if (itrTGPos != targetPosMaps.end()) {
                    targetPosition = itrTGPos->second;
                }
                return targetPosition;
            }


            const Types::Symbol* getApproxiDeltaSymbol(decltype(m_callPolicySymbols.optionSymbolVecs) const& symbolCallVecs,
                                                       double findAtDelta , char optionType, double undelyClose, int optionKBarIndex){
                std::vector<const Types::Symbol *> filterSymbolVecs;
                int itmIndex =0 ;
                // for (auto symbolItr: symbolCallVecs) {
                for (auto itr = symbolCallVecs.rbegin(); itr != symbolCallVecs.rend(); itr++) {
                    auto symbolItr = *itr;
                //    if (symbolItr->m_kSeriesMap.at(m_kperiod)->m_seriesIndex > 0) {
                        if (std::abs(symbolItr->m_kSeriesMap.at(m_kperiod)->m_lastDelta) < 0.00001 ||
                            std::abs(symbolItr->m_kSeriesMap.at(m_kperiod)->m_lastDelta) > 0.70) {
                            continue;
                        }
                        if ((optionType == 'C' && symbolItr->m_kSeriesMap.at(m_kperiod)->m_insInfo.strikePrice < undelyClose) ||
                            (optionType == 'P' && symbolItr->m_kSeriesMap.at(m_kperiod)->m_insInfo.strikePrice > undelyClose)) {
                            itmIndex ++;
                        }
                        if (itmIndex >2) {
                            continue;
                        }
                       // fprintf(stderr, "add delta Filter %s, delta=%.3f, underlyClose=%.3f\n", symbolItr->instrumentInfo.instrumentID.data(), 
			//symbolItr->m_kSeriesMap.at(m_kperiod)->m_lastDelta, undelyClose);
                        filterSymbolVecs.emplace_back(symbolItr);
                //    }
                }
                if (filterSymbolVecs.size() > 0) {
                    auto specialDeltaItr = std::min_element(filterSymbolVecs.begin(), filterSymbolVecs.end(),
                                                       [findAtDelta, optionKBarIndex, this](auto &symbol_a, auto &symbol_b) {
                                                           auto kbar_a = symbol_a->m_kSeriesMap.at(m_kperiod)->m_KDataVecs[optionKBarIndex];
                                                           auto kbar_b = symbol_b->m_kSeriesMap.at(m_kperiod)->m_KDataVecs[optionKBarIndex];
                                                           return std::abs(
                                                                   std::abs(kbar_a->delta) - std::abs(findAtDelta)) <
                                                                  std::abs(std::abs(kbar_b->delta) -
                                                                           std::abs(findAtDelta));
                                                       });
                    return *specialDeltaItr;

                }else{
                    return nullptr;
                }
            }


            int addPositionByGreeks(Types::Instrument_t const& instrumentID, decltype(m_callPolicySymbols.targetSignal.targetPosMaps) & targetPosMaps, double greeks, double diffGreeks){
                auto itrTGPos = targetPosMaps.find(instrumentID);
                if (itrTGPos == targetPosMaps.end()) {
                    targetPosMaps[instrumentID] = 0;
                    itrTGPos = targetPosMaps.find(instrumentID);
                }
                int preT =     itrTGPos->second;
                itrTGPos->second += round(diffGreeks / ( greeks ) );
                fprintf(stderr, "add Ins=%s, diffGreeks=%.3f, preT=%d, targetPos=%d\n",
                        instrumentID.data(), diffGreeks, preT, itrTGPos->second);
                return 1;
            }

            void _checkMaxPositionRisk(  decltype(m_callPolicySymbols.targetSignal.targetPosMaps) & targetPosMaps, int type, int maxPosition){
                for(auto itrCTGPos = targetPosMaps.begin(); itrCTGPos!=targetPosMaps.end(); itrCTGPos++){

                    if(type == -1){ // pos must be negtive and less than -maxPosition
                        if( -maxPosition > itrCTGPos->second){
                            itrCTGPos->second = -maxPosition;
                        }else if(itrCTGPos->second >0){
                            itrCTGPos->second = 0;
                        }
                    }else if(type == 1){ // pos must be positive and less than maxPosition
                        if( maxPosition < itrCTGPos->second){
                            itrCTGPos->second = maxPosition;
                        }else if(itrCTGPos->second <0){
                            itrCTGPos->second = 0;
                        }
                    }else if(type ==0){  // abs(pos) <  maxPosition
                        if( -maxPosition > itrCTGPos->second && itrCTGPos->second <0){
                            itrCTGPos->second = -maxPosition;
                        }else if( maxPosition < itrCTGPos->second && itrCTGPos->second >0){
                            itrCTGPos->second = maxPosition;
                        }
                    }
                }
            }
    };
}
}


#endif //Cosmos_LPOLICY_H
