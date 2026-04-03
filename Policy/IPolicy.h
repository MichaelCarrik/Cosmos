//
// Created by zhangyingwei on 2026/3/27.
//

#ifndef COSMOS_IPOLICY_H
#define COSMOS_IPOLICY_H
#include "../Types/Type.h"
#include "../Utils/LogUtils.h"
#include "../KData/KSeries.h"

namespace Cosmos {
    namespace Policy {

        struct TargetSignal {
            std::map<Types::Instrument_t, int> targetPosMaps;
            std::map<Types::Instrument_t, int> lastTargetPosMaps;
        };

        class IPolicy {
        public:
            std::string m_policyName{""};
            std::string m_engineName{""};
            Types::Instrument_t m_underlyInstrument{""};
            Types::KPeriod m_kperiod;
            const KData::KSeries *m_underlyKseries{nullptr};
            int m_lastUnderlyBarIndex{0};
            double m_MV;
            double  m_multi{0.0};
            int m_tradingDay{0};

            spdlog::logger *m_configLog{nullptr};

            IPolicy(
                  std::string const&policyName, std::string const&engineName, Types::Instrument_t& instrument,
                  Types::KPeriod kperiod, double mv, double multi, int tradingDay) :  m_policyName(policyName),
                    m_engineName(engineName), m_underlyInstrument(instrument), m_kperiod(kperiod), m_MV(mv),
                    m_multi(multi),  m_tradingDay(tradingDay){
                  }


            virtual void
            start(std::unordered_map<Types::Instrument_t, Types::Symbol *, Types::InstrumentHash> &) = 0;

            virtual void runTick(const Types::MarketData *pMD) = 0;

            virtual void updateParam(const Types::NetModifyParam*) = 0;

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
                std::string policyStr{"policy"};
                std::string fileName = m_policyName + "_" + std::string(m_underlyInstrument.data());
                m_configLog = Utils::initLogs(policyStr, m_engineName, fileName);
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

        };
    }
}
#endif //COSMOS_IPOLICY_H
