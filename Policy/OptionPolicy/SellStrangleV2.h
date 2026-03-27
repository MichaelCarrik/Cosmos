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

          //  int m_tradeTime{0};
            double m_MV{0.0};
            double m_multi{1000.0};
            bool m_isCheck{false};
            int m_lastPsTime{0};
            int m_maxPosition{0};
            int m_isRefreshDelta{0};
            double m_openAtDelta{0.25};
            double m_biasRation{0.01};
            double m_basePrice{0.0};
         //   double m_adjCoef{0.35};
       //      Types::Instrument_t  m_underlyInstrument{""};
        public:
            SellStrangleV2( Types::KPeriod kperiod, double mv,
                  std::string const&policyName, std::string const&engineName,
                  Types::Instrument_t& instrument,  double multi,
                  int tradingday, int maxPosition, int isRefreshDelta, double openAtDelta, double biasRation) :
                  m_MV(mv),
                    m_multi(multi), m_maxPosition(maxPosition), m_isRefreshDelta(isRefreshDelta), m_openAtDelta(openAtDelta),
                    m_biasRation(biasRation) {

                if(m_openAtDelta < 0.1 || m_openAtDelta > 0.5){
                    assert(false);
                }
                m_kperiod = kperiod;
                m_policyName = policyName;
                m_engineName = engineName;
                m_tradingday = tradingday;
                m_underlyInstrument = instrument;
                spdlog::info( "createPolicy engineName={}, policyName={}, kperiod={}  mv={}, tradingday={}, "
                              "underly={}, maxPosition={}, isRefreshDelta={}, openAtDelta={}, biasRation={}",
                        engineName, policyName, (int) kperiod, m_MV, m_tradingday, m_underlyInstrument.data(),
                              m_maxPosition, m_isRefreshDelta, m_openAtDelta, m_biasRation);

                _initPolicyLogger();
            }

            ~SellStrangleV2() {

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
                                char stpname[56]{"basePrice"};
                                _getValueInLine(buf, stpname, fileRead.basePriceStr);

                            }
                            fileReadVecs.emplace_back(fileRead);
                        }
                    }
                }
                fclose(fp);

            }

            virtual void start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) override {
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(), m_underlyInstrument.data());
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
                        m_basePrice = std::stof(fileReadItr.basePriceStr.c_str());
                    }
                }



                fprintf(stderr,
                        "[%s_%s] start, m_kperiod=%dMins, m_MV=%.3f, m_multi=%.3f, m_biasRation=%.3f, maxPosition=%d, isRefreshDelta=%d, "
                        "openAtDelta=%.3f, basePrice=%.3f, expireday=%d, configIndex=%d\n",
                        m_engineName.c_str(), m_policyName.c_str(), Types::KPeroidToIntervalMap[m_kperiod], m_MV, m_multi, m_biasRation,
                        m_maxPosition, m_isRefreshDelta, m_openAtDelta, m_basePrice,
                        m_expireday, m_configIndex);
                m_configIndex++;

            };



            virtual void runTick(const  Types::MarketData *pMD) override {

//                if(strcmp(pMD->instrumentID.data(), "IO2305-C-4000") ==0 ){
//                    fprintf(stderr,"runTick instrument=%s, updateTime=%s, psSecond=%d, m_lastPsTime=%d, "
//                                   "m_lastUnderlyBarIndex=%d, m_seriesIndex=%d\n",
//                            pMD->instrumentID.data(), pMD->updateTime.data(), pMD->psSecond, m_lastPsTime,
//                            m_lastUnderlyBarIndex, m_underlyKseries->m_seriesIndex);
//                }

                if(strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) ==0 ){

                    if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        m_isCheck = true;
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                        m_lastPsTime = pMD->psSecond;
                    }else if(m_isCheck == true &&  m_lastUnderlyBarIndex > 0){
                        if(pMD->psSecond - m_lastPsTime > 10){
                        //    fprintf(stderr,"instrument=%s, updateTime=%s\n", pMD->instrumentID.data(), pMD->updateTime.data());
                            auto lastUnderlyKB = m_underlyKseries->m_KDataVecs[m_underlyKseries->m_seriesIndex-1];

                            double allCallDelta = getAllHoldDelta(m_callPolicySymbols);
                            double allPutDelta = getAllHoldDelta(m_putPolicySymbols);

                            bool isZeroCall =  isHaveZeroDeltaPos(m_callPolicySymbols);
                            bool isZeroPut=  isHaveZeroDeltaPos(m_putPolicySymbols);

                            if (isZeroCall == true || isZeroPut== true) {
                                spdlog::info("policyName=%s, updateTime=%s, isZeroCall=%d, isZeroPut=%d\n",
                                    m_policyName.c_str(), pMD->updateTime.data(), isZeroCall, isZeroPut);
                            }

                            if( m_tradingday != m_expireday && (abs(lastUnderlyKB->m_close - m_basePrice) > m_basePrice * m_biasRation
                                  || (allCallDelta ==0 && allPutDelta == 0) || m_isRefreshDelta == 1) && isZeroCall==false && isZeroPut==false){
                                //rebalance;
                                _rebalanceDelta(m_callPolicySymbols, m_putPolicySymbols, allCallDelta, allPutDelta ,
                                                -m_MV/(lastUnderlyKB->m_close * m_multi), m_openAtDelta, lastUnderlyKB->m_close );
                                m_isRefreshDelta = 0;
                                m_basePrice = lastUnderlyKB->m_close;
                            }

                            allCallDelta = getAllHoldDelta(m_callPolicySymbols);
                            allPutDelta = getAllHoldDelta(m_putPolicySymbols);

                            _checkMaxPositionRisk(m_callPolicySymbols.optionTargetPosMaps, -1, m_maxPosition);
                            _checkMaxPositionRisk(m_putPolicySymbols.optionTargetPosMaps, -1, m_maxPosition);

                            m_configLog->info("configIndex={}, instrument={}, {}, {}, {}, close={:.3f}, delta={:.3f}, "
                                              "targetPosition={}, basePrice={:.3f}, seriesIndex={}, "
                                              "allCallDelta={:.3f}, allPutDelta={:.3f}",
                                              m_configIndex, lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingday,
                                              lastUnderlyKB->m_updateTimeBegin.data(), lastUnderlyKB->m_endPsTime, lastUnderlyKB->m_close,
                                              0.0, 0, m_basePrice, m_underlyKseries->m_seriesIndex-1, allCallDelta, allPutDelta);
                            _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                            _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);

                            m_configLog->flush();
                            m_configIndex ++;
                            m_isCheck = false;
                        }
                    }
                }
            }


        };
    }
}

#endif //OPTIONTRADING_SELLSTRANGLEV2_H
