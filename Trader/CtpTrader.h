//
// Created by zhangyw on 7/21/19.
//

#ifndef TRADEBOTS_CTPTRADER_H
#define TRADEBOTS_CTPTRADER_H

#include <future>
#include "../Utils/MemoryList.h"
#include "../dependencies/sdk/ctp/v6.6.9/ThostFtdcTraderApi.h"
#include "../Types/OrderField.h"
#include "../Types/ArbitrageOrderField.h"
#include "../Driver/RealtimeDriver.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "../Types/Param.h"
#include "../Types/Symbol.h"
#include "../Types/QuerySymbol.h"
#include "../Types/CtpTradeConnection.h"
#include "CtpInterpreter.h"
//#include "NetRootInterpreter.h"
#include <set>


namespace Cosmos {
    namespace Trader {
        struct CtpPosition{
            int buy_position{0};
            int buy_todayPosition{0};
            int buy_openVolume{0};
            int sell_position{0};
            int sell_todayPosition{0};
            int sell_openVolume{0};
        };

        struct PO {
            std::string orderRef{""};
            Types::OrderStatus orderStatus;
            int frontId{0};
            int sessionId{0};
            Types::Instrument_t instrument{""};
        };

        class CtpTrader : public CThostFtdcTraderSpi {

            struct QueryPosition{
                 Types::TradePosition tradePosition;
                int cancelOrderCounts{0};
                int query_buyVolume{0};
                int query_sellVolume{0};
                 Types::Instrument_t  instrumentid{""};
            };



        public:
             Driver::RealtimeDriver * m_driver;
             Types::CtpTradeConnection m_ctpConnection;
            CtpTrader( Driver::RealtimeDriver *driver, std::string& configPath):m_configPath(configPath){
                m_driver = driver;
                m_driver->template add_receiver< Types::UnderlyInfo>(
                        m_driver->passn([this]( Types::UnderlyInfo const &underlyInfo) {
                            onUnderlyInfo(std::forward<decltype(underlyInfo)>(underlyInfo));
                        }));
            }


            ~CtpTrader()
            {
                if (m_pTradeApi != NULL) {
                    m_pTradeApi->Release();
                }
            }


            std::promise<int> m_loginPromise;
            std::promise<int>  m_queryOrderPromise;
            std::promise<int>  m_queryPositionPromise;
            std::promise<int>  m_queryTraderPromise;
            std::promise<int>  m_queryInstrumentPromise;
            std::promise<int>  m_queryMarketDataPromise;

             Types::Instrument_t m_queryMarketInstrument{""};
            std::vector<Types::MarketData *> m_initMarketDataVector;

            int m_tradingDay;
            int m_frontID{0};
            int m_sessionID{0};
            int m_maxOrderRef{0};
            std::vector< Types::QryRspTrade> m_QryRspTradeVec;
            std::unordered_map<Types::Instrument_t, Types::TradePosition, Types::InstrumentHash> m_onQuerySymbolMap;
            std::unordered_map<Types::Instrument_t, CtpPosition, Types::InstrumentHash> m_ctpPositionMap;

            int m_requestID{0};

            std::string m_configPath;
            double m_preSettlementPrice{0.0};
            std::vector< Types::InstrumentInfo> m_tradeInsinfoVec;
            std::map<std::string, PO> m_queryOrderStatusMap;

            CtpInterpreter * m_interpreter;
            bool m_isLogin{false};
         //   NetRootInterpreter * m_interpreter;

            int start(int &);
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


            int queryTradeInstrument( );

             Utils::MemoryList< Types::EventData,  Types::OrderBuffSize> m_eventDataList{0};

             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize> m_receiveOrderList{0};
             Utils::MemoryList< Types::ArbitrageOrderField,  Types::OrderBuffSize> m_receiveAbtrgOrderList{0};



///?��???��???????��?��?????????????????��???????????��??????????��??��????
            virtual void OnFrontConnected();

            ///?��???��???????��?��???????????????��????????��??��?????��???��?????��???��??API?����????????????????��??????��????��??
            ///@param nReason ?��?��???��
            ///        0x1001 ???????���?
            ///        0x1002 ???????���?
            ///        0x2001 ???????????��
            ///        0x2002 ?????????���?
            ///        0x2003 ?????��?����???
            virtual void OnFrontDisconnected(int nReason);

            ///???????��???????��???��????????��????��????????��??��????
            ///@param nTimeLapse ?��??????????��??????��??
            virtual void OnHeartBeatWarning(int nTimeLapse);

            ///???��???????��??
            virtual void  OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast);


            ///???????��?��??
            virtual void
            OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast);

            ///???????��?��??
            virtual void
            OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast);

            ///???��?????��?????��?��??
            virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate,
                                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///��??????��?????��?????��?��??
            virtual void OnRspTradingAccountPasswordUpdate(
                    CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate,
                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

//    ///?��?????��?��?��?��??????????????????
//    virtual void OnRspUserAuthMethod(CThostFtdcRspUserAuthMethodField *pRspUserAuthMethod, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
//
//    ///?????????��???????��??????
//    virtual void OnRspGenUserCaptcha(CThostFtdcRspGenUserCaptchaField *pRspGenUserCaptcha, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
//
//    ///?????????��???????��??????
//    virtual void OnRspGenUserText(CThostFtdcRspGenUserTextField *pRspGenUserText, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///��??????????��?��??
            virtual void
            OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast);


            ///?????????????��?��??
            virtual void
            OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast);

            ///???????????????��?��??
            virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///��?????����???��?��??
            virtual void
            OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID, bool bIsLast);

            ///?��??��??����????????��??
            virtual void OnRspQueryMaxOrderVolume(CThostFtdcQryMaxOrderVolumeField *pQueryMaxOrderVolume,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///??��????��???��???????��??
            virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???????????��??
            virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///?????????????��??
            virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction,
                                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
            void assignPosition();




            ///???????????????��?��??
            virtual void
            OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast);

            ///??????????����???��?��??
            virtual void OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???????????��?��??
            virtual void
            OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast);

            ///��??????????��?��??
            virtual void
            OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast);

            ///��?????����???��?��??
            virtual void
            OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID, bool bIsLast);

            ///?��??��?????����???��?��??
            virtual void OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);


            ///?��??����?????????��?��??
            virtual void
            OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast);

            ///???��?��??��????��??
            virtual void
            OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???????��??
            virtual void
            OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��????��????????��??
            virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��??��??????��?��??
            virtual void
            OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast);

            ///???��?��????��????��??
            virtual void
            OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast);

            ///???��?��?????������???��??
            virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo,
                                             int nRequestID, bool bIsLast);

            ///???��?��??????��????????��??
            virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
                                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???????????????��??
            virtual void
            OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????��?��?��??
            virtual void
            OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast);

            ///???��?��???��???��??
            virtual void
            OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast);

            ///???��?��???????��??
            virtual void
            OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast);

            ///???��?��?????��?��??
            virtual void
            OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast);

            ///???��?��????��????��???��???��??
            virtual void
            OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast);

            ///???��?��??��????????��??
            virtual void
            OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast);

            ///???��?��????��????????��???��??
            virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
                                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????��?????��??
            virtual void
            OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast);

            ///???��?��???��???????????��??
            virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��????��????????��???��??
            virtual void OnRspQryInvestorPositionCombineDetail(
                    CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail,
                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///?��??��??????��??????????????��??????��?????��??
            virtual void OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey,
                                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???????????????��??
            virtual void
            OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast);

            ///???��?��????��???????/??????��??????��??
            virtual void
            OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????��?����????????��??
            virtual void OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????��?��?��??��????????��??
            virtual void
            OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???????��??
            virtual void
            OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast);

            ///???��?��???????��?��??����?��?????????��??
            virtual void
            OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast);

            ///???��?��???��??��???????
            virtual void
            OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast);

            ///???��?��???��??����
            virtual void
            OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast);

            ///???��?��??��??????????????????��??
            virtual void
            OnRspQryMMInstrumentCommissionRate(CThostFtdcMMInstrumentCommissionRateField *pMMInstrumentCommissionRate,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��??��????????????????????��??
            virtual void OnRspQryMMOptionInstrCommRate(CThostFtdcMMOptionInstrCommRateField *pMMOptionInstrCommRate,
                                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��??��??????????��??
            virtual void
            OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��??��??????��?��??
            virtual void OnRspQrySecAgentTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???????��?��??��??????��?????��??
//    virtual void OnRspQrySecAgentCheckMode(CThostFtdcSecAgentCheckModeField *pSecAgentCheckMode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

//    ///???��?��???????��?��???????��??
//    virtual void OnRspQrySecAgentTradeInfo(CThostFtdcSecAgentTradeInfoField *pSecAgentTradeInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????????��??��??��??
            //   virtual void OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????????????????��??
//    virtual void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???????????��??
            virtual void
            OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                              bool bIsLast);

            ///???��?��???????��??
            virtual void
            OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast);

            ///???��?��??��????��??
            virtual void
            OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��??????��??????��??
//    virtual void OnRspQryOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��????��??????��??
//    virtual void OnRspQryInvestUnit(CThostFtdcInvestUnitField *pInvestUnit, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��??����??????��????????��??
            virtual void OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard,
                                                     CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???��??����???��??
            virtual void
            OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast);

            ///???��?��??��????��???��??
            virtual void
            OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast);

            ///???��?��???????????????��??
            virtual void
            OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast);

            ///?��?��????
            virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///��???????
            virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

            ///????????
            virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

            ///��????????��?��??��?
            virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

            ///��?????����?��?��??��?
            virtual void
            OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

            ///???????����???????
            virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus);

            ///???��?��????????
            virtual void OnRtnBulletin(CThostFtdcBulletinField *pBulletin);

            ///???��????
            virtual void OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo);

            ///?��???????????��?��?��
            virtual void OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder);

            ///????????????
            virtual void OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder);

            ///?????????????��?��??��?
            virtual void
            OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo);

            ///??????????����?��?��??��?
            virtual void
            OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction, CThostFtdcRspInfoField *pRspInfo);

            ///?????????��?��??��?
            virtual void
            OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo);

            ///��???????
            virtual void OnRtnQuote(CThostFtdcQuoteField *pQuote);

            ///��????????��?��??��?
            virtual void OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo);

            ///��?????����?��?��??��?
            virtual void
            OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo);

            ///????????
            virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp);

            ///��??????��?????????��????
            virtual void
            OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken);

            ///?��??��?????����?��?��??��?
            virtual void OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction,
                                                  CThostFtdcRspInfoField *pRspInfo);

            ///????��?????????
//    virtual void OnRtnOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose);

            ///????��??????????��?��??��?
//    virtual void OnErrRtnOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField *pInputOptionSelfClose, CThostFtdcRspInfoField *pRspInfo);

            ///????��???????����?��?��??��?
//    virtual void OnErrRtnOptionSelfCloseAction(CThostFtdcOptionSelfCloseActionField *pOptionSelfCloseAction, CThostFtdcRspInfoField *pRspInfo);

            ///?��??����??????
            virtual void OnRtnCombAction(CThostFtdcCombActionField *pCombAction);

            ///?��??����???????��?��??��?
            virtual void OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction,
                                                  CThostFtdcRspInfoField *pRspInfo);

            ///???��?��???????????��??
            virtual void
            OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast);

            ///???��?��?????????��??
            virtual void OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                                             int nRequestID, bool bIsLast);

            ///???��?��???????????��??
            virtual void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????��?????��??
            virtual void
            OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast);

            ///???��?��?????????????��?????��??
            virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams,
                                                     CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��?????????????��?????��??
            virtual void OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///???��?��???��?????????��????
            virtual void OnRspQueryCFMMCTradingAccountToken(
                    CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken,
                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

            ///????????????��???��?????????
            virtual void OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer);

            ///????????????��???��?????????
            virtual void OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer);

            ///????????????????��?????????
            virtual void OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal);

            ///????????????????��?????????
            virtual void OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal);

            ///????????????��???��?????????
            virtual void OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer);

            ///????????????��???��?????????
            virtual void OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer);

            ///?????????��??????????????????????��????????��?????????��?����??����?????????????
            virtual void OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal);

            ///?????????��??????????????????????��????????��?????????��?����??����?????????????
            virtual void OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal);

            ///?????????��???????��??????
            virtual void OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount);

            ///????????????��???��??????��?��??��?
            virtual void
            OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo);

            ///????????????��???��??????��?��??��?
            virtual void
            OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo);

            ///?????????��??????????????????????��??????��?��??��?
            virtual void OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal,
                                                                  CThostFtdcRspInfoField *pRspInfo);

            ///?????????��??????????????????????��??????��?��??��?
            virtual void OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal,
                                                                  CThostFtdcRspInfoField *pRspInfo);

            ///?????????��???????��???��?��??��?
            virtual void OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount,
                                                          CThostFtdcRspInfoField *pRspInfo);

            ///????????????????��????????��?????????��?����??����?????????????
            virtual void OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal);

            ///????????????????��????????��?????????��?����??����?????????????
            virtual void OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal);

            ///????????????��???��?????????
            virtual void
            OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID, bool bIsLast);

            ///????????????��???��?????????
            virtual void
            OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID, bool bIsLast);

            ///?????????��???????��??????
            virtual void OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount,
                                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                            bool bIsLast);

            ///???????????????��????
            virtual void OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount);

            ///?????????????��?��????
            virtual void OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount);

            ///????????��??��????????????
            virtual void OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount);

            CThostFtdcTraderApi *m_pTradeApi;

        public:


            char getOrderSide( Types::OrderSide orderSide);
            char getOrderPositionEffect( Types::PositionEffectType pet);
            char getOrderHedgeFlag( Types::HedgeType ht);
            void updateOrder_with(const CThostFtdcOrderField * info,  Types::OrderField *pOrder);

            void sendOrder(  Types::OrderField const & orderField);
            void sendAbtrgOrder(  Types::ArbitrageOrderField const & orderField);

            int _queryAllPosition();

            int _querySinglePosition( Types::Instrument_t const & queyInstrument );
            int _querySingleTrade( Types::Instrument_t const & queyInstrument );
            int _queryOrder();
            int _queryMarketData();

            bool cancelOrder( Types::OrderField const & inputOrder, int64_t& epoch_time) ;

            bool cancelAbtrgOrder( Types::ArbitrageOrderField const & inputOrder, int64_t& epoch_time) ;
            void onUnderlyInfo(Types::UnderlyInfo const& underlyInfo);
            void onQuerySymbol( Types::QuerySymbol const & querySymbol);


            const std::vector<Types::InstrumentInfo>* getInstrumentInfoVec() {
                return &m_tradeInsinfoVec;
            };

        };

    };
}


#endif //TRADEBOTS_CTPTRADER_H
