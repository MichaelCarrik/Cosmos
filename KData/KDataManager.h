//
// Created by zhangyw on 1/9/21.
//

#ifndef TESTHFT3_KDATA_H
#define TESTHFT3_KDATA_H


#include <array>
#include <map>
#include <unordered_map>
#include "../Utils/TradingHours.h"
#include <vector>
#include <string>
#include <cassert>
#include "../Utils/cppmysql.h"
#include "../Utils/Utils.h"
//#include "MockBase.h"
#include "../Types/Type.h"
#include "KSeries.h"
#include "../Types/KPeriod.h"
#include "../Driver/RealtimeDriver.h"
#include "UpdateOptionModelPamt.h"

namespace Cosmos {
    namespace KData {
        class KDataManager {
        public:
            Utils::CppMySQL3DB *m_mysql{nullptr};
            std::map<Types::Instrument_t, std::unordered_map<Types::KPeriod, KSeries *> *> m_allKLineSeries;
            //       std::map<std::tuple< Types::Instrument_t,  Types::KPeriod>, std::map<int, CalPutSeries *> *> m_underlyToOptionSeriesMap;
            int m_tradingday{0};
            bool m_isUseUnderlyPrice{true};
            bool m_isDay{true};
            bool m_isGetHistory{false};
            int m_biasSeconds{0};
            UpdateOptionModelPamt *m_updateOptionModelPamt{nullptr};

            KDataManager(int tradingDay, bool isDay, bool isUseUnderlyPrice,
                         Utils::CppMySQL3DB *mysql, int biasSeconds) :
            m_tradingday(tradingDay), m_isDay(isDay), m_isUseUnderlyPrice(isUseUnderlyPrice),
            m_mysql(mysql), m_biasSeconds(biasSeconds){
                m_updateOptionModelPamt = new UpdateOptionModelPamt();
            }


            KSeries *KMAddTick(const Types::MarketData *pMD, Types::KPeriod period);
            void KMAddTick(const Types::MarketData *pMD);
            KSeries *getSeries(Types::Instrument_t const &instrument, Types::KPeriod period);
            void getHisKbars(Types::Instrument_t const &instrument, Types::ProductClass productClass,
                             Types::KPeriod period, std::vector<KData *> &hisKdata,
                             int tradingday, bool isReal, bool isDay);
            void initKSeries(Types::InstrumentInfo const &insInfo, Types::KPeriod period,
                             int tradingday, double riskFreeR, std::vector<KData *> &historyKline, bool isDay);
            void _initUnderlyToOptionSeriesMap(KSeries *optionKSeries, Types::KPeriod const &kperiod);

            double _calForwardPrice(const KSeries *underlySeries);

            void checkSeriesRecord(KSeries *series,  int lastSeriesIndex);
        };
    }
}
#endif //TESTHFT3_KDATA_H
