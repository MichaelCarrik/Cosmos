//
// Created by zhangyw on 6/13/20.
//

#include "MockTrader.h"

namespace TrendFollow {
    namespace Trader {

        int MockTrader::start(int tradingday){
            m_tradingDay = tradingday;
            std::unordered_map<int,   Types::OrderField*> temp;
            m_pendingOrdersMap= temp;
            m_lastMD = nullptr;
            return 0;
        };

        void MockTrader::sendOrder( Types::OrderField const &inputOrderField) {
            int assignid = 0;
            auto order = m_orderLists.getNewMemory(assignid);
            memcpy(order, &inputOrderField, sizeof( Types::OrderField));
            order->tOrderID = assignid;
            order->orderStatus = Types::OrderStatus::accept;
            m_pendingOrdersMap[order->tOrderID] = order;
            this->onRtnOrder(order);
        };

        void MockTrader::cancelOrder( Types::OrderField const &cancelOrderField, int64_t &epoch_time) {
            m_pendingOrdersMap.erase(cancelOrderField.tOrderID);
            auto cancelOrder = m_orderLists.getMemoryById(cancelOrderField.tOrderID);
            cancelOrder->orderStatus = Types::OrderStatus::canceled;
            this->onRtnOrder(cancelOrder);
        };


        void MockTrader::onQuerySymbol( Types::QuerySymbol const & querySymbol){

            auto itrSymbol = m_tradeSymbols.find(querySymbol.instrumentID);
            if(itrSymbol == m_tradeSymbols.end()){
                auto symbol = new  Types::Symbol();
                symbol->instrumentInfo.instrumentID = querySymbol.instrumentID;
                m_tradeSymbols[symbol->instrumentInfo.instrumentID] = symbol;
                itrSymbol = m_tradeSymbols.find(querySymbol.instrumentID);
            }

             Types::OnQuerySymbol onQuerySymbol;
            onQuerySymbol.id = querySymbol.id;
            onQuerySymbol.tradingDay = m_tradingDay;
            onQuerySymbol.tradePosition  = itrSymbol->second->tradePosition;
            strcpy(onQuerySymbol.instrumentID.data(), querySymbol.instrumentID.data());
            m_driver->send(onQuerySymbol);
        };

        void MockTrader::onMarketData(Types::MarketData const & marketData){
            m_lastMD = &marketData;
            this->simMatch();
        };

        void MockTrader::onUnderlyInfo( Types::UnderlyInfo const&underlyInfo) {
            std::vector< Types::InstrumentInfo> optionInfos;
            Utils::readInstrumentsFromFiles(m_tradingDay, underlyInfo.instrumentID, m_rawTickPath,  optionInfos, m_isDay);

           for(auto & instrumentInfo : optionInfos){
               if(strcmp(instrumentInfo.underly.data(), underlyInfo.instrumentID.data()) == 0){
                   m_driver->send(instrumentInfo);
               }
           }
        }

        void MockTrader::simMatch(){
            std::vector<int> erase_vec;
            for(auto itr = m_pendingOrdersMap.begin(); itr !=m_pendingOrdersMap.end(); itr++){

                auto order = itr->second;
                if(strcmp(order->instrumentID.data(), m_lastMD->instrumentID.data())!=0){
                    continue;
                }
                bool isFilled{false};
                if(order->orderSide ==  Types::OrderSide::buy and order->orderPrice > m_lastMD->lastPrice - 0.0001){
                    isFilled= true;
                }else if(order->orderSide ==  Types::OrderSide::sell and order->orderPrice < m_lastMD->lastPrice + 0.0001){
                    isFilled= true;
                }

                if(isFilled== true){
                    order->lastFilledPrice = m_lastMD->lastPrice;
                    order->lastFilledVolume = order->orderVolume - order->lastFilledVolume;
                    order->filledVolume += order->lastFilledVolume;
                    order->orderStatus = Types::OrderStatus::allTraded;
                    this->onRtnOrder(order);
                    auto itrSymbol = m_tradeSymbols.find(order->instrumentID);
                    if(itrSymbol == m_tradeSymbols.end()){
                           assert(false);
                    }
                    itrSymbol->second->tradePosition.update_filled_position(order->orderSide, order->pet,
                                                                            order->orderPrice, order->lastFilledVolume);
                    erase_vec.emplace_back(order->tOrderID);
                }
            }

            for(auto i : erase_vec){
                m_pendingOrdersMap.erase(i);
            }
        };

        void MockTrader::onRtnOrder(const  Types::OrderField * inputOrderField){

            auto receiveOrder = m_receiveOrderLists.getNewMemory();
            memcpy(receiveOrder, inputOrderField, sizeof( Types::OrderField));
            auto eventData = m_eventOrderLists.getNewMemory();
            eventData->point = receiveOrder;
            eventData->type =1;
            m_driver->callback_asyncEventData(eventData, receiveOrder->policyID);
        };
    }
}
