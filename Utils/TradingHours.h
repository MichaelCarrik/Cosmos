//
// Created by zhangyw on 7/18/19.
//

#ifndef TRADEBOTS_TRADINGHOURS_H
#define TRADEBOTS_TRADINGHOURS_H



#include <map>
#include <vector>
#include "../Types/Type.h"
#include "Utils.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace Cosmos {
    namespace Utils{

        static const int DayBegin = 8*3600;

        enum class FTTrait{
            FT_TRADING,
            FT_AUCTION,
            FT_SETTLEMENT,
            FT_NOT_TRADING
        };

        struct Duration{
            int beginTime{0};
            int endTime{0};
        };

        struct TradingSession{
            std::vector<Duration> auctionVec;
            std::vector<Duration> tradingVec;
            std::vector<int> criticalEndingVec;
            int closeTodaySeconds;
             Types::ExchangeType exchangeId;
            bool isHaveNight{false};

        };
          class TradingHours {
          private:
              static std::map< Types::Product_t, TradingSession> m_productTradingSession;
              static std::map< Types::Instrument_t , TradingSession> m_instrumentTradingSession;
              static std::map<Types::Product_t, std::unordered_map<int, int> *> m_productPsTimeToSession;
              // static std::map<Types::Product_t , Types::Instrument_t> m_productToTradingHoursMap;
              // static std::map<  std::tuple<Types::Product_t, Types::KPeriod>, std::vector<KData::KTime>> m_tradingHoursToKTimeMap;

        public:
              static void loadConfig(std::string & );

              static bool isProductIn( Types::Product_t& product);

              static void loadTradingHourTypes( boost::property_tree::ptree & pt, std::map<std::string, TradingSession> & tradingHourTypesMap);

              static TradingSession* getTradingSession( Types::Instrument_t const& instrument);

              static  Types::ExchangeType getExchangeType(std::string &exchange);

              static  bool isNoTradeBeforeCritical( Types::Instrument_t const& instrument ,int psTime, int beforeSeonds);

              static bool isNoTradeBeforeTime( Types::Instrument_t const& instrument ,int psTime ,int);

              static bool isNoTradeAfterTime( Types::Instrument_t const& instrument, int psTime ,int);

              static void initInstrumentTradingHours( Types::Instrument_t const & instrument);

              static  FTTrait getProductTrait( Types::Instrument_t const & instrument, int psTime);

              static int getDayMinutesCount( Types::Product_t const &);

              static void calTradeSection(TradingSession const& tradingSession, std::unordered_map<int, int> * resultMap);
              static int  getPsTimeToNumb(Types::Product_t product, int psTime,  int&);

          };

    }
}


#endif //TRADEBOTS_TRADINGHOURS_H
