//
// Created by zhangyingwei on 2026/3/20.
//

#ifndef OPTIONTRADING_V2_KDATA_H
#define OPTIONTRADING_V2_KDATA_H

#include "../Types/MarketData.h"
#include "../Types/Type.h"
#include "../Utils/Utils.h"


namespace Cosmos {
    namespace KAmountBarEngine {

        class KAmountData {
        public:
            Types::Instrument_t m_instrument{""};
            Types::UpdateTime_t m_updateTimeBegin{""};
            Types::UpdateTime_t m_updateTimeEnd{""};
            int m_beginPsTime{0};
            int m_endPsTime{0};
            int m_tradingDay{0};
            Types::Product_t m_productID{""};
            double m_open{0.0};
            double m_high{0.0};
            double m_low{999999.9};
            double m_close{0.0};
            int m_volume{0};
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


            int expireDay{0};
            bool isInsert{false};

            void update(const Types::MarketData *pMD, int diffVolume, double diffAmount) {
                m_endPsTime = pMD->psSecond;
                Utils::ToUpdateTime(m_endPsTime, m_updateTimeEnd);
                m_high = std::max(m_high, pMD->lastPrice);
                m_low = std::min(m_low, pMD->lastPrice);
                m_close = pMD->lastPrice;
                m_volume += diffVolume;
                m_amount += diffAmount;
                m_bidVolume = pMD->bidVolume[0];
                m_askVolume = pMD->askVolume[0];
                m_bidPrice = m_bidVolume > 0 ? pMD->bidPrice[0] : 0.0;
                m_askPrice = m_askVolume > 0 ? pMD->askPrice[0] : 0.0;
                m_oi = pMD->oi;
            }

            void
            updateDayKBar(int psTime,  double price, double high, double low, double open, int diffVolume, double diffAmount, double oi,
                          double settlement) {
                m_endPsTime = psTime;
                Utils::ToUpdateTime(m_endPsTime, m_updateTimeEnd);
                m_open = open;
                m_high = high;
                m_low = low;
                m_close = price;
                m_volume += diffVolume;
                m_amount += diffAmount;
                if (settlement > 0.0 && settlement < 9999999.0) {
                    m_settlement = settlement;
                }

                m_oi = oi;
            }


            void initKBar(Types::Instrument_t &instrument, int beginPsTime, int tradingday,
                          const Types::MarketData *lastPMD, int diffVolume, double diffAmount) {
                strcpy(m_instrument.data(), instrument.data());
                m_beginPsTime = beginPsTime;

                Utils::ToUpdateTime(m_beginPsTime, m_updateTimeBegin);

                Utils::InstrumentToProduct(m_instrument, m_productID);
                m_tradingDay = tradingday;
                m_open = lastPMD->lastPrice;
                update(lastPMD, diffVolume, diffAmount);
            }

            // for no night
            void initKDayKBar(Types::Instrument_t &instrument,  int beginPsTime, int tradingday,
                              const Types::MarketData *marketData, int diffVolume, double diffAmount) {
                strcpy(m_instrument.data(), instrument.data());
                m_beginPsTime = beginPsTime;

                Utils::ToUpdateTime(m_beginPsTime, m_updateTimeBegin);

                Utils::InstrumentToProduct(m_instrument, m_productID);
                m_tradingDay = tradingday;
                m_open = marketData->openPrice;
                m_high = marketData->highestPrice;
                m_low = marketData->lowestPrice;
                m_upperLimit = marketData->upperLimitPrice;
                m_lowerLimit = marketData->lowerLimitPrice;
                update(marketData, diffVolume, diffAmount);
            }
        };
    }
}

#endif //OPTIONTRADING_V2_KDATA_H
