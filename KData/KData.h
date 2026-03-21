//
// Created by zhangyingwei on 2026/3/20.
//

#ifndef OPTIONTRADING_V2_KDATA_H
#define OPTIONTRADING_V2_KDATA_H

#include "../Types/Type.h"
#include "CallPutSeries.h"

namespace TrendFollow {
    namespace KData {
        class KData {
        public:
            Types::Instrument_t m_instrument{""};
            Types::UpdateTime_t m_updateTimeBegin{""};
            Types::UpdateTime_t m_updateTimeEnd{""};
            int m_beginPsTime{0};
            int m_endPsTime{0};
            int m_tradingday{0};
            Types::Product_t m_productID{""};
            double m_open{0.0};
            double m_high{0.0};
            double m_low{999999.9};
            double m_close{0.0};
            int m_volume{0L};
            double m_amount{0.0};
            double m_oi{0.0};
            double m_upperLimit{99999.0};
            double m_lowerLimit{0.0};
            double m_settlement{0.0};
            double m_bidPrice{0.0};
            double m_askPrice{0.0};
            double m_forwardPrice{0.0};
            int m_bidVolume{0};
            int m_askVolume{0};
            double IV{0.0};
            double bidIV{0.0};
            double askIV{0.0};
            double delta{0.0};
            double gamma{0.0};
            double vega{0.0};
            double theta{0.0};
            int expireDay{0};
            bool isInsert{false};

            void update(const Types::MarketData *pMD) {
                m_high = std::max(m_high, pMD->lastPrice);
                m_low = std::min(m_low, pMD->lastPrice);
                m_close = pMD->lastPrice;
                m_volume = pMD->volume;
                m_amount = pMD->amount;
                m_bidVolume = pMD->bidVolume[0];
                m_askVolume = pMD->askVolume[0];
                m_bidPrice = m_bidVolume > 0 ? pMD->bidPrice[0] : 0.0;
                m_askPrice = m_askVolume > 0 ? pMD->askPrice[0] : 0.0;
                m_oi = pMD->oi;
            }

            void
            updateDayKBar(double price, double high, double low, double open, long volume, double amount, double oi,
                          double settlement) {
                m_open = open;
                m_high = high;
                m_low = low;
                m_close = price;
                m_volume = volume;
                m_amount = amount;
                if (settlement > 0.0 && settlement < 9999999.0) {
                    m_settlement = settlement;
                }

                m_oi = oi;
            }


            void initKBar(Types::Instrument_t &instrument, int beginPsTime, int endPsTime, int tradingday,
                          const Types::MarketData *lastPMD) {
                strcpy(m_instrument.data(), instrument.data());
                m_beginPsTime = beginPsTime;
                m_endPsTime = endPsTime;
                Utils::ToUpdateTime(m_beginPsTime, m_updateTimeBegin);
                Utils::ToUpdateTime(m_endPsTime, m_updateTimeEnd);
                Utils::InstrumentToProduct(m_instrument, m_productID);
                m_tradingday = tradingday;
                m_open = lastPMD->lastPrice;
                update(lastPMD);
            }

            // for no night
            void initKDayKBar(Types::Instrument_t &instrument,  int beginPsTime, int endPsTime,int tradingday,
                              const Types::MarketData *marketData) {
                strcpy(m_instrument.data(), instrument.data());
                m_beginPsTime = beginPsTime;
                m_endPsTime = endPsTime;
                Utils::ToUpdateTime(m_beginPsTime, m_updateTimeBegin);
                Utils::ToUpdateTime(m_endPsTime, m_updateTimeEnd);
                Utils::InstrumentToProduct(m_instrument, m_productID);
                m_tradingday = tradingday;
                m_open = marketData->openPrice;
                m_high = marketData->highestPrice;
                m_low = marketData->lowestPrice;
                m_upperLimit = marketData->upperLimitPrice;
                m_lowerLimit = marketData->lowerLimitPrice;
                update(marketData);
            }
        };
    }
}

#endif //OPTIONTRADING_V2_KDATA_H
