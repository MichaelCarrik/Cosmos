//
// Created by zhangyingwei on 2026/5/14.
//

#include "UpdateOptionModelPamt.h"

namespace Cosmos {
    namespace KData {
        void UpdateOptionModelPamt::init(KSeries *series, Types::KPeriod const &period, int tradingDay, bool m_isDay) {
            std::tuple<Types::Instrument_t, Types::KPeriod> key = std::make_tuple(
                series->m_insInfo.instrumentID, period);
            if (series->m_insInfo.productIDClass == Types::ProductClass::option) {
                auto optionSeries = series;
                auto keyUnderly = std::make_pair(optionSeries->m_insInfo.underly, period);

                auto underlyTodayBeginIndexItr = m_underlyTodayBeginIndexMap.find(keyUnderly);
                if (underlyTodayBeginIndexItr == m_underlyTodayBeginIndexMap.end()) {
                    m_underlyTodayBeginIndexMap[keyUnderly] = 0;
                }

                auto itrutom = m_underlyToOptionSeriesMap.find(keyUnderly);
                if (itrutom == m_underlyToOptionSeriesMap.end()) {
                    std::vector<KSeries *> *temp = new std::vector<KSeries *>();
                    m_underlyToOptionSeriesMap.insert(std::make_pair(keyUnderly, temp));
                    itrutom = m_underlyToOptionSeriesMap.find(keyUnderly);
                    for (auto i = 0; i < optionSeries->m_underlySeries->m_seriesIndex; i++) {
                        if (optionSeries->m_underlySeries->m_KDataVecs[i]->m_tradingDay == tradingDay && m_isDay ==
                            false) {
                            break;
                        } else if (optionSeries->m_underlySeries->m_KDataVecs[i]->m_tradingDay ==
                                   tradingDay && m_isDay == true &&
                                   Utils::ToPsSeconds(optionSeries->m_underlySeries->m_KDataVecs[i]->m_updateTimeBegin,
                                                      true) >
                                   Utils::DayBegin) {
                            break;
                        }

                        m_underlyTodayBeginIndexMap[keyUnderly] = i;
                    }

                    if (m_underlyTodayBeginIndexMap[keyUnderly] > 0) {
                        m_underlyTodayBeginIndexMap[keyUnderly] = m_underlyTodayBeginIndexMap[keyUnderly] + 1;
                    }
                }
                itrutom->second->emplace_back(optionSeries);
            }
        }

        void UpdateOptionModelPamt::fillOptionSeries(KSeries *optionSeries, int updateUnderlyIndex,
                                                     int updateOptionIndex) {
            if (optionSeries->m_lastPMD != nullptr) {
                int updateEndPsTime = optionSeries->m_underlySeries->m_KDataVecs[updateUnderlyIndex]->m_endPsTime;
                if (optionSeries->m_KDataVecs[updateOptionIndex]->m_tradingDay == 0) {
                    optionSeries->ffill(updateEndPsTime);
                }
            }
        }


        int UpdateOptionModelPamt::getUnderlyTodayBeginIndex(Types::Instrument_t const &instrumentID,
                                                             Types::KPeriod period) {
            std::tuple<Types::Instrument_t, Types::KPeriod> key = std::make_tuple(
                instrumentID, period);
            auto underlyTodayBeginIndexItr = m_underlyTodayBeginIndexMap.find(key);
            if (underlyTodayBeginIndexItr == m_underlyTodayBeginIndexMap.end()) {
                fprintf(stderr, "instrument=%s, period=%d\n", instrumentID, static_cast<int>(period));
                assert(false);
            }
            return underlyTodayBeginIndexItr->second;
        }

        void UpdateOptionModelPamt::updateGreeks(KSeries *underlySeries, double forwardPrice,int lastSeriesIndex) {

            int underlyTodayBeginIndex = getUnderlyTodayBeginIndex(underlySeries->m_insInfo.instrumentID,
                                                                   underlySeries->m_Period);

            int updateOptionIndex = lastSeriesIndex - underlyTodayBeginIndex;

            for (auto &calPutItr: * (underlySeries->m_callPutSeriesMap)) {
                if (strcmp(calPutItr.second->putSeries->m_insInfo.instrumentID.data(), "sc2505P550") == 0 &&
                    strcmp(underlySeries->m_lastPMD->updateTime.data(), "14:30:50") > 0) {
                    int a = 1;
                }
                fillOptionSeries(calPutItr.second->callSeries, lastSeriesIndex, updateOptionIndex);
                fillOptionSeries(calPutItr.second->putSeries, lastSeriesIndex, updateOptionIndex);


                auto callOptionKData = calPutItr.second->callSeries->m_KDataVecs[updateOptionIndex];
                //   callOptionKData->m_forwardPrice = forwardPrice;
                auto underlyKData = calPutItr.second->callSeries->m_underlySeries->m_KDataVecs[lastSeriesIndex];
                auto putOptionKData = calPutItr.second->putSeries->m_KDataVecs[updateOptionIndex];
                auto callTheoryPrice = getKFairClose(callOptionKData);
                auto putTheoryPrice = getKFairClose(putOptionKData);

                double callBidPrioPrice = callOptionKData->m_bidPrice;
                double callAskPrioPrice = callOptionKData->m_askPrice;
                double putBidPrioPrice = putOptionKData->m_bidPrice;
                double putAskPrioPrice = putOptionKData->m_askPrice;


                if ((callOptionKData->m_tradingDay != 0 && (
                         underlyKData->m_tradingDay != callOptionKData->m_tradingDay ||
                         strcmp(underlyKData->m_updateTimeBegin.data(),
                                callOptionKData->m_updateTimeBegin.data()) != 0)) ||
                    (putOptionKData->m_tradingDay != 0 && (underlyKData->m_tradingDay != putOptionKData->m_tradingDay ||
                                                           strcmp(underlyKData->m_updateTimeBegin.data(),
                                                                  putOptionKData->m_updateTimeBegin.data()) != 0))) {
                   // assert(false);

                    fprintf(stderr, "calGreeks error : underly=%s(%d, %s), callOptionKD=%s(%d, %s), putOptionKD=%s(%d, %s), period=%d\n",
                            underlyKData->m_instrument.data(),
                            underlyKData->m_tradingDay, underlyKData->m_updateTimeBegin.data(),
                            callOptionKData->m_instrument.data(),
                            callOptionKData->m_tradingDay, callOptionKData->m_updateTimeBegin.data(),
                            putOptionKData->m_instrument.data(),
                            putOptionKData->m_tradingDay, putOptionKData->m_updateTimeBegin.data(),
                            Types::KPeroidToIntervalVec[static_cast<int>(underlySeries->m_Period)]);
                    // if (strcmp(this->m_insInfo.productID.data(),"MO")!=0 &&
                    //   strcmp(this->m_insInfo.productID.data(),"HO")!=0 &&
                    //   strcmp(this->m_insInfo.productID.data(),"IO")!=0 ) {
                    //     assert(false);
                    //    }
                }


                if (calPutItr.first > forwardPrice) {
                    auto putTheoryBidPrice = callOptionKData->m_bidPrice - underlyKData->m_askPrice + calPutItr.first;
                    auto putTheoryAskPrice = callOptionKData->m_askPrice - underlyKData->m_bidPrice + calPutItr.first;
                    // put otm
                    if (underlyKData->m_bidVolume == 0 || underlyKData->m_askVolume == 0) {
                        putTheoryBidPrice = callOptionKData->m_bidPrice - forwardPrice + calPutItr.first;
                        putTheoryAskPrice = callOptionKData->m_askPrice - forwardPrice + calPutItr.first; // put otm
                    }

                    if ((putTheoryAskPrice - putTheoryBidPrice < putOptionKData->m_askPrice - putOptionKData->m_bidPrice
                         + Types::g_epsilon
                         && putTheoryAskPrice > 0 && putTheoryBidPrice > 0) || putOptionKData->m_bidVolume == 0 ||
                        putOptionKData->m_askVolume == 0) {
                        putTheoryPrice = (putTheoryBidPrice + putTheoryAskPrice) * 0.5;
                    }

                    putBidPrioPrice = std::max(putBidPrioPrice, putTheoryBidPrice);
                    putAskPrioPrice = putAskPrioPrice < Types::g_epsilon
                                          ? putTheoryAskPrice
                                          : std::min(putAskPrioPrice, putTheoryAskPrice);
                } else {
                    auto callTheoryBidPrice = underlyKData->m_bidPrice + putOptionKData->m_bidPrice - calPutItr.first;
                    auto callTheoryAskPrice = underlyKData->m_askPrice + putOptionKData->m_askPrice - calPutItr.first;
                    if (underlyKData->m_bidVolume == 0 || underlyKData->m_askVolume == 0) {
                        callTheoryBidPrice = underlyKData->m_bidPrice + forwardPrice - calPutItr.first;
                        callTheoryAskPrice = underlyKData->m_askPrice + forwardPrice - calPutItr.first;
                    }

                    if ((callTheoryAskPrice - callTheoryBidPrice < callOptionKData->m_askPrice - callOptionKData->
                         m_bidPrice + Types::g_epsilon
                         && callTheoryAskPrice > 0 && callTheoryBidPrice > 0) || callOptionKData->m_bidVolume == 0 ||
                        callOptionKData->m_askVolume == 0) {
                        callTheoryPrice = (callTheoryBidPrice + callTheoryAskPrice) * 0.5;
                    }


                    callBidPrioPrice = std::max(callBidPrioPrice, callTheoryBidPrice);
                    callAskPrioPrice = callAskPrioPrice < Types::g_epsilon
                                           ? callTheoryAskPrice
                                           : std::min(callAskPrioPrice, callTheoryAskPrice);
                }

                if (strcmp(calPutItr.second->callSeries->m_insInfo.instrumentID.data(), "ZC602C1050") == 0 &&
                    strcmp(underlySeries->m_lastPMD->updateTime.data(), "09:00:49") > 0) {
                    int a = 1;
                }

                calPutItr.second->callSeries->calGreeks(updateOptionIndex, callTheoryPrice, forwardPrice,
                                                        callBidPrioPrice, callAskPrioPrice);
                calPutItr.second->putSeries->calGreeks(updateOptionIndex, putTheoryPrice, forwardPrice, putBidPrioPrice,
                                                       putAskPrioPrice);

                // updateSingleOptionGreeks(calPutItr.second->callSeries, forwardPrice, lastSeriesIndex,  updateOptionIndex);
                // updateSingleOptionGreeks(calPutItr.second->putSeries, forwardPrice, lastSeriesIndex, updateOptionIndex);
            }
        }

        void UpdateOptionModelPamt::updateSabr(KSeries *underlySeries, double forwardPrice, int lastSeriesIndex) {
            int underlyTodayBeginIndex = getUnderlyTodayBeginIndex(underlySeries->m_insInfo.instrumentID,
                                                        underlySeries->m_Period);

            int readOptionIndex = lastSeriesIndex - underlyTodayBeginIndex;

       //     underlySeries->m_sabrModelQuantLib->sarbFit( forwardPrice, underlySeries->m_callPutSeriesMap , readOptionIndex);
        //    underlySeries->m_sabrModelQuantLib->getParameters(underlySeries->m_KDataVecs[lastSeriesIndex]->m_sabrPRMT);
        };
    }
}
