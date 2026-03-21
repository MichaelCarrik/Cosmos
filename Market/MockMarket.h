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

#include <fstream>
#include "../Driver/TestDriver.h"
#include "../Types/SubScribeQuote.h"
#include "Market.h"
#include "../Types/MarketData.h"
#include "../Types/Param.h"
#include "../Utils/TradingHours.h"
#include "../Utils/MemoryList.h"
#include "set"
#include "../KData/KDataManager.h"

namespace TrendFollow{
    namespace Market {
        class MockMarket {

        private:
             Utils::MemoryList< Types::MarketData, 9999999> m_marketDataList{0};
             Driver::TestDriver * m_driver;
            std::string m_rawTickPath{""};

        public:

            MockMarket( Driver::TestDriver* driver,  std::string&);
            std::unordered_map<  Types::Instrument_t ,  Types::PushMarket*,  Types::InstrumentHash> m_subScribeInstruments;

            void SubScribeQuote( Types::SubScribeQuote const & subscribQuote);
            void onRtnQuote(const  Types::MarketData *marketdate) ;
            int start(int tradingday, bool);
            int read_tick(Types::Instrument_t const & , int,std::string& ,std::vector<  Types::MarketData > &);
        };
    }
}




#endif //TRADEBOTS_SIMCTPMARKET_H



#endif //HFT_TREND_MOCKMARKET_H
