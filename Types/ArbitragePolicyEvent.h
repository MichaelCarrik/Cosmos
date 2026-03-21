//
// Created by zhangyw on 8/9/21.
//

#ifndef PAIRSTRADING_POLICYEVENT_H
#define PAIRSTRADING_POLICYEVENT_H

#include "ArbitrageOrderField.h"
#include "../Types/Symbol.h"
#include "../Types/PairsMarketData.h"
//#include "../Utils//EWMAPriceIndicator.h"

namespace TrendFollow{
    namespace Types {

        struct ArbitragePolicyEvent {
            int subPolicyID{-1};
            std::array<char, 4> orderPrex{""};
            Types::Instrument_t arbitrageInstrument{""};

            Types::Symbol *tradeSymbol{nullptr};
            Types::Symbol *hedgeSymbol{nullptr};
            const Types::ArbitrageOrderField * arbitrageBuyOrder{nullptr};
            const Types::ArbitrageOrderField * arbitrageSellOrder{nullptr};

            const Types::MarketData *hedgeMD{nullptr};
            const Types::MarketData *tradeMD{nullptr};
            const Types::MarketData *arbitrageMD{nullptr};

            KData::KSeries *hedgeKSeries{nullptr};
            KData::KSeries *tradeKSeries{nullptr};

            std::shared_ptr<spdlog::logger> m_orderLog;
            double lastFilledPrice{0.0};
	        int filledDirection{0};
            Types::OrderSide lastFilledOrderSide;
            int abtrgPosition{0};
            double abtrgAveragePrice{0.0};
            double profit{0.0};
            int preCloseHedgeToday{0};
            int preCloseTradeToday{0};
            int maxOpenVolume{5000};
            Types::EventType eventType;
            int64_t epoch_time;
            Types::PairsMarketData pairsMarketData;
        //    Utils::EWMAPriceIndicator ewmaHedgePrice;
        //    Utils::EWMAPriceIndicator ewmaTradePrice;
            Utils::EWMAPriceIndicator ewmaMidPrice;

            void setMidPrice(double midPrice, Types::EventType eventType){
                if(this->hedgeMD == nullptr || this->tradeMD== nullptr){
                    return;
                }
                if(eventType == Types::EventType::hedge && abs(midPrice - this->hedgeMD->midPrice) > 0.00001){
                    ewmaMidPrice.setEWMAValue(midPrice - this->tradeMD->midPrice);
                }else if(eventType == Types::EventType::trade && abs(midPrice - this->tradeMD->midPrice) > 0.00001){
                    ewmaMidPrice.setEWMAValue(this->hedgeMD->midPrice -midPrice);
                }
            }



            void _update_Price(int lastFilledVolume, double filledPrice) {
                if (this->abtrgPosition * (this->abtrgPosition + lastFilledVolume) > 0 && this->abtrgPosition * lastFilledVolume >0) {
                    this->abtrgAveragePrice = (this->abtrgAveragePrice * this->abtrgPosition + lastFilledVolume * filledPrice) /
                                              (this->abtrgPosition + lastFilledVolume);
                } else if (this->abtrgPosition * (this->abtrgPosition + lastFilledVolume) > 0 && this->abtrgPosition * lastFilledVolume <0){
                    this->profit +=  lastFilledVolume* ( this->abtrgAveragePrice - filledPrice );
                }
                else if (this->abtrgPosition * (this->abtrgPosition + lastFilledVolume) <= 0) {
                    this->profit += this->abtrgPosition * (filledPrice - this->abtrgAveragePrice);
                    this->abtrgAveragePrice = filledPrice;
                }
                this->abtrgPosition += lastFilledVolume;
                if(   this->abtrgPosition ==0){
                    this->abtrgAveragePrice=0.0;
                }

            };


            void update_abtrgPosition(OrderSide orderSide, double lastFilledPrice ,int lastFilledVolume) {
                //       std::unique_lock<std::shared_mutex> lockGuard(filled_lck);
                if (orderSide == OrderSide::buy ) { //BO
                    this->_update_Price(lastFilledVolume, lastFilledPrice);
                }  else if (orderSide == OrderSide::sell) {     //SO
                    this->_update_Price( -lastFilledVolume, lastFilledPrice);
                }
                this->lastFilledOrderSide  = orderSide;
                this->lastFilledPrice = lastFilledPrice;

            }
        };

        struct PolicyMarket{
            const Types::EventType eventType;
            Types::ArbitragePolicyEvent * arbitragePolicyEvent{nullptr};
	       PolicyMarket(Types::EventType marketType): eventType(marketType){};
        };
    }
};

#endif //PAIRSTRADING_POLICYEVENT_H
