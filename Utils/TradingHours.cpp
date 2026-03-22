//
// Created by zhangyw on 7/18/19.
//

#include "TradingHours.h"





namespace Cosmos {
    namespace Utils {
         std::map< Types::Product_t, TradingSession> TradingHours::m_productTradingSession;
         std::map< Types::Instrument_t , TradingSession> TradingHours::m_instrumentTradingSession;

        void TradingHours::loadTradingHourTypes( boost::property_tree::ptree & pt, std::map<std::string, TradingSession> & tradingHourTypesMap){
            for(auto tradingHour : boost::make_iterator_range(pt.get_child("TradingHour.TradingHourTypes").equal_range("TradingHourType"))){

                    auto id = tradingHour.second.get<std::string>("<xmlattr>.id");
                    TradingSession  tradingSession;

                    for(auto& trading : boost::make_iterator_range(tradingHour.second.equal_range("Trading")))
                    {

                        Duration duration;
                        auto beginTime = trading.second.get<std::string>("<xmlattr>.start");
                        auto endTime = trading.second.get<std::string>("<xmlattr>.end");

                        duration.beginTime =  Utils::ToPsSeconds(beginTime);
                        duration.endTime =  Utils::ToPsSeconds(endTime);
                        int  isCritical =  trading.second.get<int>("<xmlattr>.isCritical");
                        if(isCritical ==1){
                            tradingSession.criticalEndingVec.emplace_back(duration.endTime);
                        }
                        if( duration.beginTime < 9*3600){
                            tradingSession.isHaveNight=true;
                        }
                        tradingSession.closeTodaySeconds =   duration.endTime;
                        tradingSession.tradingVec.emplace_back(duration);
//                        spdlog::info("TradingHours::loadConfig trading : productid={}, beginTime={}, endTime={}",
//                                     productId, duration.beginTime, duration.endTime);
                    }

                    for(auto auction : boost::make_iterator_range(tradingHour.second.equal_range("Auction")))
                    {
                        Duration duration;
                        auto beginTime = auction.second.get<std::string>("<xmlattr>.start");

                        auto endTime = auction.second.get<std::string>("<xmlattr>.end");
                        duration.beginTime =  Utils::ToPsSeconds(beginTime);
                        duration.endTime =  Utils::ToPsSeconds(endTime);
                        tradingSession.auctionVec.emplace_back(duration);
                    }

                tradingHourTypesMap[id] = tradingSession;
            }
        }

        bool TradingHours::isProductIn( Types::Product_t& product){
            auto itr = m_productTradingSession.find(product);
            if(itr == m_productTradingSession.end()){
                return false;
            }
            return true;
        }

         Types::ExchangeType TradingHours::getExchangeType(std::string &exchange){
            if(strcmp(exchange.c_str(),"SHFE") ==0){
                return  Types::ExchangeType::SHFE;
            }else if(strcmp(exchange.c_str(),"INE") ==0){
                return  Types::ExchangeType::INE;
            }else if(strcmp(exchange.c_str(),"DCE") ==0){
                return  Types::ExchangeType::DCE;
            }else if(strcmp(exchange.c_str(),"CZCE") ==0){
                return  Types::ExchangeType::ZCE;
            }else if(strcmp(exchange.c_str(),"CFFEX") ==0){
                return  Types::ExchangeType::CFFEX;
            }else if(strcmp(exchange.c_str(),"GFE") ==0){
                return  Types::ExchangeType::GFE;
            }
            assert(false);
        }


        void TradingHours::loadConfig(std::string & config){

            boost::property_tree::ptree pt;
            boost::property_tree::read_xml(config, pt);

            std::map<std::string, TradingSession>  tradingHourTypesMap;
            TradingHours::loadTradingHourTypes(pt, tradingHourTypesMap);


       //     spdlog::info("TradingHours::loadConfig  init usingProduct={}",usingProduct);

            for(auto exchange : boost::make_iterator_range(pt.get_child("TradingHour").get_child("Exchanges").equal_range("Exchange"))){
                auto exchangeIdStr =exchange.second.get<std::string>("<xmlattr>.id");
                auto products_pt = exchange.second.equal_range("Product");
             //   spdlog::info("TradingHours::loadConfig  exchange ");

                for (auto product :boost::make_iterator_range(products_pt)) {
              //      spdlog::info("TradingHours::loadConfig  product \n");

                    auto productId = product.second.get<std::string>("<xmlattr>.id");

                    auto tradingHourId = product.second.get<std::string>("<xmlattr>.trading_hour_type");
                    auto itr_th = tradingHourTypesMap.find(tradingHourId);
                    if(itr_th != tradingHourTypesMap.end()){
                         Types::Product_t ptchars{""};
                        strcpy(ptchars.data(), productId.c_str());
                        TradingHours::m_productTradingSession[ptchars] = itr_th->second;
                        TradingHours::m_productTradingSession[ptchars].exchangeId =getExchangeType(exchangeIdStr);
                    }else{
            //            spdlog::error("TradingHours::loadConfig  can not find {} tradingHour \n" , productId);
                    }
                }
            }
        }

        int TradingHours::getDayMinutesCount( Types::Product_t const &product) {
            int dayMinutesCount = 0;
            auto itr = TradingHours::m_productTradingSession.find(product);
            if (itr == TradingHours::m_productTradingSession.end()) {
                assert(false && "cannot find in m_instrumentTradingSession");
            }
            for (auto &tradingPtr: itr->second.tradingVec) {
                dayMinutesCount += tradingPtr.endTime -  tradingPtr.beginTime;

            }

            return dayMinutesCount/60;
        }

        bool TradingHours::isNoTradeBeforeCritical( Types::Instrument_t const& instrument ,int psTime, int beforeSeonds) {
            fprintf(stderr, "isNoTradeBeforeCritical, %s\n",instrument.data());
            auto itr = TradingHours::m_instrumentTradingSession.find(instrument);
            if(itr == TradingHours::m_instrumentTradingSession.end()){
                fprintf(stderr, "cannot find %s in m_instrumentTradingSession\n", instrument.data());
                assert(false);
            }
            for(auto &endTime : itr->second.criticalEndingVec){

                if(psTime + beforeSeonds >= endTime && psTime <= endTime  ){
                    spdlog::error("isNoTradeBeforeCritical : endTime={}, psTime={}",endTime, psTime);
                    return true;
                }
            }
            return false;

        }


        bool TradingHours::isNoTradeBeforeTime( Types::Instrument_t const& instrument ,int psTime, int beforeSeonds){

            auto itr = TradingHours::m_instrumentTradingSession.find(instrument);
            if(itr == TradingHours::m_instrumentTradingSession.end()){
                assert(false && "cannot find in m_instrumentTradingSession");
            }
            for(auto &tradingPtr : itr->second.tradingVec){

                if(psTime + beforeSeonds >= tradingPtr.endTime  && psTime <= tradingPtr.endTime  ){
                  //  spdlog::error("isNoTradeBeforeTime : endTime={}, psTime={}",tradingPtr.endTime, psTime);
                    return true;
                }
            }

            return false;
        }


        bool TradingHours::isNoTradeAfterTime( Types::Instrument_t const& instrument , int psTime, int afterSeonds){

            auto itr = TradingHours::m_instrumentTradingSession.find(instrument);
            if(itr == TradingHours::m_instrumentTradingSession.end()){
                assert(false && "cannot find in m_instrumentTradingSession");
            }
            for(auto &tradingPtr : itr->second.tradingVec){

                if(psTime - afterSeonds <= tradingPtr.beginTime  && psTime >= tradingPtr.beginTime  ){
               //     spdlog::error("isNoTradeAfterTime : beginTime={}, psTime={}",tradingPtr.beginTime, psTime);
                    return true;
                }
            }

            return false;
        }


        FTTrait TradingHours::getProductTrait( Types::Instrument_t const & instrument, int psTime){

            auto itr = TradingHours::m_instrumentTradingSession.find(instrument);
            if(itr == TradingHours::m_instrumentTradingSession.end()){
                assert(false && "cannot find in m_instrumentTradingSession");
            }
            for(auto &tradingPtr : itr->second.tradingVec){
               // fprintf(stderr, "getProductTrait instruemnt=%s, psTime=%d, beginTime=%d, endTime=%d\n", 
		//		instrument.data(), psTime, tradingPtr.beginTime, tradingPtr.endTime);
                if(tradingPtr.beginTime <= psTime && psTime <= tradingPtr.endTime  ){
                    return FTTrait::FT_TRADING;
                }
            }

            for(auto &auctionPtr: itr->second.auctionVec) {
                if (auctionPtr.beginTime <= psTime && psTime <= auctionPtr.endTime) {
                    return FTTrait::FT_AUCTION;
                }
            }
//
//            if(psTime > itr->second.closeTodaySeconds && psTime -itr->second.closeTodaySeconds < 360*15 ){
//                return FTTrait::FT_SETTLEMENT;
//            }

            return FTTrait::FT_NOT_TRADING;

        }

         void TradingHours::initInstrumentTradingHours( Types::Instrument_t const& instrument){
         //   fprintf(stderr, "initInstrumentTradingHours %s\n", instrument.data());
             Types::Product_t productId{""};
              Utils::InstrumentToProduct(instrument,productId);
            auto itr = TradingHours::m_productTradingSession.find(productId);
            if(itr == TradingHours::m_productTradingSession.end()){
                char buff[256]{""};
                sprintf(buff, "cannot find %s in m_productTradingSession", instrument.data());
                fprintf(stderr, "%s\n", buff);
                assert(false && "initInstrumentTradingHours");
            }

            m_instrumentTradingSession[instrument] = itr->second;
        }


        TradingSession* TradingHours::getTradingSession( Types::Instrument_t const & instrument){
            auto itr = TradingHours::m_instrumentTradingSession.find(instrument);
            if(itr == TradingHours::m_instrumentTradingSession.end()){
                assert(false && "cannot find in m_instrumentTradingSession");
            }
            return  &itr->second;

        }





    }
}
