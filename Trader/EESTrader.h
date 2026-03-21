//
// Created by zhangyw on 3/9/21.
//

#ifndef TRENDFOLLOW_EESTRADER_H_BAK
#define TRENDFOLLOW_EESTRADER_H_BAK

#include "TapTradeAPI.h"
#include "TapAPIError.h"
#include <future>
#include <set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "../Types/OrderField.h"
#include "../Types/QuerySymbol.h"
#include "../Driver/RealtimeDriver.h"
#include "NetRootTrader.h.bak"


namespace TrendFollow {
    namespace Trader {

        struct QueryPosition{
             Types::TradePosition tradePosition;
            int query_buyVolume{0};
            int query_sellVolume{0};
             Types::Instrument_t  instrumentid{""};
        };

        struct OTR{
            int infoOrderNumb{0};
            int filledOrderNumb{0};
        };

        class EESTrader : public ITapTradeAPINotify {

            struct EETradeConnection {
                std::string username;
                std::string password;
                std::string authCode;
                std::string appId;
                std::string host;
                int port;
            };

        public:


            EESTrader( Driver::RealtimeDriver *driver, std::string &config_path) {
                m_configPath = config_path;
                m_driver = driver;
                m_orderList = new Utils::MemoryList<Types::OrderField ,Types::OrderBuffSize> (0);
                m_abtrgOrderList= new Utils::MemoryList<Types::ArbitrageOrderField ,Types::OrderBuffSize> (0);
	        m_EESOrderActionList = new Utils::MemoryList<TapAPIOrderCancelReq,  Types::OrderBuffSize>(0);
            }

            ~EESTrader(void) {
            };

            void SetAPI(ITapTradeAPI *pAPI) {
                m_pTradeApi = pAPI;
            };

        private:

             Driver::RealtimeDriver *m_driver{nullptr};
            std::string m_configPath;
            ITapTradeAPI *m_pTradeApi{nullptr};
            EETradeConnection m_eitradConnection;
            std::promise<int> m_loginPromise;
            std::promise<int> m_queryPositionPromise;
            std::promise<int> m_queryOrderPromise;
            int m_tradingDay{0};
            TAPIUINT32	m_uiSessionID;

            folly::SpinLock m_sendOrderLock;
            QueryPosition m_queryPosition;
            std::unordered_map<Types::Instrument_t, Types::TradePosition, Types::InstrumentHash> m_onQuerySymbolMap;

            std::unordered_map<Types::Instrument_t, OTR, Types::InstrumentHash> m_OTRMap;

             Utils::MemoryList< Types::EventData,  Types::OrderBuffSize> m_eventDataList{0};
             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize> m_receiveOrderList{0};
             Utils::MemoryList< Types::ArbitrageOrderField,  Types::OrderBuffSize> m_receiveAbtrgOrderList{0};

             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize>* m_orderList{nullptr};
             Utils::MemoryList< Types::ArbitrageOrderField,  Types::OrderBuffSize>* m_abtrgOrderList{nullptr};

             Utils::MemoryList<TapAPINewOrder ,  Types::OrderBuffSize> m_EESOrderList{0};
             Utils::MemoryList<TapAPIOrderCancelReq,  Types::OrderBuffSize>* m_EESOrderActionList{0};


        public:
            //对ITapTradeAPINotify的实现
            virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPITradeLoginRspInfo *loginRspInfo);

            virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode){};

            virtual void TAP_CDECL OnRtnOrder(const TapAPIOrderInfoNotice *info);

            virtual void TAP_CDECL OnAPIReady();

            virtual void TAP_CDECL
            OnRspQryFill(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFillInfo *info){};

            virtual void TAP_CDECL OnRspQryPosition(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                    const TapAPIPositionInfo *info);

            void setOTR(Types::Instrument_t& instrument,  char orderStatus);


            virtual void TAP_CDECL OnConnect() {};

            virtual void TAP_CDECL
            OnRspOrderAction(TAPIUINT32 sessionID, TAPIUINT32 errorCode, const TapAPIOrderActionRsp *info) {};

            virtual void TAP_CDECL OnRspChangePassword(TAPIUINT32 sessionID, TAPIINT32 errorCode) {};

            virtual void TAP_CDECL
            OnRspSetReservedInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, const TAPISTR_50 info) {};

            virtual void TAP_CDECL OnRspQryAccount(TAPIUINT32 sessionID, TAPIUINT32 errorCode, TAPIYNFLAG isLast,
                                                   const TapAPIAccountInfo *info) {};

            virtual void TAP_CDECL
            OnRspQryFund(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFundData *info) {};

            virtual void TAP_CDECL OnRtnFund(const TapAPIFundData *info) {};

            virtual void TAP_CDECL OnRspQryExchange(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                    const TapAPIExchangeInfo *info) {};

            virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                     const TapAPICommodityInfo *info) {
             //  fprintf(stderr, "CommodityType=%c, CommodityNo=%s\n",  info->CommodityType, info->CommodityNo );
            };

            virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                    const TapAPITradeContractInfo *info) {};

            virtual void TAP_CDECL OnRtnContract(const TapAPITradeContractInfo *info) {};

            virtual void TAP_CDECL
            OnRspQryOrder(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo *info) ;

            virtual void TAP_CDECL OnRspQryOrderProcess(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                        const TapAPIOrderInfo *info) {};

            virtual void TAP_CDECL OnRtnFill(const TapAPIFillInfo *info) {
//                fprintf(stderr, "OnRtnFill CommodityNo=%s, ContractNo=%s, MatchPrice=%.3f, MatchQty=%d, MatchSide=%c, MatchSource=%c\n",
//                         info->CommodityNo, info->ContractNo,  info->MatchPrice, info->MatchQty, info->MatchSide, info->MatchSource);
            };

            virtual void TAP_CDECL OnRtnPosition(const TapAPIPositionInfo *info) {};

            virtual void TAP_CDECL
            OnRspQryClose(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPICloseInfo *info){};

            virtual void TAP_CDECL OnRtnClose(const TapAPICloseInfo *info) {};

            virtual void TAP_CDECL OnRtnPositionProfit(const TapAPIPositionProfitNotice *info) {};

            virtual void TAP_CDECL OnRspQryDeepQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                     const TapAPIDeepQuoteQryRsp *info) {};

            virtual void TAP_CDECL
            OnRspQryExchangeStateInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                      const TapAPIExchangeStateInfo *info) {};

            virtual void TAP_CDECL OnRtnExchangeStateInfo(const TapAPIExchangeStateInfoNotice *info) {};

            virtual void TAP_CDECL OnRtnReqQuoteNotice(const TapAPIReqQuoteNotice *info) {}; //V9.0.2.0 20150520
            virtual void TAP_CDECL OnRspUpperChannelInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                         const TapAPIUpperChannelInfo *info) {};

            virtual void TAP_CDECL OnRspAccountRentInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                                        const TapAPIAccountRentInfo *info) {};

            virtual void TAP_CDECL
            OnRspSubmitUserLoginInfo(TAPIUINT32, TAPIINT32, TAPIYNFLAG, const TapAPISubmitUserLoginRspInfo *) {};


            int start(int &);

            void sendOrder( Types::OrderField const &orderField);
            void sendAbtrgOrder(  Types::ArbitrageOrderField const & orderField);

            void cancelOrder( Types::OrderField const &orderField, int64_t &epoch_time);
            void cancelAbtrgOrder( Types::ArbitrageOrderField const & inputOrder, int64_t& epoch_time);

            void onQuerySymbol( Types::QuerySymbol const & querySymbol);
            int _queryPosition( Types::Instrument_t queryInstrument);

            void update_AbtrgOrder(const TapAPIOrderInfoNotice * info, Types::ArbitrageOrderField *abtrgOrder);

            char getOrderPositionEffect(Types::PositionEffectType pet);

            template<typename T>
             Types::EventData* convertToEvent(const  Types::OrderField * orderField,  T* list){
                auto event = list->getNewMemory();
                event->point = orderField;
                event->type =1;
                return event;
            }

            template<typename T>
             Types::EventData* convertToAbtrgEvent(const  Types::ArbitrageOrderField * orderField,  T* list){
                auto event = list->getNewMemory();
                event->point = orderField;
                event->type =2;
                return event;
            }

        };
    }
}

#endif //TRENDFOLLOW_EESTRADER_H_BAK
