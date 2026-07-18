//
// Created by zhangyw on 1/9/21.
//

#ifndef COSMOS_KDATAMANAGER_H
#define COSMOS_KDATAMANAGER_H


#include <array>
#include <map>
#include <unordered_map>
#include <vector>
#include "../Utils/cppmysql.h"
#include "../Utils/Utils.h"

#include "../Types/Type.h"
#include "KAmountSeries.h"
#include "../Types/KPeriod.h"
#include "../Driver/RealtimeDriver.h"


namespace Cosmos {
    namespace KAmountBarEngine {
        class KAmountManager {
        public:
            Utils::CppMySQL3DB *m_mysql{nullptr};
            std::map<Types::Instrument_t, std::unordered_map<Types::KPeriod, KAmountSeries* > *> m_allKLineSeries;
            int m_tradingDay{0};

            bool m_isDay{true};
            bool m_isGetHistory{false};

            KAmountManager(int tradingDay, bool isDay,
                         Utils::CppMySQL3DB *mysql) :
            m_tradingDay(tradingDay), m_isDay(isDay),m_mysql(mysql){

            }


            KAmountSeries *KMAddTick(const Types::MarketData *pMD, Types::KPeriod period);
            void KMAddTick(const Types::MarketData *pMD);
            KAmountSeries *getSeries(Types::Instrument_t const &instrument, Types::KPeriod period);
            void getHisKbars(Types::Instrument_t const &instrument, Types::ProductClass productClass,
                             Types::KPeriod period, std::vector<KAmountData *> &hisKdata,
                             int tradingday, bool isReal, bool isDay);
            void initKSeries(Types::InstrumentInfo const &insInfo, Types::KPeriod period,
                             int tradingday, double riskFreeR, std::vector<KAmountData *> &historyKline,
                             bool isDay, double);

            void checkSeriesRecord(KAmountSeries *series,  int lastSeriesIndex);
        };
    }
}
#endif //COSMOS_KDATAMANAGER_H
