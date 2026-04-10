//
// Created by zhangyw on 8/10/19.
//

#ifndef HFT_MM_SYMBOL_H
#define HFT_MM_SYMBOL_H


#include "Type.h"
#include "../Types/MarketData.h"
#include "../Types/OrderField.h"
#include "OrderField.h"
#include "../Types/KPeriod.h"
#include "../KData/KSeries.h"
#include "../Types/Signal.h"

namespace Cosmos
{
    namespace Types
    {
        struct TradePosition
        {
            int filledPosition{0}; // Total Net Filled Position
            double averagePrice{0.0};
            int T_buyHold{0}; // Total Net today Long Position
            int Y_buyHold{0}; // Total Net yestoday Long Position
            int T_sellHold{0}; // Total Net today Short Position
            int Y_sellHold{0}; // Total Net yestoday Short Position

            double profit{0.0};

            void update_Price(int lastFilledVolume, double filledPrice)
            {
                if (this->filledPosition * (this->filledPosition + lastFilledVolume) > 0 && this->filledPosition *
                    lastFilledVolume > 0)
                {
                    this->averagePrice = (this->averagePrice * this->filledPosition + lastFilledVolume * filledPrice) /
                        (this->filledPosition + lastFilledVolume);
                }
                else if (this->filledPosition * (this->filledPosition + lastFilledVolume) > 0 && this->filledPosition *
                    lastFilledVolume < 0)
                {
                    this->profit += lastFilledVolume * (this->averagePrice - filledPrice);
                }
                else if (this->filledPosition * (this->filledPosition + lastFilledVolume) <= 0)
                {
                    this->profit += this->filledPosition * (filledPrice - this->averagePrice);
                    this->averagePrice = filledPrice;
                }
                this->filledPosition += lastFilledVolume;
                if (this->filledPosition == 0)
                {
                    this->averagePrice = 0.0;
                }
            };


            void update_filled_position(OrderSide orderSide, PositionEffectType pet, double lastFilledPrice,
                                        int lastFilledVolume)
            {
                //       std::unique_lock<std::shared_mutex> lockGuard(filled_lck);
                if (orderSide == OrderSide::buy && pet == PositionEffectType::open)
                {
                    //BO
                    this->update_Price(lastFilledVolume, lastFilledPrice);
                    this->T_buyHold += lastFilledVolume;
                }
                else if (orderSide == OrderSide::buy &&
                    (pet == PositionEffectType::T_close || pet == PositionEffectType::Y_close))
                {
                    //BC
                    this->update_Price(lastFilledVolume, lastFilledPrice);
                    if (pet == PositionEffectType::T_close)
                    {
                        this->T_sellHold -= lastFilledVolume;
                    }
                    else if (pet == PositionEffectType::Y_close)
                    {
                        this->Y_sellHold -= lastFilledVolume;
                    }
                }
                else if (orderSide == OrderSide::sell && pet == PositionEffectType::open)
                {
                    //SO
                    this->update_Price(-lastFilledVolume, lastFilledPrice);
                    this->T_sellHold += lastFilledVolume;
                }
                else if (orderSide == OrderSide::sell && (pet == PositionEffectType::T_close || pet ==
                    PositionEffectType::Y_close))
                {
                    //SC
                    this->update_Price(-lastFilledVolume, lastFilledPrice);
                    if (pet == PositionEffectType::T_close)
                    {
                        this->T_buyHold -= lastFilledVolume;
                    }
                    else if (pet == PositionEffectType::Y_close)
                    {
                        this->Y_buyHold -= lastFilledVolume;
                    }
                }
            }
        };

        struct RiskIndicator
        {
            int openBuyVolume{0};
            int openSellVolume{0};
            int sendOrderNumb{0};
            int cancelOrderNumb{0};
            int filledOrderNumb{0};
            int optionSendOrderNumb{0};
            int optionQuoteOrderNumb{0};
            int optionCancelOrderNumb{0};
            int optionFilledOrderNumb{0};




            void updateRiskIndicator(const Types::OrderField* orderField, bool isOption)
            {
                if (isOption == false)
                {
                    if (orderField->orderStatus == OrderStatus::signal)
                    {
                        sendOrderNumb += 1;
                    }
                    else if (orderField->orderStatus == OrderStatus::allTraded)
                    {
                        filledOrderNumb += 1;
                    }
                    else if (orderField->orderStatus == OrderStatus::canceled)
                    {
                        cancelOrderNumb += 1;
                    }

                    if (orderField->orderStatus == OrderStatus::allTraded || orderField->orderStatus ==
                     OrderStatus::partTraded)
                    {
                        if (orderField->pet == Types::PositionEffectType::open && orderField->orderSide == OrderSide::buy)
                        {
                            openBuyVolume += orderField->lastFilledVolume;
                        }
                        else if (orderField->pet == Types::PositionEffectType::open && orderField->orderSide == OrderSide::sell)
                        {
                            openSellVolume += orderField->lastFilledVolume;
                        }
                    }
                }else
                {
                    if (orderField->orderStatus == OrderStatus::signal)
                    {
                        optionSendOrderNumb += 1;
                    }
                    else if (orderField->orderStatus == OrderStatus::allTraded)
                    {
                        optionFilledOrderNumb += 1;
                    }
                    else if (orderField->orderStatus == OrderStatus::canceled)
                    {
                        optionCancelOrderNumb += 1;
                    }
                }

            }
        };

        class Symbol
        {
        public:
            int policyID;
            InstrumentInfo instrumentInfo{""};
            TradePosition tradePosition;
            int targetPosition{0};

            RiskIndicator riskIndicator;

            const MarketData* lastMD;
            const OrderField* order;

            spdlog::logger* m_positionLog{nullptr};
            std::map<KPeriod, const KData::KSeries*> m_kSeriesMap;

            Symbol * underlySymbol{nullptr};

            int getPendingPosition()
            {
                if (order != nullptr && order->isTerminal == false)
                {
                    auto position = order->orderVolume - order->filledVolume;
                    if (order->orderSide == Types::OrderSide::sell)
                    {
                        position = -position;
                    }
                    return position;
                }
                return 0;
            }

            void posWrite(bool isTrade, int tradingDay, Types::UpdateTime_t const& updateTime, int milli,
                          int tradeOrderId)
            {
                m_positionLog->info(
                    "symbol={} tickTime={} {}.{} {}, filledPos={}, avgPrice={:.3f}, T_BHold={}, Y_BHold={}, T_SHold={}, "
                    "Y_SHold={}, profit={:.3f}, tradeOrderId={}, openBVlm={}, openSVlm={}, "
                    "SendNumb={}, CancelNumb={}, filledNumb={}, O_SendNumb={}, O_CancelNumb={}, O_filledNumb={},",
                    this->instrumentInfo.instrumentID.data(), tradingDay, updateTime.data(), milli, isTrade,
                    this->tradePosition.filledPosition, this->tradePosition.averagePrice,this->tradePosition.T_buyHold,
                    this->tradePosition.Y_buyHold, this->tradePosition.T_sellHold, this->tradePosition.Y_sellHold,
                    this->tradePosition.profit, tradeOrderId, this->riskIndicator.openBuyVolume,
                    this->riskIndicator.openSellVolume, this->riskIndicator.sendOrderNumb, this->riskIndicator.cancelOrderNumb,
                    this->riskIndicator.filledOrderNumb, this->riskIndicator.optionSendOrderNumb, this->riskIndicator.optionCancelOrderNumb,
                    this->riskIndicator.optionFilledOrderNumb);
                m_positionLog->flush();
            }
        };
    }
}

#endif //HFT_MM_SYMBOL_H
