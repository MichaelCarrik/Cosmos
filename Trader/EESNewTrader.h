///@system  Dstar V10 api demo
///@file    ApiClient.h
///@author  Hao Lin 2021-01-20

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <string.h>
#include <stdio.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "DstarTradeApiError.h"
#include "DstarTradeApiDataType.h"
#include "DstarTradeApiStruct.h"
#include "DstarTradeApi.h"
#include "UdpClient.h"
#include <future>
#include "../Types/OrderField.h"
#include "../Types/QuerySymbol.h"
#include "../Driver/RealtimeDriver.h"

namespace TrendFollow {
    namespace Trader {
        struct EESNewTradeConnection {
            std::string username;
            std::string password;
            std::string authCode;
            std::string appId;
            std::string host;
            unsigned short port;
        };


        class EESNewTrader : public IDstarTradeSpi {
        public:
            EESNewTrader( Driver::RealtimeDriver *driver, std::string &config_path);

            virtual ~EESNewTrader();

            void SetAddress(const char *ip, int port);

            void SetUser(const char *account, char *passwd, char *appid, char *licenseno);

            int CreateApi();

            int Init();

            bool IsReady();

            bool IsUdpAuth();

            DstarApiAccountIndexType GetAccountIndex();

            DstarApiAuthCodeType GetUdpAuthCode();

            //API
        protected:
            ///客户端与通知接口通信连接断开
            virtual void OnFrontDisconnected();

            ///错误应答
            virtual void OnRspError(DstarApiErrorCodeType nErrorCode);

            ///登录请求响应,错误码为0说明用户登录成功。
            virtual void OnRspUserLogin(const DstarApiRspLoginField *pLoginRsp);

            ///系统信息提交响应
            virtual void OnRspSubmitInfo(const DstarApiRspSubmitInfoField *pRspSubmitInfo);

            ///席位信息响应
            virtual void OnRspSeat(const DstarApiSeatField *pSeat);

            ///合约响应
            virtual void OnRspContract(const DstarApiContractField *pContract);

            ///手续费参数响应
            virtual void OnRspTrdFeeParam(const DstarApiTrdFeeParamField *pFeeParam);

            ///保证金参数响应
            virtual void OnRspTrdMarParam(const DstarApiTrdMarParamField *pMarParam);

            ///市场状态信息响应
            virtual void OnRspTrdExchangeState(const DstarApiTrdExchangeStateField *pTrdExchangeState);

            ///昨持仓快照响应
            virtual void OnRspPrePosition(const DstarApiPrePositionField *pPrePosition);

            ///持仓快照响应
            virtual void OnRspPosition(const DstarApiPositionField *pPosition);

            ///资金快照响应
            virtual void OnRspFund(const DstarApiFundField *pFund);

            ///委托响应
            virtual void OnRspOrder(const DstarApiOrderField *pOrder);

            ///报价响应
            virtual void OnRspOffer(const DstarApiOfferField *pOffer);

            ///成交响应
            virtual void OnRspMatch(const DstarApiMatchField *pTrade);

            ///出入金响应
            virtual void OnRspCashInOut(const DstarApiCashInOutField *pCashInOut);

            ///API准备就绪,只有用户收到此就绪通知时才能进行后续的操作
            virtual void OnApiReady(const DstarApiSerialIdType nSerialId);

            ///UDP认证请求响应,错误码为0说明认证成功。
            virtual void OnRspUdpAuth(const DstarApiRspUdpAuthField *pRspUdpAuth);

            ///报单应答
            virtual void OnRspOrderInsert(const DstarApiRspOrderInsertField *pOrderInsert);

            ///撤单应答
            virtual void OnRspOrderDelete(const DstarApiRspOrderDeleteField *pOrderDelete);

            ///报价应答
            virtual void OnRspOfferInsert(const DstarApiRspOfferInsertField *pOfferInsert);

            ///最新请求号应答
            virtual void OnRspLastReqId(const DstaApiRspLastReqIdField *pLastReqId);

            ///报单通知
            virtual void OnRtnOrder(const DstarApiOrderField *pOrder);

            ///成交通知
            virtual void OnRtnMatch(const DstarApiMatchField *pTrade);

            ///出入金通知
            virtual void OnRtnCashInOut(const DstarApiCashInOutField *pCashInOut);

            ///报价通知
            virtual void OnRtnOffer(const DstarApiOfferField *pOffer);

            ///询价通知
            virtual void OnRtnEnquiry(const DstarApiEnquiryField *pEnquiry);

            ///市场状态通知
            virtual void OnRtnTrdExchangeState(const DstarApiTrdExchangeStateField *pTrdExchangeState);

            ///浮盈通知
            virtual void OnRtnPosiProfit(const DstarApiPosiProfitField *pPosiProfit);

            ///席位信息通知
            virtual void OnRtnSeat(const DstarApiSeatField *pSeat);

            // 密码修改应答
            virtual void OnRspPwdMod(const DstarApiRspPwdModField *pRspPwdModField);

            // 密码修改通知
            virtual void OnRtnPwdMod(const DstarApiPwdModField *pPwdModField);

        public:


            //udp认证
            void UdpAuth();

            //提交采集信息
            int SubmitSystemInfo();

            // 报单
            void ReqOrderInsert(const DstarApiReqOrderInsertField *pOrder);

            // 报价
            void ReqOfferInsert(const DstarApiReqOfferInsertField *pOffer);

            // 撤单
            void ReqOrderDelete(const DstarApiReqOrderDeleteField *pOrder);

            // 组合报单
            void ReqCmbOrderInsert(const DstarApiReqCmbOrderInsertField *pCmbOrder);

        public:

            int start(int &);

            void sendOrder( Types::OrderField const &orderField){};
            void sendAbtrgOrder(  Types::ArbitrageOrderField const & orderField){};

            void cancelOrder( Types::OrderField const &orderField, int64_t &epoch_time){};
            void cancelAbtrgOrder( Types::ArbitrageOrderField const & inputOrder, int64_t& epoch_time){};

            void onQuerySymbol( Types::QuerySymbol const & querySymbol){};
            int queryTradeInstrument();

            std::unordered_map<Types::Instrument_t, Types::TradePosition, Types::InstrumentHash> m_onQuerySymbolMap;

             Utils::MemoryList< Types::EventData,  Types::OrderBuffSize> m_eventDataList{0};
             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize> m_receiveOrderList{0};
             Utils::MemoryList< Types::ArbitrageOrderField,  Types::OrderBuffSize> m_receiveAbtrgOrderList{0};

             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize>* m_orderList{nullptr};
             Utils::MemoryList< Types::ArbitrageOrderField,  Types::OrderBuffSize>* m_abtrgOrderList{nullptr};

            IDstarTradeApi *m_pApi;
            DstarApiRspLoginField m_LoginInfo;

            char m_FrontIp[21];
            int m_FrontPort;
             Driver::RealtimeDriver *m_driver{nullptr};

            DstarApiReqLoginField m_LoginReq;
            EESNewTradeConnection m_eitradConnection;
            std::promise<int> m_loginPromise;
            std::string m_configPath;
            bool m_bReady;
            bool m_bUdpAuth;
            int m_tradingDay{0};
            TUdpClient m_udpClient;
        };
    }
}

#endif
