///@system  Dstar V10 api demo
///@file    EESNewTrader.cpp
///@author  Hao Lin 2021-01-20


#include "EESNewTrader.h"
namespace TrendFollow {
    namespace Trader {

        int EESNewTrader::start(int & tradingday) {
            boost::property_tree::ptree pt;
            boost::property_tree::read_xml(m_configPath, pt);


            m_eitradConnection.username = pt.get_child("ThostUser2.ConnectConfig.Username").get<std::string>(
                    "<xmlattr>.value");
            m_eitradConnection.password = pt.get_child("ThostUser2.ConnectConfig.Password").get<std::string>(
                    "<xmlattr>.value");
            m_eitradConnection.authCode = pt.get_child("ThostUser2.ConnectConfig.Auth").get<std::string>(
                    "<xmlattr>.code");
            m_eitradConnection.appId = pt.get_child("ThostUser2.ConnectConfig.AppId").get<std::string>(
                    "<xmlattr>.code");
            m_eitradConnection.host = pt.get_child("ThostUser2.ConnectConfig.host").get<std::string>("<xmlattr>.value");
            m_eitradConnection.port = pt.get_child("ThostUser2.ConnectConfig.port").get< unsigned short>("<xmlattr>.value");
//            std::string NRConfigPath = pt.get_child("ThostUser2.ConnectConfig.NRTradeConfig").get<std::string>(
//                    "<xmlattr>.value");

//            m_eitradConnection.brokerId = pt.get_child("ThostUser2.ConnectConfig.BrokerID").get<std::string>("<xmlattr>.name");
            m_pApi = CreateDstarTradeApi();
            m_udpClient.Init( m_eitradConnection.host.c_str(),  m_eitradConnection.port);

            m_pApi->RegisterSpi(this);
            DstarApiIpType hostchar{""};
            strcpy(hostchar, m_eitradConnection.host.c_str());
            m_pApi->RegisterFrontAddress(hostchar,  m_eitradConnection.port);

            strncpy(m_LoginReq.AccountNo, m_eitradConnection.username.c_str(), sizeof(DstarApiAccountNoType) - 1);
            strncpy(m_LoginReq.Password, m_eitradConnection.password.c_str(), sizeof(DstarApiPasswdType) - 1);
            strncpy(m_LoginReq.AppId, m_eitradConnection.appId.c_str(), sizeof(DstarApiAppIdType) - 1);
            strncpy(m_LoginReq.LicenseNo, m_eitradConnection.authCode.c_str(), sizeof(DstarApiLicenseNoType) - 1);


            printf("host=%s, port=%d\n", m_eitradConnection.host.c_str(), m_eitradConnection.port);

            fprintf(stderr,"username=%s, password=%s, authcode=%s, appid=%s\n",
                    m_LoginReq.AccountNo, m_LoginReq.Password, m_LoginReq.AppId, m_LoginReq.LicenseNo);
            std::promise<int> tempPromise;
            m_loginPromise.swap(tempPromise);
            m_pApi->SetLoginInfo(&m_LoginReq);
            m_pApi->SetCpuId(0, 1);
            m_pApi->SetSubscribeStartId(-1);
            int ret = SubmitSystemInfo();
            m_pApi->Init(DSTAR_API_INIT_NOQUERY);
            if (ret != 0)
            {
                return ret;
            }

            char sendbuf[1024] = {0};
            DstarApiHead *head = (DstarApiHead*)sendbuf;
            DstarApiReqUdpAuthField *req = (DstarApiReqUdpAuthField *)&sendbuf[sizeof(DstarApiHead)];
            FillHead(head, CMD_API_Req_UdpAuth, sizeof(DstarApiReqUdpAuthField));
            req->AccountIndex = g_ApiClient.GetAccountIndex();
            req->UdpAuthCode = g_ApiClient.GetUdpAuthCode();
            req->ReqIdMode = DSTAR_API_REQIDMODE_NOCHECK;
            m_udpClient.Send(sendbuf, sizeof(DstarApiReqUdpAuthField) + sizeof(DstarApiHead));



            auto loginFuture = m_loginPromise.get_future();
            auto status = loginFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                printf("EESNewTrade login deferred\n");
                spdlog::error("EESNewTrade login deferred");
                return -1;
            } else if (status == std::future_status::timeout) {
                printf("EESNewTrade login timeout\n");
                spdlog::error("EESNewTrade login timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = loginFuture.get();
                if (ret != 0) {
                    printf("EESNewTrade login retcode : %d\n", ret);
                    spdlog::error("EESNewTrade login retcode : {}", ret);
                    return -1;
                }
            }

            tradingday = m_tradingDay;

            //    m_pTradeApi->QryCommodity(&m_uiSessionID);


            return 0;
        };

        EESNewTrader::EESNewTrader( Driver::RealtimeDriver *driver, std::string &config_path) {
            m_configPath = config_path;
            m_driver = driver;
            m_orderList = new Utils::MemoryList<Types::OrderField ,Types::OrderBuffSize> (0);
            m_abtrgOrderList= new Utils::MemoryList<Types::ArbitrageOrderField ,Types::OrderBuffSize> (0);
        //    m_EESOrderActionList = new Utils::MemoryList<TapAPIOrderCancelReq,  Types::OrderBuffSize>(0);
        }

        EESNewTrader::~EESNewTrader() {
            FreeDstarTradeApi(m_pApi);
        }



        void EESNewTrader::OnFrontDisconnected() {
            printf("OnFrontDisconnected\n");
        }

        void EESNewTrader::OnRspError(DstarApiErrorCodeType nErrorCode) {
            printf("OnRspError:%u\n", nErrorCode);
        }

        void EESNewTrader::OnApiReady(const DstarApiSerialIdType nSerialId) {
            printf("OnApiReady, serial:%llu\n", nSerialId);

            m_bReady = true;
        }

        void EESNewTrader::OnRspUserLogin(const DstarApiRspLoginField *pLoginRsp) {
            m_LoginInfo = *pLoginRsp;
            printf("OnRspUserLogin user:%s index:%u error:%u authcode:%u\n",
                   m_LoginInfo.AccountNo, m_LoginInfo.AccountIndex, m_LoginInfo.ErrorCode, m_LoginInfo.UdpAuthCode);
        }

        void EESNewTrader::OnRspSubmitInfo(const DstarApiRspSubmitInfoField *pRspSubmitInfo) {
            printf("OnRspSubmitInfo user:%s error:%u \n",
                   pRspSubmitInfo->AccountNo, pRspSubmitInfo->ErrorCode);
        }

        void EESNewTrader::OnRspTrdExchangeState(const DstarApiTrdExchangeStateField *pTrdExchangeState) {
            printf("OnRspTrdExchangeState ExchangeId: %c, CommodityType: %c, CommodityNo: %s, TradingState: %c ExchangeTime: %s\n",
                   pTrdExchangeState->ExchangeId,
                   pTrdExchangeState->CommodityType,
                   pTrdExchangeState->CommodityNo,
                   pTrdExchangeState->TradingState,
                   pTrdExchangeState->ExchangeTime);
        }

        void EESNewTrader::OnRtnOrder(const DstarApiOrderField *pOrder) {
            printf("OnRtnOrder AccountNo:%s ContractNo1:%s ContractNo2:%s Direct:%c "
                   "ExchInsertTime:%s Fee:%f FrozenMargin:%f Hedge:%c Margin:%f MatchQty:%d "
                   "MinQty:%d Offset:%c OrderId:%llu OrderLocalNo:%s OrderPrice:%f OrderQty:%d "
                   "OrderState:%c OrderType:%c UpSeatNo:%s SystemNo:%s UpdateTime:%s SeatIndex:%d "
                   "CmbId:%llu ErrCode:%u\n",
                   pOrder->AccountNo,
                   pOrder->ContractNo1,
                   pOrder->ContractNo2,
                   pOrder->Direct,
                   pOrder->ExchInsertTime,
                   pOrder->Fee,
                   pOrder->FrozenMargin,
                   pOrder->Hedge,
                   pOrder->Margin,
                   pOrder->MatchQty,
                   pOrder->MinQty,
                   pOrder->Offset,
                   pOrder->OrderId,
                   pOrder->OrderLocalNo,
                   pOrder->OrderPrice,
                   pOrder->OrderQty,
                   pOrder->OrderState,
                   pOrder->OrderType,
                   pOrder->UpSeatNo,
                   pOrder->SystemNo,
                   pOrder->UpdateTime,
                   pOrder->SeatIndex,
                   pOrder->CmbId,
                   pOrder->ErrCode);
        }

        void EESNewTrader::OnRtnOffer(const DstarApiOfferField *pOffer) {
            printf("OnRtnOffer AccountNo:%s BuyMatchQty:%d BuyOffset:%c BuyPrice:%f ContractNo:%s EnquiryNo:%s "
                   "ExchInsertTime:%s FrozenMargin:%f Margin:%f OrderId:%llu OrderLocalNo:%s OrderQty:%d "
                   "OrderState:%c Reference:%llu SellMatchQty:%d SellOffset:%c SellPrice:%f SerialId:%llu "
                   "SystemNo:%s UpSeatNo:%s UpdateTime:%s ErrCode:%u\n",
                   pOffer->AccountNo,
                   pOffer->BuyMatchQty,
                   pOffer->BuyOffset,
                   pOffer->BuyPrice,
                   pOffer->ContractNo,
                   pOffer->EnquiryNo,
                   pOffer->ExchInsertTime,
                   pOffer->FrozenMargin,
                   pOffer->Margin,
                   pOffer->OrderId,
                   pOffer->OrderLocalNo,
                   pOffer->OrderQty,
                   pOffer->OrderState,
                   pOffer->Reference,
                   pOffer->SellMatchQty,
                   pOffer->SellOffset,
                   pOffer->SellPrice,
                   pOffer->SerialId,
                   pOffer->SystemNo,
                   pOffer->UpSeatNo,
                   pOffer->UpdateTime,
                   pOffer->ErrCode);
        }

        void EESNewTrader::OnRtnMatch(const DstarApiMatchField *pMatch) {
            printf("OnRtnMatch MatchId: %llu AccountNo:%s CloseProfit:%f ContractNo:%s Direct:%c ExchMatchNo:%s "
                   "Fee:%f FrozenMargin:%f Hedge:%c Margin:%f MatchId:%llu MatchPrice:%f MatchQty:%d "
                   "MatchTime:%s Offset:%c OrderId:%llu OrderType:%c Premium:%f Reference:%llu SerialId:%llu "
                   "SystemNo:%s UpdateTime:%s\n",
                   pMatch->MatchId,
                   pMatch->AccountNo,
                   pMatch->CloseProfit,
                   pMatch->ContractNo,
                   pMatch->Direct,
                   pMatch->ExchMatchNo,
                   pMatch->Fee,
                   pMatch->FrozenMargin,
                   pMatch->Hedge,
                   pMatch->Margin,
                   pMatch->MatchId,
                   pMatch->MatchPrice,
                   pMatch->MatchQty,
                   pMatch->MatchTime,
                   pMatch->Offset,
                   pMatch->OrderId,
                   pMatch->OrderType,
                   pMatch->Premium,
                   pMatch->Reference,
                   pMatch->SerialId,
                   pMatch->SystemNo,
                   pMatch->UpdateTime);
        }

        void EESNewTrader::OnRtnEnquiry(const DstarApiEnquiryField *pEnquiry) {
            printf("OnRtnEnquiry ContractNo:%s Direct:%c EnquiryNo:%s\n",
                   pEnquiry->ContractNo,
                   pEnquiry->Direct,
                   pEnquiry->EnquiryNo);
        }

        void EESNewTrader::OnRtnTrdExchangeState(const DstarApiTrdExchangeStateField *pTrdExchangeState) {
            printf("OnRspTrdExchangeState ExchangeId: %c, CommodityType: %c, CommodityNo: %s, TradingState: %c ExchangeTime: %s\n",
                   pTrdExchangeState->ExchangeId,
                   pTrdExchangeState->CommodityType,
                   pTrdExchangeState->CommodityNo,
                   pTrdExchangeState->TradingState,
                   pTrdExchangeState->ExchangeTime);
        }

        void EESNewTrader::OnRtnPosiProfit(const DstarApiPosiProfitField *pPosiProfit) {
            printf("OnRtnPosiProfit AccountNo: %s PosiProfit: %f SerialId: %llu\n",
                   pPosiProfit->AccountNo,
                   pPosiProfit->PosiProfit,
                   pPosiProfit->SerialId);
        }

        void EESNewTrader::OnRspSeat(const DstarApiSeatField *pSeatInfo) {
            printf("OnRspSeat seatindex:%d exchange:%c seatno:%s\n", pSeatInfo->SeatIndex, pSeatInfo->Exchange,
                   pSeatInfo->SeatNo);
        }

        void EESNewTrader::OnRspContract(const DstarApiContractField *pContract) {
            printf("OnRspContract ContractIndex:%u ContractNo:%s\n", pContract->ContractIndex, pContract->ContractNo);
        }

        void EESNewTrader::OnRspTrdFeeParam(const DstarApiTrdFeeParamField *pFeeParam) {
            printf("OnRspTrdFeeParam AccountNo:%s ContractNo:%s OpenRatio:%f OpenVolume:%f CloseRatio:%f CloseVolume:%f CloseTRatio:%f CloseTVolume:%f\n",
                   pFeeParam->AccountNo,
                   pFeeParam->ContractNo,
                   pFeeParam->OpenRatio,
                   pFeeParam->OpenVolume,
                   pFeeParam->CloseRatio,
                   pFeeParam->CloseVolume,
                   pFeeParam->CloseTRatio,
                   pFeeParam->CloseTVolume);
        }

        void EESNewTrader::OnRspTrdMarParam(const DstarApiTrdMarParamField *pMarParam) {
            printf("OnRspTrdMarParam AccountNo:%s ContractNo:%s BuySpeculateParam:%f BuyHedgeParam:%f SellSpeculateParam:%f SellHedgeParam:%f\n",
                   pMarParam->AccountNo,
                   pMarParam->ContractNo,
                   pMarParam->BuySpeculateParam,
                   pMarParam->BuyHedgeParam,
                   pMarParam->SellSpeculateParam,
                   pMarParam->SellHedgeParam);
        }

        void EESNewTrader::OnRspPrePosition(const DstarApiPrePositionField *pPrePosition) {
            printf("OnRspPrePosition,AccountNo:%s, ContractNo:%s, %d, %f, %d, %f\n",
                   pPrePosition->AccountNo,
                   pPrePosition->ContractNo,
                   pPrePosition->PreBuyQty,
                   pPrePosition->PreBuyAvgPrice,
                   pPrePosition->PreSellQty,
                   pPrePosition->PreSellAvgPrice);
        }

        void EESNewTrader::OnRspPosition(const DstarApiPositionField *pPosition) {
            printf("OnRspPosition,AccountNo:%s, ContractNo:%s, SerialId:%llu, %d, %d, %f, %d, %d, %f\n",
                   pPosition->AccountNo,
                   pPosition->ContractNo,
                   pPosition->SerialId,
                   pPosition->PreBuyQty,
                   pPosition->TodayBuyQty,
                   pPosition->BuyAvgPrice,
                   pPosition->PreSellQty,
                   pPosition->TodaySellQty,
                   pPosition->SellAvgPrice);
        }

        void EESNewTrader::OnRspFund(const DstarApiFundField *pFund) {
            printf("OnRspFund AccountNo:%s, PreEquity:%f, Equity:%f, Avail:%f, Fee:%f, Margin:%f, "
                   "FrozenMargin:%f, Premium:%f, CloseProfit:%f, PositionProfit:%f, CashIn:%f, CashOut:%f\n",
                   pFund->AccountNo, pFund->PreEquity, pFund->Equity, pFund->Avail,
                   pFund->Fee, pFund->Margin, pFund->FrozenMargin, pFund->Premium,
                   pFund->CloseProfit, pFund->PositionProfit, pFund->CashIn, pFund->CashOut);
        }

        void EESNewTrader::OnRspOrder(const DstarApiOrderField *pOrder) {
            printf("OnRspOrder ContractNo:%s, OrderPrice:%f, OrderQty:%d, MinQty:%d, MatchQty:%d, Direct:%c, Offset:%c, Hedge:%c, OrderType:%c, "
                   "ValidType:%c, OrderState:%c, ErrCode:%d, OrderId:%llu, OrderLocalNo:%s, SystemNo:%s, ExchInsertTime:%s, "
                   "SerialId:%llu, FrozenMargin:%f, AccountNo:%s, Ref:%d, SeatIndex:%d\n",
                   pOrder->ContractNo1, pOrder->OrderPrice, pOrder->OrderQty, pOrder->MinQty,
                   pOrder->MatchQty, pOrder->Direct, pOrder->Offset, pOrder->Hedge,
                   pOrder->OrderType, pOrder->ValidType, pOrder->OrderState, pOrder->ErrCode,
                   pOrder->OrderId, pOrder->OrderLocalNo, pOrder->SystemNo, pOrder->ExchInsertTime,
                   pOrder->SerialId, pOrder->FrozenMargin, pOrder->AccountNo, pOrder->Reference, pOrder->SeatIndex);
        }

        void EESNewTrader::OnRspOffer(const DstarApiOfferField *pOffer) {
            printf("OnRspOffer BuyOffset:%c, SellOffset:%c, ContractNo:%s, OrderQty:%d, ErrCode:%d, "
                   "BuyPrice:%f, SellPrice:%f, OrderId:%llu, "
                   "OrderLocalNo:%s, SystemNo:%s, QueryNo:%s, OrderState:%c, SerialId:%lld, FrozenMargin:%f, "
                   "AccountNo:%s, SeatIndex:%d Reference:%llu\n",
                   pOffer->BuyOffset, pOffer->SellOffset, pOffer->ContractNo,
                   pOffer->OrderQty, pOffer->ErrCode, pOffer->BuyPrice, pOffer->SellPrice,
                   pOffer->OrderId, pOffer->OrderLocalNo, pOffer->SystemNo,
                   pOffer->EnquiryNo, pOffer->OrderState, pOffer->SerialId, pOffer->FrozenMargin,
                   pOffer->AccountNo, pOffer->SeatIndex, pOffer->Reference);
        }

        void EESNewTrader::OnRspMatch(const DstarApiMatchField *pMatch) {
            printf("OnRspMatch contract:%s qty:%d price:%f offset:%c direct:%c hedge:%c time:%s orderid:%llu "
                   "matchid:%llu systemno:%s serial:%llu fee:%f margin:%f frozen:%f premium:%f closeprofit:%f "
                   "acccount:%s reference:%d ordertype:%c\n",
                   pMatch->ContractNo, pMatch->MatchQty, pMatch->MatchPrice, pMatch->Offset,
                   pMatch->Direct, pMatch->Hedge, pMatch->MatchTime,
                   pMatch->OrderId, pMatch->MatchId, pMatch->SystemNo, pMatch->SerialId,
                   pMatch->Fee, pMatch->Margin, pMatch->FrozenMargin, pMatch->Premium,
                   pMatch->CloseProfit, pMatch->AccountNo, pMatch->Reference, pMatch->OrderType);
        }

        void EESNewTrader::OnRspCashInOut(const DstarApiCashInOutField *pCashInOut) {
            printf("OnRspCashInOut SerialId:%llu CashInOutType:%c CashInOutValue:%f AccountNo:%s OperateTime:%s\n",
                   pCashInOut->SerialId, pCashInOut->CashInOutType, pCashInOut->CashInOutValue, pCashInOut->AccountNo,
                   pCashInOut->OperateTime);
        }

        void EESNewTrader::OnRspUdpAuth(const DstarApiRspUdpAuthField *pRspUdpAuth) {
            if (pRspUdpAuth->ErrorCode == 0) {
                m_bUdpAuth = true;
            }
            printf("OnRspUdpAuth AccountIndex:%d UdpAuthCode:%u ErrorCode:%u\n",
                   pRspUdpAuth->AccountIndex, pRspUdpAuth->UdpAuthCode, pRspUdpAuth->ErrorCode);
        }

        void EESNewTrader::OnRspOrderInsert(const DstarApiRspOrderInsertField *pOrderInsert) {
            printf("OnRspOrderInsert AccountNo:%s ClientReqId:%d SeatIndex:%d OrderId:%llu ErrCode:%u MaxClientReqId:%u InsertTime:%s Reference:%llu\n",
                   pOrderInsert->AccountNo,
                   pOrderInsert->ClientReqId,
                   pOrderInsert->SeatIndex,
                   pOrderInsert->OrderId,
                   pOrderInsert->ErrCode,
                   pOrderInsert->MaxClientReqId,
                   pOrderInsert->InsertTime,
                   pOrderInsert->Reference);
        }

        void EESNewTrader::OnRspOfferInsert(const DstarApiRspOfferInsertField *pOfferInsert) {
            printf("OnRspOfferInsert AccountNo:%s ClientReqId:%d SeatIndex:%d OrderId:%llu ErrCode:%u MaxClientReqId:%u InsertTime:%s Reference:%llu\n",
                   pOfferInsert->AccountNo,
                   pOfferInsert->ClientReqId,
                   pOfferInsert->SeatIndex,
                   pOfferInsert->OrderId,
                   pOfferInsert->ErrCode,
                   pOfferInsert->MaxClientReqId,
                   pOfferInsert->InsertTime,
                   pOfferInsert->Reference);
        }

        void EESNewTrader::OnRspLastReqId(const DstaApiRspLastReqIdField *pLastReqId) {
            printf("OnRspLastReqId ClientReqId:%d \n", pLastReqId->LastClientReqId);
        }

        void EESNewTrader::OnRspOrderDelete(const DstarApiRspOrderDeleteField *pOrderDelete) {
            printf("OnRspOrderDelete AccountNo:%s ClientReqId:%d SeatIndex:%d OrderId:%llu ErrCode:%u MaxClientReqId:%u InsertTime:%s Reference:%llu\n",
                   pOrderDelete->AccountNo,
                   pOrderDelete->ClientReqId,
                   pOrderDelete->SeatIndex,
                   pOrderDelete->OrderId,
                   pOrderDelete->ErrCode,
                   pOrderDelete->MaxClientReqId,
                   pOrderDelete->InsertTime,
                   pOrderDelete->Reference);
        }

        void EESNewTrader::OnRtnCashInOut(const DstarApiCashInOutField *pCashInOut) {
            printf("OnRtnCashInOut\n");
        }

        void EESNewTrader::OnRtnSeat(const DstarApiSeatField *pSeat) {
            printf("OnRspSeat seatindex:%d exchange:%c seatno:%s\n", pSeat->SeatIndex, pSeat->Exchange, pSeat->SeatNo);
        }

        void EESNewTrader::OnRspPwdMod(const DstarApiRspPwdModField *pRspPwdModField) {
            printf("OnRspPwdMod AccountNo:%s ErrorCode:%d\n", pRspPwdModField->AccountNo, pRspPwdModField->ErrorCode);
        }

        void EESNewTrader::OnRtnPwdMod(const DstarApiPwdModField *pPwdModField) {
            printf("OnRtnPwdMod AccountNo:%s\n", pPwdModField->AccountNo);
        }

        int EESNewTrader::SubmitSystemInfo() {
            char systeminfo[1024] = {0};
            int nLen = 1024;
            unsigned int nVersion = 0;
            int ret = m_pApi->GetSystemInfo(systeminfo, &nLen, &nVersion);
            printf("GetSystemInfo, ret:%d, len:%d, version:%d\n", ret, nLen, nVersion);
            if (ret != 0) {
                return ret;
            }

            DstarApiSubmitInfoField pSubmitInfo = {0};
            strncpy(pSubmitInfo.AccountNo, m_LoginReq.AccountNo, sizeof(DstarApiAccountNoType) - 1);
            pSubmitInfo.AuthType = DSTAR_API_AUTHTYPE_DIRECT;
            pSubmitInfo.AuthKeyVersion = nVersion;
            memcpy(pSubmitInfo.SystemInfo, systeminfo, nLen);
            strncpy(pSubmitInfo.LicenseNo, m_LoginReq.LicenseNo, sizeof(DstarApiLicenseNoType) - 1);
            strncpy(pSubmitInfo.ClientAppId, m_LoginReq.AppId, sizeof(DstarApiAppIdType) - 1);

            m_pApi->SetSubmitInfo(&pSubmitInfo);

            return ret;
        }

        void EESNewTrader::ReqOrderInsert(const DstarApiReqOrderInsertField *pOrder) {
            m_pApi->ReqOrderInsert(pOrder);
        }

        void EESNewTrader::ReqOfferInsert(const DstarApiReqOfferInsertField *pOffer) {
            m_pApi->ReqOfferInsert(pOffer);
        }

        void EESNewTrader::ReqOrderDelete(const DstarApiReqOrderDeleteField *pOrder) {
            m_pApi->ReqOrderDelete(pOrder);
        }

        void EESNewTrader::ReqCmbOrderInsert(const DstarApiReqCmbOrderInsertField *pCmbOrder) {
            m_pApi->ReqCmbOrderInsert(pCmbOrder);
        }
    }
}