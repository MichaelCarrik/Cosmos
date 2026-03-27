//
// Created by zhangyw on 6/13/20.
//

#ifndef HFT_TREND_MOCKTRADER_H
#define HFT_TREND_MOCKTRADER_H

#include "../Types/OrderField.h"
#include "../Driver/TestDriver.h"
#include "../Types/QuerySymbol.h"
#include <set>



namespace Cosmos {
    namespace Trader {
        class MockTrader {
            int m_tradingDay{0};
             Driver::TestDriver * m_driver;
            const Types::MarketData *m_lastMD{nullptr};
            std::string m_rawTickPath{""};
            int m_isDay{0};
            std::unordered_map<int,   Types::OrderField*> m_pendingOrdersMap;
             Utils::MemoryList< Types::OrderField, 99999> m_orderLists{0};
             Utils::MemoryList< Types::OrderField, 99999> m_receiveOrderLists{0};
             Utils::MemoryList< Types::EventData, 99999> m_eventOrderLists{0};
            std::map< Types::Instrument_t,  Types::Symbol*> m_tradeSymbols;
            std::vector< Types::InstrumentInfo> * m_queryInstruments{nullptr};

        public:
            std::vector< Types::InstrumentInfo*> m_tradeInsInfoVec;
            std::vector<Types::MarketData *> m_initMarketDataVector;
            MockTrader( Driver::TestDriver *driver,std::string& rawTickPath, int tradingDay, int isDay):
            m_driver(driver), m_rawTickPath(rawTickPath),m_tradingDay(tradingDay), m_isDay(isDay){
                m_driver->template add_receiver< Types::MarketData>(
                        m_driver->passn([this]( Types::MarketData const & marketData) {
                            onMarketData(std::forward<decltype(marketData)>(marketData));
                        }));

                m_driver->template add_receiver< Types::UnderlyInfo>(
                        m_driver->passn([this]( Types::UnderlyInfo const &underlyInfo) {
                            onUnderlyInfo(std::forward<decltype(underlyInfo)>(underlyInfo));
                        }));

            }
            void sendOrder( Types::OrderField const &OrderField);

            void cancelOrder( Types::OrderField const &cancelOrderField, int64_t &epoch_time);

            int start(int tradingday);

            void onQuerySymbol( Types::QuerySymbol const & querySymbol);

            void onMarketData(Types::MarketData const & marketData);

            void onUnderlyInfo(Types::UnderlyInfo const& underlyInfo);

            void simMatch();

            void onRtnOrder(const  Types::OrderField * );


        };
    }
}

#endif //HFT_TREND_MOCKTRADER_H
