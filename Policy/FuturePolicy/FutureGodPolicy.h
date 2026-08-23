//
// Created by zhangyingwei on 2026/8/23.
//

#ifndef COSMOS_FUTUREGODPOLICY_H
#define COSMOS_FUTUREGODPOLICY_H


#include "IFuturePolicy.h"

namespace Cosmos {
    namespace Policy {
        class FutureGodPolicy : public IFuturePolicy {
        private:
            Types::Instrument_t m_instrumentB{""};
            const KData::KSeries *m_instrumentBKseries{nullptr};

            int m_configIndex{0};
            int m_targetPosA{0};
            int m_targetPosB{0};
            int m_tradeNum{0};

        public:
            FutureGodPolicy(std::string const &policyName, std::string const &engineName,
                            Types::Instrument_t &instrumentA,
                            Types::Instrument_t &instrumentB, int targetPosA, int targetPosB, Types::KPeriod kperiod,
                            double mv,
                            double multi, int tradingDay, int adjRiskTime) : IFuturePolicy(policyName, engineName,
                                                                                 instrumentA, kperiod,
                                                                                 mv, multi, tradingDay, adjRiskTime),
                                                                             m_instrumentB(instrumentB),
                                                                             m_targetPosA(targetPosA),
                                                                             m_targetPosB(targetPosB) {
                _initPolicyLogger();
            }

            ~FutureGodPolicy() {
            }

            virtual void updateParam(const Types::NetModifyParam *netModifyParam) override {
            };

            virtual void start(
                std::unordered_map<Types::Instrument_t, Types::Symbol *, Types::InstrumentHash> &
                inputSymbolMap) override {
                Types::Product_t product{""};
                Utils::InstrumentToProduct(m_underlyInstrument, product);


                m_underlyKseries = getSeries(inputSymbolMap, m_kperiod, m_underlyInstrument);
                m_instrumentBKseries = getSeries(inputSymbolMap, m_kperiod, m_underlyInstrument);

                initGodSignalPos(inputSymbolMap, m_engineName, m_policyName, m_underlyInstrument);


                fprintf(
                    stderr,
                    "createPolicy policyName=%s, engineName=%s, instrument=%s  kperiod=%d, mv=%.3f, multi=%.3f, tradingDay=%d, "
                    "signalPrice=%.3f, marketPosition=%d, targetPosA=%d, preTargetPosA=%d, targetPosB=%d, preTargetPosB=%d\n",
                    m_policyName.c_str(), m_engineName.c_str(), m_underlyInstrument.data(), static_cast<int>(m_kperiod),
                    m_MV, m_multi, m_tradingDay,
                    m_trendSignal.signalPrice, m_trendSignal.marketPosition,
                    m_targetSignal.targetPosMaps[m_underlyInstrument],
                    m_targetSignal.lastTargetPosMaps[m_underlyInstrument], m_targetSignal.targetPosMaps[m_instrumentB],
                    m_targetSignal.lastTargetPosMaps[m_instrumentB]);

                spdlog::info(
                    "createPolicy policyName={}, engineName={}, instrument={}  kperiod={}, mv={:.3f}, multi={:.3f}, tradingDay={}, "
                    "signalPrice={:.3f}, marketPosition={}, "
                    "targetPosA=%d, preTargetPosA=%d, targetPosB=%d, preTargetPosB=%d", m_policyName.c_str(),
                    m_engineName.c_str(), m_underlyInstrument.data(),
                    static_cast<int>(m_kperiod), m_MV, m_multi, m_tradingDay, m_trendSignal.signalPrice,
                    m_trendSignal.marketPosition,
                    m_targetSignal.targetPosMaps[m_underlyInstrument],
                    m_targetSignal.lastTargetPosMaps[m_underlyInstrument],
                    m_targetSignal.targetPosMaps[m_instrumentB], m_targetSignal.lastTargetPosMaps[m_instrumentB]);
            };

            void initGodSignalPos(std::unordered_map<Types::Instrument_t, Types::Symbol *, Types::InstrumentHash> &
                                  inputSymbolMap, std::string const &engineName, std::string const &policyName,
                                  Types::Instrument_t const &underlyInstrument) {
                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s_%s.txt", m_engineName.c_str(), m_policyName.c_str(),
                        underlyInstrument.data());
                // m_trendSignal.signalPrice = atof(this->GetLastValueFromFile(configPath, "sgnPrice").c_str());
                // m_trendSignal.marketPosition = atoi(this->GetLastValueFromFile(configPath, "mktPos").c_str());
                // m_targetSignal.targetPosMaps[m_underlyInstrument] = atoi(this->GetLastValueFromFile(configPath, "tgtPos").c_str());
                // m_targetSignal.lastTargetPosMaps[m_underlyInstrument] =  atoi(this->GetLastValueFromFile(configPath, "tgtPos").c_str());
                m_configIndex = atoi(this->GetLastValueFromFile(configPath, "configIndex").c_str());
                std::vector<FileRead> fileReadVecs;
                _GetValueFromFileByConfigIndex(configPath, m_underlyInstrument, m_configIndex, fileReadVecs);

                for (auto symbolItr: inputSymbolMap) {
                    if (symbolItr.second->instrumentInfo.productIDClass == Types::ProductClass::future &&
                        (strcmp(symbolItr.second->instrumentInfo.underly.data(), underlyInstrument.data()) == 0 ||
                         strcmp(symbolItr.second->instrumentInfo.underly.data(), m_instrumentB.data()) == 0
                        )) {
                        for (auto fileReadItr: fileReadVecs) {
                            if (strcmp(fileReadItr.instrumentStr.c_str(),
                                       symbolItr.second->instrumentInfo.instrumentID.data()) == 0) {
                                m_targetSignal.targetPosMaps[symbolItr.second->instrumentInfo.instrumentID] = std::stoi(
                                    fileReadItr.targetPositionStr.c_str());
                                m_targetSignal.lastTargetPosMaps[symbolItr.second->instrumentInfo.instrumentID] =
                                        std::stoi(fileReadItr.targetPositionStr.c_str());
                            }
                        }
                    }
                }
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

            virtual void runTick(const Types::MarketData *pMD) override {
                if (strcmp(pMD->instrumentID.data(), m_underlyInstrument.data()) == 0) {
                    if (m_lastUnderlyBarIndex == 0 and m_underlyKseries->m_seriesIndex > m_lastUnderlyBarIndex) {
                    } else if (m_lastUnderlyBarIndex < m_underlyKseries->m_seriesIndex) {
                        m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex - 1;

                        auto lastBarA = m_underlyKseries->m_KDataVecs[m_lastUnderlyBarIndex];
                        auto lastBarB = m_instrumentBKseries->m_KDataVecs[m_instrumentBKseries->m_seriesIndex - 1];

                        m_targetSignal.targetPosMaps[m_underlyInstrument] = m_targetPosA;
                        m_targetSignal.targetPosMaps[m_instrumentB] = m_targetPosB;


                        writePolicyLog(lastBarA, m_underlyKseries->m_lastPMD);
                        writePolicyLog(lastBarB, m_instrumentBKseries->m_lastPMD);
                        m_targetSignal.lastTargetPosMaps.clear();
                        std::copy(m_targetSignal.targetPosMaps.begin(), m_targetSignal.targetPosMaps.end(),
                                  std::inserter(m_targetSignal.lastTargetPosMaps,
                                                m_targetSignal.lastTargetPosMaps.begin()));
                    }
                    m_lastUnderlyBarIndex = m_underlyKseries->m_seriesIndex;
                }
            };

            virtual void writePolicyLog(const KData::KData *lastUnderlyKB, const Types::MarketData *pMD) override {
                m_configLog->info("configIndex={}, {}, {}, {}, {}, {:03d}, close={:.3f}({:.3f}, {:.3f}), "
                                  "sgnPrice={:.3f}, tgtPos={}",
                                  m_configIndex, lastUnderlyKB->m_instrument.data(), lastUnderlyKB->m_tradingDay,
                                  lastUnderlyKB->m_updateTimeBegin.data(), pMD->updateTime.data(), pMD->milliSeconds,
                                  lastUnderlyKB->m_close, pMD->bidPrice[0], pMD->askPrice[0], m_trendSignal.signalPrice,
                                  m_targetSignal.targetPosMaps[m_underlyInstrument]
                );
                m_configLog->flush();
            }
        };
    };
}

#endif //COSMOS_FUTUREGODPOLICY_H
