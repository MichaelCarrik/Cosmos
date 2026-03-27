//
// Created by zhangyw on 6/2/19.
//

#ifndef TRADEBOTS_TRADER_H
#define TRADEBOTS_TRADER_H

#include <utility>
#include <set>
#include "../Driver/RealtimeDriver.h"
#include "../Types/OrderField.h"
#include "../Types/Param.h"
#include "../Types/QuerySymbol.h"


namespace Cosmos{
    namespace Trader{




        template<typename T , typename Driver>
        class Trader{
        private:
            T *_traderSpi;
            Driver* _realtimeDriver;
        public:




            template <typename ...Args>
            Trader(Driver* realtimeDriver, Args &&... args){
                _realtimeDriver = realtimeDriver;

                _realtimeDriver->register_sendOrderFunction(_realtimeDriver->passn([this]( Types::OrderField const & orderField){
                    sendOrder(std::forward<decltype(orderField)>(orderField) );
                }));

                _realtimeDriver->register_cancelOrderFunction(_realtimeDriver->passn([this]( Types::OrderField const & cancelOrderField, int64_t& epoch_time){
                    cancelOrder(std::forward<decltype(cancelOrderField) >(cancelOrderField), epoch_time );
                }));

                _realtimeDriver->template add_receiver< Types::QuerySymbol>(
                        _realtimeDriver->passn([this]( Types::QuerySymbol const & querySymbol) {
                            onQuerySymbol(std::forward<decltype(querySymbol)>(querySymbol));
                        }));

                _traderSpi = new T(_realtimeDriver,std::forward<decltype(args)>(args)...);
            };


            void sendOrder( Types::OrderField const & OrderField){
                _traderSpi->sendOrder(OrderField );
            };

            void cancelOrder( Types::OrderField const & cancelOrderField, int64_t& epoch_time){
                _traderSpi->cancelOrder(cancelOrderField, epoch_time );
            };

            template<typename ...Args>
            int start(Args &&... args) {
                return  _traderSpi->start(std::forward<decltype(args)>(args)...);
            };

            void onQuerySymbol( Types::QuerySymbol const & querySymbol){
                _traderSpi->onQuerySymbol(std::forward<decltype(querySymbol)>(querySymbol));
            };

            int queryTradeInstrument( std::set< Types::Instrument_t> * instruments){
                return _traderSpi->queryTradeInstrument(std::forward<decltype(instruments)>(instruments));
            };

            decltype(_traderSpi->m_initMarketDataVector) * getInitMarketVec(){
                return &_traderSpi->m_initMarketDataVector;
            };

            decltype(_traderSpi->m_tradeInsInfoVec) * getInstrumentInfoVec() {
                return &_traderSpi->m_tradeInsInfoVec;
            }

        };
    }
}



#endif //TRADEBOTS_TRADER_H
