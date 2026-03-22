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


        class UpdateGreeks {
            std::map<std::tuple< Types::Instrument_t,  Types::KPeriod>, int *> *m_allKLineIndexs;
            std::map<std::tuple< Types::Instrument_t,  Types::KPeriod>, std::vector<KSeries *> *> m_underlyToOptionSeriesMap;
            std::map< Types::KPeriod, int> m_underlyTodayBeginIndexMap;

        public:
            UpdateGreeks() {
                m_allKLineIndexs = new std::map<std::tuple< Types::Instrument_t,  Types::KPeriod>, int *>();
            }
            void  init(KSeries *series, Types::KPeriod const& period,int tradingDay, bool m_isDay) {

                        std::tuple< Types::Instrument_t,  Types::KPeriod> key = std::make_tuple(
                                series->m_insInfo.instrumentID, period);
                        m_allKLineIndexs->insert(std::make_pair(key, new int(series->m_seriesIndex)));

                        if (series->m_insInfo.productIDClass == Types::ProductClass::option) {
                            auto optionSeries = series;
                            auto keyUnderly = std::make_pair(optionSeries->m_insInfo.underly, period);

                            auto underlyTodayBeginIndexItr = m_underlyTodayBeginIndexMap.find(period);
                            if (underlyTodayBeginIndexItr == m_underlyTodayBeginIndexMap.end()) {
                                m_underlyTodayBeginIndexMap[period] = 0;
                            }

                            auto itrutom = m_underlyToOptionSeriesMap.find(keyUnderly);
                            if (itrutom == m_underlyToOptionSeriesMap.end()) {
                                std::vector<KSeries *> *temp = new std::vector<KSeries *>();
                                m_underlyToOptionSeriesMap.insert(std::make_pair(keyUnderly, temp));
                                itrutom = m_underlyToOptionSeriesMap.find(keyUnderly);
                                for (auto i = 0; i < optionSeries->m_underlySeries->m_seriesIndex; i++) {
                                    if (optionSeries->m_underlySeries->m_KDataVecs[i]->m_tradingday == tradingDay &&m_isDay == false) {
                                        break;
                                    }else if (optionSeries->m_underlySeries->m_KDataVecs[i]->m_tradingday ==
                                               tradingDay && m_isDay == true &&
                                               Utils::ToPsSeconds( optionSeries->m_underlySeries->m_KDataVecs[i]->m_updateTimeBegin, true) >
                                                                   Utils::DayBegin) {
                                        break;
                                    }

                                    m_underlyTodayBeginIndexMap[period] = i;
                                }

                                if(m_underlyTodayBeginIndexMap[period] >0){
                                    m_underlyTodayBeginIndexMap[period] = m_underlyTodayBeginIndexMap[period]+1;
                                }
                            }
                            itrutom->second->emplace_back(optionSeries);
                        }
            }

            void updateSingleOptionGreeks(KSeries *optionSeries, double forwardPrice, int updateUnderlyIndex, int updateOptionIndex) {

                 if (optionSeries->m_lastPMD == nullptr) {
                     return;
                 }

                int updateBeginPsTime = optionSeries->m_underlySeries->m_KDataVecs[updateUnderlyIndex]->m_beginPsTime;
                if (optionSeries->m_KDataVecs[updateOptionIndex]->m_tradingday == 0) {
                    optionSeries->ffill(updateBeginPsTime);
                }

                optionSeries->calGreeks(updateUnderlyIndex, updateOptionIndex, forwardPrice, updateBeginPsTime);

            }

            void updateGreeks(KSeries* underlySeries, double forwardPrice, std::map<int, CallPutSeries *>* calPutMap, int lastSeriesIndex) {

                auto underlyTodayBeginIndexItr = m_underlyTodayBeginIndexMap.find(underlySeries->m_Period);
                if (underlyTodayBeginIndexItr == m_underlyTodayBeginIndexMap.end()) {
                    fprintf(stderr, "instrument=%s\n", underlySeries->m_insInfo.instrumentID.data());
                    assert(false);
                }


                if ( strcmp(underlySeries->m_lastPMD->instrumentID.data() , "IM2606") ==0 ) {
                    int a = 1;
                }


                int updateOptionIndex = lastSeriesIndex - underlyTodayBeginIndexItr->second;

                for (auto & calPutItr : *calPutMap) {
                    updateSingleOptionGreeks(calPutItr.second->callSeries, forwardPrice, lastSeriesIndex,  updateOptionIndex);
                    updateSingleOptionGreeks(calPutItr.second->putSeries, forwardPrice, lastSeriesIndex, updateOptionIndex);
                }
            }
        };

    }
}

#endif //OPTIONTRADING_UPDATEGREEKS_H
