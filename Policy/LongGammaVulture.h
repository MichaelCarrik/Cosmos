//
// Created by zhangyingwei on 2024/10/29.
//

#ifndef OPTIONTRADING_LONGGAMMAVULTURE_H
#define OPTIONTRADING_LONGGAMMAVULTURE_H

#include "IOptionPolicy.h"
#include "../Types/Type.h"
#include "../Utils/Indicator.h"
#include <vector>

namespace Cosmos {
    namespace Policy {

        class LongGammaVulture : public IOptionPolicy {
        private:
            int m_configIndex{0};
            bool m_isCheck{false};
            double m_mv{0.0};
            double m_multi{1000.0};
            int m_lastPsTime{0};
            int m_maxPosition{0};
            double m_openAtDelta{0.25};
            int m_tradeNum{0};
            int m_marketPosition{0};
            int m_preMarketPosition{0};
            double m_holdStrikePrice{0.0};
            double m_signalPrice{0.0};

            const KData::KSeries *m_dayKSeries{nullptr};

            std::vector<double> m_dayHighestVec;
            std::vector<double> m_dayLowestVec;
            std::vector<double> m_dayCloseVec;
            std::vector<double> m_fiveMinCloseVec;

            double m_band;
            double m_upBand;
            double m_downBand;
            double m_minsMA;
            int m_dayMinutesCount{0};
            int m_onoff{0};
            int m_length{0};
            double m_alpha{0.7};
            double m_mark{2.0};

        public:
            LongGammaVulture( Types::KPeriod kperiod, double mv, std::string const& policyName,
                         std::string &engineName,
                         Types::Instrument_t &instrument, double multi, int tradingday, int maxPosition,
                         double openAtDelta) :
                    m_mv(mv), m_multi(multi), m_maxPosition(maxPosition), m_openAtDelta(openAtDelta){

                m_underlyInstrument = instrument;

                if (m_openAtDelta < 0.1 || m_openAtDelta > 0.5) {
                    assert(false);
                }
                m_kperiod = kperiod;
                m_policyName = policyName;
                m_engineName = engineName;
                m_tradingday = tradingday;
                spdlog::info("createPolicy engineName={}, policyName={}, kperiod={}  mv={}, tradingday={}, "
                             "underly={}, , maxPosition={}, openAtDelta={}",
                             engineName, policyName, (int) kperiod, m_mv, m_tradingday, m_underlyInstrument.data(),
                             m_maxPosition, m_openAtDelta);
                _initPolicyLogger();
            }

            ~LongGammaVulture() {}

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
                _initVulture();
            }

            void _updateVultureSignal(const KData::KData *lastUnderlyBar) {

                int beginI = 0, endI = 0;
                if (m_lastUnderlyBarIndex < m_length) {
                    beginI = 0;
                    endI = m_lastUnderlyBarIndex;

                } else {
                    beginI = m_lastUnderlyBarIndex - m_length + 1;
                    endI = m_lastUnderlyBarIndex ;
                }
                m_fiveMinCloseVec.emplace_back(lastUnderlyBar->m_close);
                m_minsMA = Indicator::MA(m_fiveMinCloseVec, beginI, endI);

            }

            void _initVulture() {
                 for (auto i = 0; i < m_dayKSeries->m_seriesIndex; i++) {
                        auto dayBar = m_dayKSeries->m_KDataVecs[i];
                        if (dayBar->m_tradingday < m_tradingday) {
                            m_dayHighestVec.emplace_back(dayBar->m_high);
                            m_dayLowestVec.emplace_back(dayBar->m_low);
                            m_dayCloseVec.emplace_back(dayBar->m_close);
                        }
                    }

                    double highest = 9999.0, lowest = 0.0;
                    if (m_dayHighestVec.size() > 0) {
                        if (m_dayHighestVec.size() < m_mark) {
                            highest = *std::max_element(std::begin(m_dayHighestVec), std::end(m_dayHighestVec));
                            lowest = *std::min_element(std::begin(m_dayLowestVec), std::end(m_dayLowestVec));
                        } else {
                            highest = *std::max_element(std::end(m_dayHighestVec) - m_mark, std::end(m_dayHighestVec));
                            lowest = *std::min_element(std::end(m_dayLowestVec) - m_mark, std::end(m_dayLowestVec));
                        }
                        m_band = (highest - lowest) * m_alpha / m_mark;
                        double dayClose = m_dayCloseVec[m_dayCloseVec.size() - 1];
                        //	double dayClose = m_dayKSeries->m_kseries[m_dayKSeries->m_seriesIndex-1]->m_close;
                        m_upBand = dayClose + m_band;
                        m_downBand = dayClose - m_band;
                    } else {
                        m_band = (highest + lowest) * 0.5;
                        m_upBand = highest;
                        m_downBand = lowest;
                    }

                    for (auto i = 0; i < m_underlyKseries->m_seriesIndex; i++) {
                        m_fiveMinCloseVec.emplace_back(m_underlyKseries->m_KDataVecs[i]->m_close);
                    }

                    int beginI = 0, endI = 0;
                    if (m_underlyKseries->m_seriesIndex < m_length) {
                        beginI = 0;
                        endI = m_underlyKseries->m_seriesIndex-1;

                    } else {
                        beginI = m_underlyKseries->m_seriesIndex - m_length ;
                        endI = m_underlyKseries->m_seriesIndex -1;
                    }

                    m_minsMA = Indicator::MA(m_fiveMinCloseVec, beginI, endI);
            }

            virtual void start(std::unordered_map< Types::Instrument_t,  Types::Symbol *,  Types::InstrumentHash> &inputSymbolMap) override {

                Types::Product_t product{""};
                Utils::InstrumentToProduct(m_underlyInstrument, product);
                m_dayMinutesCount = Utils::TradingHours::getDayMinutesCount(product);
                m_length = int(m_dayMinutesCount / 5 ) * 15;

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

                m_dayKSeries = symbolItr->second->m_kSeriesMap.at(Types::KPeriod::D1);

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

            //    _writePolicyLog(lastUnderlyBar,lastSAR);

                fprintf(stderr,
                        "[%s_%s] start, m_kperiod=%dMins, m_mv=%.3f, m_multi=%.3f, maxPosition=%d, "
                        "openAtDelta=%.3f, m_length=%d, m_mark=%.3f, m_alpha=%.3f, expireday=%d, "
                        "marketPosition=%d, preMarketPostion=%d, signalPrice=%.3f, holdStrikePrice=%.3f, "
                        "configIndex=%d\n", m_engineName.c_str(), m_policyName.c_str(), Types::KPeroidToIntervalMap[m_kperiod],
                        m_mv, m_multi,m_maxPosition, m_openAtDelta, m_length, m_mark, m_alpha, m_expireday, m_marketPosition,
                        m_preMarketPosition,m_signalPrice, m_holdStrikePrice ,m_configIndex);
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
                        _updateVultureSignal(lastUnderlyBar);

                        if(m_tradingday  != m_expireday){
                            _isCloseMarketPosition(lastUnderlyBar);
                            _marketPosToOptionPos(lastUnderlyBar, m_marketPosition, m_preMarketPosition);
                            m_preMarketPosition = m_marketPosition;

                            _isOpenMarketPosition(lastUnderlyBar);
                            _marketPosToOptionPos(lastUnderlyBar, m_marketPosition, m_preMarketPosition);
                            m_preMarketPosition = m_marketPosition;

                            _checkMaxPositionRisk(m_callPolicySymbols.optionTargetPosMaps, 1, m_maxPosition);
                            _checkMaxPositionRisk(m_putPolicySymbols.optionTargetPosMaps, 1, m_maxPosition);

                        }

                        _writePolicyLog(lastUnderlyBar);
                        m_configLog->flush();
                        m_configIndex++;
                        m_isCheck = false;
                        m_preMarketPosition = m_marketPosition;
                    }
                }
            };

            void _writePolicyLog(const KData::KData *lastUnderlyKB) {

                m_configLog->info("configIndex={}, instrument={}, {}, {}, {}, close={:.3f}, "
                                  "marketPosition={}, preMarketPosition={}, signalPrice={:.3f}, "
                                  "holdStrikePrice={:.3f}, m_upBand={:.3f}, m_downBand={:.3f}",
                                  m_configIndex, lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingday,
                                  lastUnderlyKB->m_updateTime.data(), lastUnderlyKB->m_psTime, lastUnderlyKB->m_close,
                                  m_marketPosition, m_preMarketPosition, m_signalPrice, m_holdStrikePrice, m_upBand, m_downBand);
                _writeOptionPolicyLog(m_callPolicySymbols, m_configIndex);
                _writeOptionPolicyLog(m_putPolicySymbols, m_configIndex);
            }


            void _isCloseMarketPosition(const KData::KData *lastUnderlyBar) {
                if (m_marketPosition > 0 && lastUnderlyBar->m_close >= m_holdStrikePrice ) {  //long close
                    ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_holdStrikePrice = 0.0;
                } else if (m_marketPosition < 0 && lastUnderlyBar->m_close <= m_holdStrikePrice ) {  //short close
                    ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_holdStrikePrice = 0.0;
                }
            }

            void _isOpenMarketPosition(const KData::KData *lastUnderlyBar) {
                if (lastUnderlyBar->m_close >= m_minsMA) {
                    m_onoff = 1;
                } else if (lastUnderlyBar->m_close <= m_minsMA) {
                    m_onoff = -1;
                }

                if (m_marketPosition > 0 && lastUnderlyBar->m_close <= m_downBand) {  //long close
                    //   ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_onoff = 0;
                } else if (m_marketPosition < 0 && lastUnderlyBar->m_close >= m_upBand) {  //short close
                    //    ++m_tradeNum;
                    m_marketPosition = 0;
                    m_signalPrice = lastUnderlyBar->m_close;
                    m_onoff = 0;
                }

                if ( m_marketPosition == 0) {
                    if (lastUnderlyBar->m_close >= m_upBand && lastUnderlyBar->m_close > lastUnderlyBar->m_open &&
                        m_onoff == 1) //long open
                    {
                        m_marketPosition = 1;
                        m_signalPrice = lastUnderlyBar->m_close;
                        m_onoff = 0;
                    } else if (lastUnderlyBar->m_close <= m_downBand && lastUnderlyBar->m_close < lastUnderlyBar->m_open &&
                               m_onoff == -1)//short open
                    {
                        m_marketPosition = -1;
                        m_signalPrice = lastUnderlyBar->m_close;
                        m_onoff = 0;
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

#endif //OPTIONTRADING_LONGGAMMAVULTURE_H
