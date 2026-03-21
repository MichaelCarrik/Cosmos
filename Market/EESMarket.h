//
// Created by zhangyw on 3/9/21.
//

#ifndef TRENDFOLLOW_EESMARKET_H_BAK
#define TRENDFOLLOW_EESMARKET_H_BAK

#include "TapAPIError.h"
#include "TapQuoteAPI.h"
#include "../Driver/RealtimeDriver.h"
#include "../Utils/MemoryList.h"
#include "future"
#include <set>
#include "../KData/KDataManager.h"

namespace TrendFollow {
    namespace Market {
        class EESMarket : public ITapQuoteAPINotify {
            struct MarketConnect{
                std::string username;
                std::string password;
                std::string host;
                int port;
            };
        public:

            EESMarket( Driver::RealtimeDriver *driver, std::string &config_path):m_driver(driver),m_configPath(config_path){

            }

            ~EESMarket(){

            }
            void SubScribeQuote( Types::SubScribeQuote const & subScribeQuote) ;
            void SetAPI(ITapQuoteAPI *pAPI);
            int start() ;
            int subscribeAllMarketData();
             KData::KDataManager* initKSeries(int tradingday, bool isDay, std::set< Types::Instrument_t> *queryInstruments);
            virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo *info);
            virtual void TAP_CDECL OnAPIReady();
            virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode);
            virtual void TAP_CDECL OnRspChangePassword(TAPIUINT32 sessionID, TAPIINT32 errorCode){};
            virtual void TAP_CDECL OnRspQryExchange(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIExchangeInfo *info){};
            virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteCommodityInfo *info){};
            virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteContractInfo *info){};
            virtual void TAP_CDECL OnRtnContract(const TapAPIQuoteContractInfo *info){};
            virtual void TAP_CDECL OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract *info){};
            virtual void TAP_CDECL OnRtnQuote(const TapAPIQuoteWhole *info);
            virtual void TAP_CDECL OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole *info);


         //    KData::KDataManager m_kDataManager;
            MarketConnect m_marketConnect;
            ITapQuoteAPI* m_pMdApi{nullptr};
            std::promise<int> m_loginPromise;

            std::string m_configPath;
             Driver::RealtimeDriver *m_driver{nullptr};
            TAPIUINT32	m_uiSessionID;
            std::unordered_map<  Types::Instrument_t ,  Types::PushMarket*,  Types::InstrumentHash> m_subScribeInstruments;

        };
    }
}

#endif //TRENDFOLLOW_EESMARKET_H_BAK
