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

namespace Cosmos {
    namespace Types {

        struct TradePosition{

            int filledPosition{0};     // Total Net Filled Position
            double averagePrice{0.0};
            int T_buyHold{0};    // Total Net today Long Position
            int Y_buyHold{0};    // Total Net yestoday Long Position
            int T_sellHold{0};   // Total Net today Short Position
            int Y_sellHold{0};   // Total Net yestoday Short Position
            int Y_buyPendingClose{0};
            int T_buyPendingClose{0};
            int Y_sellPendingClose{0};
            int T_sellPendingClose{0};
            int openBuyVolume{0};
            int openSellVolume{0};
            int filledOrderNumb{0};
            int infoOrderNumb{0};
            double profit{0.0};

            void update_Price(int lastFilledVolume, double filledPrice) {
                if (this->filledPosition * (this->filledPosition + lastFilledVolume) > 0 && this->filledPosition * lastFilledVolume >0) {
                    this->averagePrice = (this->averagePrice * this->filledPosition + lastFilledVolume * filledPrice) /
                                         (this->filledPosition + lastFilledVolume);
                } else if (this->filledPosition * (this->filledPosition + lastFilledVolume) > 0 && this->filledPosition * lastFilledVolume <0){
                    this->profit +=  lastFilledVolume* ( this->averagePrice - filledPrice );
                }
                else if (this->filledPosition * (this->filledPosition + lastFilledVolume) <= 0) {
                    this->profit += this->filledPosition * (filledPrice - this->averagePrice);
                    this->averagePrice = filledPrice;
                }
                this->filledPosition += lastFilledVolume;
                if(   this->filledPosition ==0){
                    this->averagePrice=0.0;
                }

            };


            void update_filled_position(OrderSide orderSide, PositionEffectType pet, double lastFilledPrice ,int lastFilledVolume) {
                //       std::unique_lock<std::shared_mutex> lockGuard(filled_lck);
                if (orderSide == OrderSide::buy && pet == PositionEffectType::open) { //BO
                    this->update_Price(lastFilledVolume, lastFilledPrice);

                    this->T_buyHold += lastFilledVolume;
                    this->openBuyVolume += lastFilledVolume;

                } else if (orderSide == OrderSide::buy &&
                           ( pet == PositionEffectType::T_close || pet == PositionEffectType::Y_close)) {//BC
                    this->update_Price(lastFilledVolume, lastFilledPrice);

                    if (pet == PositionEffectType::T_close) {
                        this->T_sellHold -= lastFilledVolume;
                    } else if ( pet == PositionEffectType::Y_close) {
                        this->Y_sellHold -= lastFilledVolume;
                    }
                } else if (orderSide == OrderSide::sell && pet == PositionEffectType::open) {     //SO
                    this->update_Price(-lastFilledVolume, lastFilledPrice);
                    this->T_sellHold += lastFilledVolume;
                    this->openSellVolume += lastFilledVolume;
                } else if (orderSide == OrderSide::sell && (pet == PositionEffectType::T_close || pet == PositionEffectType::Y_close)) {     //SC
                    this->update_Price(-lastFilledVolume, lastFilledPrice);
                    if (pet == PositionEffectType::T_close) {
                        this->T_buyHold -=  lastFilledVolume;
                    } else if (pet == PositionEffectType::Y_close) {
                        this->Y_buyHold -= lastFilledVolume;
                    }
                }
            }
        };

        class Symbol {
        public:
            int policyID;
            InstrumentInfo instrumentInfo{""};
            TradePosition tradePosition;
            int targetPosition{0};
            int preTargetPosition{0};

            const  Types::MarketData * lastMD;
            const  Types::OrderField * order;
            int lastOrderPsTime{0};
	        double symbolLastFilledPrice{0.0};
            int sendCount{0};
            int readKlineIndex{0};
            ExchangeType exchangeId;
            SignalIntension intension{SignalIntension::put};
            spdlog::logger* m_positionLog{nullptr};
            std::map<Types::KPeriod, const  KData::KSeries*> m_kSeriesMap;

            int getPendingPosition(){
                if(order != nullptr && order->isTerminal == false){
                    auto position = order->orderVolume - order->filledVolume;
                    if (order->orderSide == Types::OrderSide::sell){
                        position = -position;
                    }
                    return position;
                }
                return 0;
            }

            void posWrite(bool isTrade, int tradingDay,  Types::UpdateTime_t const& updateTime, int milli, int tradeOrderId) {
                m_positionLog->info(
                        "tradeSymbol={} tickTime={} {}.{} {}, filledPosition={}, averagePrice={:.3f}, T_buyHold={}, Y_buyHold={}, T_sellHold={}, "
                        "Y_sellHold={}, T_BuyClosePend={}, T_SellClosePend={}, profit={:.3f}, tradeOrderId={}, openBuyVolume={}, openSellVolume={}, "
                        "infoOrderNumb={}, filledOrderNumb={}, lastOrderPsTime={}",
                      this->instrumentInfo.instrumentID.data(),  tradingDay, updateTime.data(), milli, isTrade,
                       this->tradePosition.filledPosition, this->tradePosition.averagePrice,
                        this->tradePosition.T_buyHold, this->tradePosition.Y_buyHold,
                        this->tradePosition.T_sellHold, this->tradePosition.Y_sellHold,
                        this->tradePosition.T_buyPendingClose, this->tradePosition.T_sellPendingClose,
                        this->tradePosition.profit, tradeOrderId , this->tradePosition.openBuyVolume, this->tradePosition.openSellVolume,
                        this->tradePosition.infoOrderNumb, this->tradePosition.filledOrderNumb,  this->lastOrderPsTime );
                m_positionLog->flush();
            }



//            void initlog(std::string logkey, std::string logPath) {
//                m_positionLog = spdlog::basic_logger_st(logkey, logPath);
//            }

        };


    }
}

#endif //HFT_MM_SYMBOL_H
