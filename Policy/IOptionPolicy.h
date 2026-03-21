//
// Created by zhangyw on 2/2/21.
//

#ifndef TRENDFOLLOW_LPOLICY_H
#define TRENDFOLLOW_LPOLICY_H

#include "../Types/Symbol.h"


namespace TrendFollow {
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
            std::map<Types::Instrument_t, int> optionTargetPosMaps;
            std::map<Types::Instrument_t, int> optionLastTargetPosMaps;
        };

        class IOptionPolicy {

        public:
            std::string m_policyName{""};
            std::string m_engineName{""};
            int m_configIndex{0};
             Types::KPeriod m_kperiod;
            const KData::KSeries *m_underlyKseries{nullptr};
            int m_lastUnderlyBarIndex{0};

            PolicySymbolStruct m_callPolicySymbols;
            PolicySymbolStruct m_putPolicySymbols;

            int m_tradingday{0};
            int m_expireday{0};
             Types::Instrument_t m_underlyInstrument{""};

             Types::SignalIntension m_intension{Types::SignalIntension::put};
            std::shared_ptr<spdlog::logger> m_configLog;

            virtual void
            start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &m_symbolNearMap) = 0;

            virtual void runTick(const  Types::MarketData *pMD) = 0;

            int _initOptionPolicySymbolVecs(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap,
                                       Types::Instrument_t const& underlyInstrument, PolicySymbolStruct & policySymbols,
                                       std::vector<FileRead>const& fileReadVecs, char optionType ){
                int expireDay =0;
                for(auto symbolItr : inputSymbolMap){
                    if ( symbolItr.second->instrumentInfo.productIDClass == Types::ProductClass::option &&
                          symbolItr.second->instrumentInfo.optionType == optionType &&
                            strcmp(symbolItr.second->instrumentInfo.underly.data(), underlyInstrument.data()) ==0)
                    {
                        expireDay = symbolItr.second->instrumentInfo.expireDate;
                        policySymbols.optionSymbolVecs.emplace_back(symbolItr.second);
                        for(auto fileReadItr : fileReadVecs){
                            if(strcmp(fileReadItr.instrumentStr.c_str(), symbolItr.second->instrumentInfo.instrumentID.data())==0){
                                policySymbols.optionTargetPosMaps[symbolItr.second->instrumentInfo.instrumentID] = std::stoi(fileReadItr.targetPositionStr.c_str());
                                policySymbols.optionLastTargetPosMaps[symbolItr.second->instrumentInfo.instrumentID] = std::stoi(fileReadItr.targetPositionStr.c_str());
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


                return expireDay;

            }

            void _getValueInLine(char *buf, char *valueName, std::string &value) {
                char *field = strstr(buf, valueName);

                if (field) {
                    std::string substr = field + strlen(valueName) + 1;
                    auto index = substr.find_first_of(',', 0);
                    if (index != std::string::npos) {
                        value = substr.substr(0, index).c_str();
                    } else {
                        value = substr.c_str();
                    }
                }
            }

            void _initPolicyLogger() {
                char logSymbolPath[128];
                memset(logSymbolPath, 0, sizeof(128));
                sprintf(logSymbolPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());

                std::filesystem::path tradeSymbolPath(logSymbolPath);
                if (!std::filesystem::exists(tradeSymbolPath.parent_path())) {
                    std::filesystem::create_directories(tradeSymbolPath.parent_path());
                }
                char logKey[128]{""};
                sprintf(logKey, ".policy_%s_%s_%s", m_engineName.c_str(), m_policyName.c_str(),
                        m_underlyInstrument.data());
                fprintf(stderr, "initLogs %s %s\n", logKey, logSymbolPath);

                m_configLog = spdlog::basic_logger_st(logKey, logSymbolPath);
            }

            std::string GetLastValueFromFile(char *filename, const char *valuename) {
                char buf[BUFSIZ], *field;
                std::string value{""};
                FILE *fp = NULL;
                fp = fopen(filename, "r");
                if (fp == NULL) {
                    printf("OnStarted, Warning: Cannot open file: %s !!!\n", filename);
                    return 0;
                } else {
                    while (fgets(buf, BUFSIZ, fp) != NULL) {
                        //  printf("buf : %s",buf);
                        field = strstr(buf, valuename);
                        if (field) {
                            std::string substr = field + strlen(valuename) + 1;
                            auto index = substr.find_first_of(',', 0);
                            if (index != std::string::npos) {
                                value = substr.substr(0, index);
                            } else {
                                value = substr;
                            }
                        }
                    }
                }
                fclose(fp);
                return value;
            }

            bool isHaveZeroDeltaPos(PolicySymbolStruct & policySymbols) {
                for (auto & optionSymbol: policySymbols.optionSymbolVecs) {
                    int  targetPosition =  getTargetPos(optionSymbol->instrumentInfo.instrumentID, policySymbols.optionTargetPosMaps);
                    if (targetPosition !=0) {
                        auto series = optionSymbol->m_kSeriesMap.at(m_kperiod);
                        auto lastIndex = series->m_seriesIndex > 0 ? series->m_seriesIndex - 1 : 0;
                        auto lastOptionK = series->m_KDataVecs[lastIndex];
                        if ( abs(lastOptionK->delta) < 0.00001 && optionSymbol->lastMD->askPrice[0] > 10* optionSymbol->instrumentInfo.tickSize) {
                            fprintf(stderr, "policyName=%s, instrument=%s, updateTime=%s, pos=%d,  delta=%.3f\n", m_policyName.c_str(), optionSymbol->instrumentInfo.instrumentID.data(),
                                lastOptionK->m_updateTime.data(), targetPosition, lastOptionK->delta);
                            return true;
                        }
                    }
                }
                return false;
            }

            void _writeOptionPolicyLog( PolicySymbolStruct & policySymbols, int configIndex) {
                for (auto & optionSymbol: policySymbols.optionSymbolVecs) {
                    int  targetPosition =  getTargetPos(optionSymbol->instrumentInfo.instrumentID, policySymbols.optionTargetPosMaps);
                    int lastTargetPosition  = getTargetPos(optionSymbol->instrumentInfo.instrumentID, policySymbols.optionLastTargetPosMaps);
                    if(targetPosition !=0 || (targetPosition ==0 && lastTargetPosition!=0)) {


                        auto series = optionSymbol->m_kSeriesMap.at(m_kperiod);
                        auto lastIndex = series->m_seriesIndex > 0 ? series->m_seriesIndex - 1 : 0;
                        auto lastOptionK = series->m_KDataVecs[lastIndex];

//                        if(lastOptionK->m_tradingday ==0){
//                            assert(false);
//                        }
                        m_configLog->info(
                                "configIndex={}, instrument={}, {}, {}, {}, close={:.3f}, delta={:.3f}, "
                                "targetPosition={}, seriesIndex={}",
                                configIndex, optionSymbol->instrumentInfo.instrumentID.data(), lastOptionK->m_tradingday,
                                lastOptionK->m_updateTime.data(), lastOptionK->m_psTime,
                                lastOptionK->m_close, lastOptionK->delta, targetPosition,
                                lastIndex);
                    }
                }
                policySymbols.optionLastTargetPosMaps.clear();
                std::copy(policySymbols.optionTargetPosMaps.begin(), policySymbols.optionTargetPosMaps.end(),
                          std::inserter(policySymbols.optionLastTargetPosMaps, policySymbols.optionLastTargetPosMaps.begin()));
            //    memcpy(&lastTargetPosMaps, &targetPosMaps, sizeof(targetPosMaps));
            }

            int getTargetPos(Types::Instrument_t const instrumentID,
                             decltype(m_callPolicySymbols.optionTargetPosMaps) const& targetPosMaps) {
                int targetPosition = 0;
                auto itrTGPos = targetPosMaps.find(instrumentID);
                if (itrTGPos != targetPosMaps.end()) {
                    targetPosition = itrTGPos->second;
                }
                return targetPosition;
            }


            const Types::Symbol* getApproxiDeltaSymbol(decltype(m_callPolicySymbols.optionSymbolVecs) const& symbolCallVecs,
                                                       double findAtDelta , char optionType, double undelyClose){
                std::vector<const Types::Symbol *> filterSymbolVecs;
                int itmIndex =0 ;
                // for (auto symbolItr: symbolCallVecs) {
                for (auto itr = symbolCallVecs.rbegin(); itr != symbolCallVecs.rend(); itr++) {
                    auto symbolItr = *itr;
                    if (symbolItr->m_kSeriesMap.at(m_kperiod)->m_seriesIndex > 0) {
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
                    }
                }
                if (filterSymbolVecs.size() > 0) {
                    auto specialDeltaItr = std::min_element(filterSymbolVecs.begin(), filterSymbolVecs.end(),
                                                       [findAtDelta, this](auto &symbol_a, auto &symbol_b) {
                                                           auto kbar_a = symbol_a->m_kSeriesMap.at(
                                                                   m_kperiod)->m_KDataVecs[symbol_a->m_kSeriesMap.at(
                                                                   m_kperiod)->m_seriesIndex - 1];
                                                           auto kbar_b = symbol_b->m_kSeriesMap.at(
                                                                   m_kperiod)->m_KDataVecs[symbol_b->m_kSeriesMap.at(
                                                                   m_kperiod)->m_seriesIndex - 1];
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


            int addPositionByGreeks(Types::Instrument_t const& instrumentID, decltype(m_callPolicySymbols.optionTargetPosMaps) & targetPosMaps, double greeks, double diffGreeks){
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

            void _checkMaxPositionRisk(  decltype(m_callPolicySymbols.optionTargetPosMaps) & targetPosMaps, int type, int maxPosition){
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


#endif //TRENDFOLLOW_LPOLICY_H
