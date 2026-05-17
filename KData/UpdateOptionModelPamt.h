//
// Created by Zhangyingwei on 2023/2/24.
//

#ifndef OPTIONTRADING_UPDATEGREEKS_H
#define OPTIONTRADING_UPDATEGREEKS_H

#include "../Types/Param.h"
#include "../Types/KPeriod.h"
#include "KSeries.h"

namespace Cosmos {
    namespace KData {
        class UpdateOptionModelPamt {

            std::map<std::tuple< Types::Instrument_t,  Types::KPeriod>, std::vector<KSeries *> *> m_underlyToOptionSeriesMap;
            std::map< std::tuple< Types::Instrument_t,  Types::KPeriod>, int> m_underlyTodayBeginIndexMap;

        public:
            UpdateOptionModelPamt() {

            }
            void  init(KSeries *series, Types::KPeriod const& period,int tradingDay, bool m_isDay) ;

            void fillOptionSeries(KSeries * optionSeries, int updateUnderlyIndex, int updateOptionIndex) ;

            int getUnderlyTodayBeginIndex(Types::Instrument_t const& instrumentID, Types::KPeriod period) ;

            template<class T>
            double getKFairClose(T *kdata) {
                if (kdata->m_bidVolume > 0 && kdata->m_askVolume > 0) {
                    return (kdata->m_bidPrice + kdata->m_askPrice) * 0.5;
                } else {
                    return kdata->m_close;
                }
            }

            void updateGreeks(KSeries* underlySeries, double forwardPrice,  int lastSeriesIndex);
            void updateSabr(KSeries* underlySeries, double forwardPrice, int);
        };

    }
}

#endif //OPTIONTRADING_UPDATEGREEKS_H
