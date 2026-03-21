//
// Created by zhangyw on 1/15/21.
//

#ifndef HFT_TREND_MARKET_H
#define HFT_TREND_MARKET_H

#include <utility>
#include "../Driver/RealtimeDriver.h"
#include "../Types/SubScribeQuote.h"
#include "../KData/KDataManager.h"


namespace TrendFollow{
    namespace Market{
        template <class  T, class Driver>
        class Market {
        private:

            Driver* _realtimeDriver;
        public:

            T *_marketSpi;

            template<typename ...Args>
            Market(Driver * driver,Args &&... args) {
                _realtimeDriver = driver;
                _realtimeDriver->register_subscribeQuoteFunction(_realtimeDriver->passn([this]( Types::SubScribeQuote const & subscribe){
                    SubScribeQuote(std::forward<decltype(subscribe)>(subscribe) );

                }));

                _marketSpi = new T(driver, std::forward<decltype(args)>(args)...);
            };

            template<typename ...Args>
            void SubScribeQuote(Args &&... args) {
                _marketSpi->SubScribeQuote(std::forward<decltype(args)>(args)...);
            };

            template<typename ...Args>
            int start(Args &&... args) {
                return _marketSpi->start(std::forward<decltype(args)>(args)...);
            };

//            template<typename ...Args>
//            PairsTrading::KData::KDataManager* initKSeries(Args &&... args) {
//                return   _marketSpi->initKSeries(std::forward<decltype(args)>(args)...);
//            };
        };
    }
}

#endif //HFT_TREND_MARKET_H
