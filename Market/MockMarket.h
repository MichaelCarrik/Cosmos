//
// Created by zhangyw on 6/10/20.
//

#ifndef HFT_TREND_MOCKMARKET_H
#define HFT_TREND_MOCKMARKET_H


//
// Created by zhangyw on 7/3/19.
//

#ifndef TRADEBOTS_SIMCTPMARKET_H
#define TRADEBOTS_SIMCTPMARKET_H


#include "../Driver/TestDriver.h"
#include "../Types/SubScribeQuote.h"
#include "../Types/MarketData.h"
#include "../Utils/TradingHours.h"
#include "../Utils/MemoryList.h"



namespace Cosmos{
    namespace Market {
        class MockMarket {

        private:
             Utils::MemoryList< Types::MarketData, 9999999> m_marketDataList{0};
             Driver::TestDriver * m_driver;
             std::string m_rawTickPath{""};
             bool m_isDay{false};
             bool m_isFuture{true};
             Types::Product_t m_productId{""};


        public:

            MockMarket( Driver::TestDriver* driver,  std::string&, std::string & product, bool isFuture=true);
            std::unordered_map<  Types::Instrument_t ,  Types::PushMarket*,  Types::InstrumentHash> m_subScribeInstruments;

            void SubScribeQuote( Types::SubScribeQuote const & subscribQuote);
            void onRtnQuote(const  Types::MarketData *marketdate) ;
            int start(int tradingday, bool);
            int read_tick(Types::Instrument_t const &, int,std::string&, std::vector<Types::MarketData>&);
            int read_ETFOptionTick(Types::Instrument_t const &, int,std::string&, std::vector<Types::MarketData>&);
            int read_ETFTick(Types::Instrument_t const &instrument, int tradingday, std::string &dayOrNight,
                       std::vector<Types::MarketData> &allSymbolMarket);
            int read_IndexTick(Types::Instrument_t const &instrument, int tradingday, std::string &dayOrNight,
              std::vector<Types::MarketData> &allSymbolMarket);
            int64_t parse_time_str_with_us(const std::string& time_str);
        };
    }
}




#endif //TRADEBOTS_SIMCTPMARKET_H



#endif //HFT_TREND_MOCKMARKET_H
