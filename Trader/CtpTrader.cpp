//
// Created by zhangyw on 7/21/19.
//

#include <filesystem>
#include "CtpTrader.h"


namespace Cosmos {
    namespace Trader {

        void CtpTrader::OnFrontConnected() {
            CThostFtdcReqAuthenticateField authField;
            memset(&authField, 0, sizeof(CThostFtdcReqAuthenticateField));
            strcpy(authField.UserID, m_ctpConnection.username.c_str());
            strcpy(authField.AppID, m_ctpConnection.appId.c_str());
            strcpy(authField.AuthCode, m_ctpConnection.authCode.c_str());
            strcpy(authField.BrokerID, m_ctpConnection.brokerId.c_str());
            int reqCount = 0;
            int retCode{1};

            while (reqCount < 3 && retCode != 0) {
                retCode = m_pTradeApi->ReqAuthenticate(&authField, m_requestID++);
                reqCount++;
                sleep(1);
            }
            if (retCode != 0) {
                spdlog::error("ReqAuthenticate error : {}", retCode);
            }
        };

        void CtpTrader::OnFrontDisconnected(int nReason) {
            fprintf(stderr, "CtpTrader OnFrontDisconnected Reason : %d", nReason);
            spdlog::error("CtpTrader OnFrontDisconnected Reason : {}", nReason);
            //    m_loginPromise.set_value(2);

        }

        void CtpTrader::OnHeartBeatWarning(int nTimeLapse) {

        }

        void CtpTrader::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo->ErrorID == 0) {
                //  printf("\tAppID [%s]\n", pRspAuthenticateField->AppID);
                //   printf("\tAppType [%c]\n", pRspAuthenticateField->AppType);
                CThostFtdcReqUserLoginField reqUserLogin;
                memset(&reqUserLogin, 0, sizeof(reqUserLogin));

                strcpy(reqUserLogin.BrokerID, m_ctpConnection.brokerId.c_str());
                strcpy(reqUserLogin.UserID, m_ctpConnection.username.c_str());
                strcpy(reqUserLogin.Password, m_ctpConnection.password.c_str());
                int retCode = m_pTradeApi->ReqUserLogin(&reqUserLogin, 1);
                if (retCode) {
                    fprintf(stderr, "ReqAuthenticate error %d\n", retCode);
                    spdlog::error("ReqAuthenticate error : {}", retCode);
                }
            } else {
                m_loginPromise.set_value(3);
                spdlog::error("OnRspAuthenticate error : {} : {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            }
        };

        void CtpTrader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) {

            if (pRspInfo->ErrorID == 0 and m_isLogin == false) {
                spdlog::info("trade login ok, wait api initail");
                //   m_loginPromise.set_value(0);
                try {
                    m_tradingDay = atoi(pRspUserLogin->TradingDay);

                    m_maxOrderRef = atoi(pRspUserLogin->MaxOrderRef) + 1;

                    m_frontID = pRspUserLogin->FrontID;
                    m_sessionID = pRspUserLogin->SessionID;
                    fprintf(stderr, "ctpTrade login ok m_maxOrderRef=%d, frontID=%d, sessionID=%d\n", m_maxOrderRef,
                            m_frontID, m_sessionID);
                    m_loginPromise.set_value(0);
                }
                catch (std::future_error const &e) {
                    spdlog::error("OnRspUserLogin Error");
                }

            } else if (m_isLogin == false) {
                m_loginPromise.set_value(4);
                fprintf(stderr, "trade login error : %d, %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                spdlog::error("trade login error : {} , {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);

            }

            //       spdlog::error( "API is not ready");


        };

        void CtpTrader::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) {

            if (pUserLogout) {
                printf("\tBrokerID [%s]\n", pUserLogout->BrokerID);
                printf("\tUserID [%s]\n", pUserLogout->UserID);
            }
            if (pRspInfo) {
                printf("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
                printf("\tErrorID [%d]\n", pRspInfo->ErrorID);
            }

        };

        void CtpTrader::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {


        };

        void CtpTrader::OnRspTradingAccountPasswordUpdate(
                CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {


        };


        void CtpTrader::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast) {
            if (pRspInfo->ErrorID != 0) {
                spdlog::error("OnRspOrderInsert : InstrumentID={}, RequestID={}, OrderRef={}, ErrorID={}, ErrorMsg={}",
                              pInputOrder->InstrumentID, pInputOrder->RequestID, pInputOrder->OrderRef,
                              pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            }
        };

        void
        CtpTrader::OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {


            if (pRspInfo->ErrorID != 0) {
                spdlog::info("OnRspOrderAction : OrderRef={}, error {} {}", pInputOrderAction->OrderRef,
                             pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            }

        };

        void CtpTrader::OnRspQueryMaxOrderVolume(CThostFtdcQryMaxOrderVolumeField *pQueryMaxOrderVolume,
                                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction,
                                                     CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };


        void CtpTrader::OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                      bool bIsLast) {
            if (pOrder != nullptr && (pRspInfo == nullptr || pRspInfo->ErrorID == 0)) {
                if (pOrder->OrderRef[0] == m_ctpConnection.OrderPrefix[0] &&
                    pOrder->OrderRef[1] == m_ctpConnection.OrderPrefix[1] &&
                    pOrder->OrderRef[2] == m_ctpConnection.OrderPrefix[2]) {
                    std::string orderSysId{pOrder->OrderSysID};
                    PO po;
                    po.orderStatus = Types::OrderStatus::signal;
                    po.orderRef = pOrder->OrderRef;
                    po.frontId = pOrder->FrontID;
                    po.sessionId = pOrder->SessionID;
                    strcpy(po.instrument.data(), pOrder->InstrumentID);
                    switch (pOrder->OrderStatus)
                    {
                        case THOST_FTDC_OSS_Accepted:
                            po.orderStatus =  Types::OrderStatus::accept;
                            break;
                        case THOST_FTDC_OST_AllTraded:

                            po.orderStatus=  Types::OrderStatus::allTraded;
                            break;

                        case THOST_FTDC_OST_PartTradedNotQueueing:
                        case THOST_FTDC_OST_PartTradedQueueing  :

                            po.orderStatus =  Types::OrderStatus::partTraded;
                            break;
                        case  THOST_FTDC_OST_Canceled   :
                            po.orderStatus =  Types::OrderStatus::canceled;
                            break;
                        default:

                            po.orderStatus =  Types::OrderStatus::unknown;
                            break;
                    }

                    m_queryOrderStatusMap[orderSysId] = po;
                    }
            }else if (pRspInfo != nullptr && pRspInfo->ErrorID != 0) {
                m_queryOrderPromise.set_value(pRspInfo->ErrorID);
            }

            if (bIsLast == true) {
                m_queryOrderPromise.set_value(0);

            }
        };

        void CtpTrader::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                      bool bIsLast) {
//            if (pTrade != nullptr && (pRspInfo == nullptr || pRspInfo->ErrorID == 0)) {
//                if (strcmp(m_queryPosition.instrumentid.data(), pTrade->InstrumentID) == 0) {
//                     Types::OrderSide orderSide;
//                    if (pTrade->Direction == THOST_FTDC_D_Buy) {
//                        orderSide =  Types::OrderSide::buy;
//                    } else {
//                        orderSide =  Types::OrderSide::sell;
//                    }
//
//                     Types::PositionEffectType pet;
//                    if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) {
//                        pet =  Types::PositionEffectType::open;
//                    } else if (pTrade->OffsetFlag == THOST_FTDC_OFEN_CloseToday) {
//                        pet =  Types::PositionEffectType::T_close;
//                    } else {
//                        pet =  Types::PositionEffectType::Y_close;
//                    }
//                     Types::QryRspTrade qryRspTrade;
//                    strcpy(qryRspTrade.instrumentID.data(), pTrade->InstrumentID);
//                    qryRspTrade.lastFilledVolume = pTrade->Volume;
//                    qryRspTrade.lastFilledPrice = pTrade->Price;
//                    qryRspTrade.orderSide = orderSide;
//                    qryRspTrade.tradeNo = pTrade->TradeID;
//                    qryRspTrade.pet = pet;
//                    m_QryRspTradeVec.emplace_back(qryRspTrade);
//                }
//
//            } else if (pRspInfo != nullptr && pRspInfo->ErrorID != 0) {
//                m_queryTraderPromise.set_value(pRspInfo->ErrorID);
//            }
//
//            if (bIsLast == true) {
//
//                m_queryTraderPromise.set_value(0);
//            }


        }

        void CtpTrader::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pInvestorPosition != nullptr && (pRspInfo == nullptr || pRspInfo->ErrorID == 0)) {

//                fprintf(stderr,"instrument=%s, direction=%c, Position=%d, TodayPosition=%d, YdPosition=%d, OpenVolume=%d\n",
//                        pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection, pInvestorPosition->Position,
//                        pInvestorPosition->TodayPosition, pInvestorPosition->YdPosition, pInvestorPosition->OpenVolume);
                Types::Instrument_t instrument{""};
                strcpy(instrument.data(), pInvestorPosition->InstrumentID);
                if(Utils::isSTGInstrument(instrument.data()) == false){
                    auto itrQS = m_ctpPositionMap.find(instrument);
                    if (itrQS == m_ctpPositionMap.end()) {
                        CtpPosition ctpPosition;
                        m_ctpPositionMap[instrument] = ctpPosition;
                        itrQS = m_ctpPositionMap.find(instrument);
                    }

                    if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long) {
                        itrQS->second.buy_todayPosition += pInvestorPosition->TodayPosition;
                        itrQS->second.buy_position += pInvestorPosition->Position;
                        itrQS->second.buy_openVolume += pInvestorPosition->OpenVolume;

                    } else if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short) {
                        itrQS->second.sell_todayPosition += pInvestorPosition->TodayPosition;
                        itrQS->second.sell_position += pInvestorPosition->Position;
                        itrQS->second.sell_openVolume += pInvestorPosition->OpenVolume;
                    }
                }


            } else if (pInvestorPosition != nullptr && pRspInfo != nullptr) {
                spdlog::error("OnRspQryInvestorPosition : instrument={}, errorID={}, errorMsg={}",
                              pInvestorPosition->InstrumentID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                m_queryPositionPromise.set_value(pRspInfo->ErrorID);
            }

            if (bIsLast == true) {

                m_queryPositionPromise.set_value(0);
            }

        };

        void CtpTrader::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo,
                                            int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
                                                     CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                   bool bIsLast) {

        };

        void CtpTrader::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
                                           int nRequestID, bool bIsLast) {
            if (pInstrument != nullptr && (pInstrument->ProductClass == THOST_FTDC_PC_Options ||
                                           pInstrument->ProductClass == THOST_FTDC_PC_SpotOption)) {
                Types::InstrumentInfo instrumentInfo;
                strcpy(instrumentInfo.instrumentID.data(), pInstrument->InstrumentID);
                Utils::InstrumentToProduct(instrumentInfo.instrumentID, instrumentInfo.productID);
                instrumentInfo.expireDate = atoi(pInstrument->ExpireDate);
                instrumentInfo.optionType = pInstrument->OptionsType == THOST_FTDC_CP_CallOptions ? 'C' : 'P';
                instrumentInfo.strikePrice = pInstrument->StrikePrice;
                instrumentInfo.productIDClass = Types::ProductClass::option;
                instrumentInfo.exchanges = Utils::getExchangeType(pInstrument->ExchangeInstID);
                instrumentInfo.tickSize = pInstrument->PriceTick;
                instrumentInfo.multi = pInstrument->VolumeMultiple;
                strcpy(instrumentInfo.underly.data(), pInstrument->UnderlyingInstrID);
                if (strcmp("CFFEX", pInstrument->ExchangeID) == 0) {

                    if (strcmp(instrumentInfo.productID.data(), "IO") == 0) {
                        instrumentInfo.underly[0] = 'I';
                        instrumentInfo.underly[1] = 'F';
                    } else if (strcmp(instrumentInfo.productID.data(), "HO") == 0) {
                        instrumentInfo.underly[0] = 'I';
                        instrumentInfo.underly[1] = 'H';
                    } else if (strcmp(instrumentInfo.productID.data(), "MO") == 0) {
                        instrumentInfo.underly[0] = 'I';
                        instrumentInfo.underly[1] = 'M';
                    }
//                    fprintf(stderr, "OnRspQryInstrument instrument=%s, UnderlyingInstrID=%s, EXCHAGE=%s\n",
//                            instrumentInfo.instrumentID.data(), instrumentInfo.underly.data(), pInstrument->ExchangeID);
                }

                //     fprintf(stderr, "OnRspQryInstrument instrument=%s, product=%s\n", pInstrument->InstrumentID, pInstrument->ProductID);
                if (true ==  Utils::TradingHours::isProductIn(instrumentInfo.productID)) {
                    m_tradeInsinfoVec.emplace_back(instrumentInfo);
                } else {
                    spdlog::error("instrument={} is not in tradingdayHour.xml", pInstrument->InstrumentID);
                }

            }

            if (pRspInfo != nullptr) {
                m_queryInstrumentPromise.set_value(std::max(pRspInfo->ErrorID, 1));
            }

            if (bIsLast == true) {
                m_queryInstrumentPromise.set_value(0);
            }

        };

        void CtpTrader::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
           // fprintf(stderr, "OnRspQryDepthMarketData %s %.3f\n", pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice);
            if (pDepthMarketData != nullptr) {
                Types::MarketData *marketData = new Types::MarketData();

                 Types::Instrument_t instemp{""};
                strcpy(instemp.data(), pDepthMarketData->InstrumentID);
                marketData->epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                marketData->lastPrice = pDepthMarketData->LastPrice;
                marketData->upperLimitPrice = pDepthMarketData->UpperLimitPrice;
                marketData->lowerLimitPrice = pDepthMarketData->LowerLimitPrice;
                marketData->settlementPrice = pDepthMarketData->SettlementPrice;
                marketData->highestPrice = pDepthMarketData->HighestPrice;
                marketData->lowestPrice = pDepthMarketData->LowestPrice;
                marketData->openPrice = pDepthMarketData->OpenPrice;

                marketData->lastPrice =  Utils::checkPriceValid(marketData->lastPrice);
                if(marketData->openPrice > 999999999.9){
                    marketData->highestPrice = marketData->lastPrice;
                    marketData->lowestPrice = marketData->lastPrice;
                    marketData->openPrice = marketData->lastPrice;
                }

                marketData->bidPrice[0] = pDepthMarketData->BidPrice1;
                marketData->askPrice[0] = pDepthMarketData->AskPrice1;
                if(pDepthMarketData->BidVolume1 == 0){
                    marketData->bidPrice[0] = marketData->lastPrice;
                }
                if(pDepthMarketData->AskVolume1 ==0){
                    marketData->askPrice[0] = marketData->lastPrice;
                }
                std::copy(std::begin(pDepthMarketData->InstrumentID), std::begin(pDepthMarketData->InstrumentID) + marketData->instrumentID.size(), std::begin(marketData->instrumentID));

                //            std::copy(std::begin(pDepthMarketData->TradingDay), std::begin(pDepthMarketData->TradingDay) + marketData->tradingDay.size(), std::begin(marketData->tradingDay));

                marketData->bidVolume[0] = pDepthMarketData->BidVolume1;
                marketData->askVolume[0] = pDepthMarketData->AskVolume1;

                std::copy(std::begin(pDepthMarketData->UpdateTime), std::begin(pDepthMarketData->UpdateTime) + marketData->updateTime.size(),std::begin(marketData->updateTime));

                marketData->milliSeconds = pDepthMarketData->UpdateMillisec;

                marketData->psSecond =  Utils::ToPsSeconds(marketData->updateTime);

                marketData->midPrice = (marketData->bidPrice[0] + marketData->askPrice[0]) *0.5;

                marketData->amount = pDepthMarketData->Turnover;
                marketData->volume = pDepthMarketData->Volume;
                marketData->oi = pDepthMarketData->OpenInterest;
                marketData->amount =  Utils::checkPriceValid(marketData->amount);
                marketData->oi =  Utils::checkPriceValid(marketData->oi);
                marketData->isInit = 1;
                m_initMarketDataVector.emplace_back(marketData);
            }
            if(bIsLast==true){
                m_queryMarketDataPromise.set_value(0);
            }
        };

        void CtpTrader::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            //   fprintf(stderr, "OnRspQrySettlementInfo : %s\n", pSettlementInfo->Content);

        };




        void
        CtpTrader::OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
                                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            int a = 0;
        };

        void CtpTrader::OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                       bool bIsLast) {

        };

        void CtpTrader::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryInvestorPositionCombineDetail(
                CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey,
                                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryInvestorProductGroupMargin(
                CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate,
                                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryMMInstrumentCommissionRate(
                CThostFtdcMMInstrumentCommissionRateField *pMMInstrumentCommissionRate,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryMMOptionInstrCommRate(CThostFtdcMMOptionInstrCommRateField *pMMOptionInstrCommRate,
                                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate,
                                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQrySecAgentTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::onUnderlyInfo(Types::UnderlyInfo const &underlyInfo) {

            for (auto &instrumentInfo: m_tradeInsinfoVec) {
                //  fprintf(stderr, "onUnderlyInfo : %s %s\n", instrumentInfo.instrumentID.data(), instrumentInfo.underly.data());
                if (strcmp(instrumentInfo.underly.data(), underlyInfo.instrumentID.data()) == 0) {
                    m_driver->send(instrumentInfo);
                }
            }
        }

        int CtpTrader::queryTradeInstrument() {
            CThostFtdcQryInstrumentField req;
            memset(&req, 0, sizeof(req));
            int iResult = m_pTradeApi->ReqQryInstrument(&req, ++m_requestID);
            auto reqInstrumentFuture = m_queryInstrumentPromise.get_future();
            auto status = reqInstrumentFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                spdlog::error("trade login deferred");
            } else if (status == std::future_status::timeout) {
                spdlog::error("trade login timeout");
                return -1;

            } else if (status == std::future_status::ready) {
                auto ret = reqInstrumentFuture.get();
                if (ret != 0) {
                    spdlog::error("trade login retcode {}", ret);
                    return -1;
                }
            }

            return 0;
        }

        int CtpTrader::_queryMarketData() {

            CThostFtdcQryDepthMarketDataField qryDepthMarketDataField;
            memset(&qryDepthMarketDataField, 0 ,sizeof(CThostFtdcQryDepthMarketDataField));
            auto reqRet = m_pTradeApi->ReqQryDepthMarketData(&qryDepthMarketDataField, m_requestID++);

            auto reqInstrumentFuture = m_queryMarketDataPromise.get_future();
            auto status = reqInstrumentFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                spdlog::error("trade _queryMarketData deferred");
            } else if (status == std::future_status::timeout) {
                spdlog::error("trade _queryMarketData timeout");
                return -1;

            } else if (status == std::future_status::ready) {
                auto ret = reqInstrumentFuture.get();
                if (ret != 0) {
                    spdlog::error("trade _queryMarketData retcode {}", ret);
                    return -1;
                }
            }

            return 0;
        }


        int CtpTrader::start(int &tradingday) {
            boost::property_tree::ptree pt;
            boost::property_tree::read_xml(m_configPath, pt);
            m_ctpConnection.username = pt.get_child("ThostUser2.ConnectConfig.Username").get<std::string>(
                    "<xmlattr>.value");
            m_ctpConnection.password = pt.get_child("ThostUser2.ConnectConfig.Password").get<std::string>(
                    "<xmlattr>.value");
            m_ctpConnection.authCode = pt.get_child("ThostUser2.ConnectConfig.Auth").get<std::string>("<xmlattr>.code");
            m_ctpConnection.appId = pt.get_child("ThostUser2.ConnectConfig.AppId").get<std::string>("<xmlattr>.code");
            m_ctpConnection.tradeFront = pt.get_child("ThostUser2.ConnectConfig.TradeFront").get<std::string>(
                    "<xmlattr>.address");
            m_ctpConnection.brokerId = pt.get_child("ThostUser2.ConnectConfig.BrokerID").get<std::string>(
                    "<xmlattr>.name");
            m_ctpConnection.interpreterConfig = pt.get_child(
                    "ThostUser2.ConnectConfig.InterpreterConfig").get<std::string>("<xmlattr>.value");

            //   m_interpreter = new NetRootInterpreter(m_pTradeApi,  m_ctpConnection.interpreterConfig,  m_maxOrderRef);

            auto prefixOrder = pt.get_child("ThostUser2.ConnectConfig.OrderPrefix").get<std::string>("<xmlattr>.value");
            std::copy(std::begin(prefixOrder), std::end(prefixOrder), std::begin(m_ctpConnection.OrderPrefix));
            m_pTradeApi = CThostFtdcTraderApi::CreateFtdcTraderApi("./logs/");
            m_pTradeApi->RegisterSpi(this);

            m_pTradeApi->SubscribePrivateTopic(THOST_TERT_QUICK);
            m_pTradeApi->SubscribePublicTopic(THOST_TERT_QUICK);

            std::array<char, 128> frontAddress{""};
            std::copy(std::begin(m_ctpConnection.tradeFront), std::end(m_ctpConnection.tradeFront),
                      std::begin(frontAddress));

            m_pTradeApi->RegisterFront(frontAddress.data());
            m_pTradeApi->Init();

            auto loginFuture = m_loginPromise.get_future();
            auto status = loginFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                fprintf(stderr, "trade login deferred\n");
                spdlog::error("trade login deferred");
            } else if (status == std::future_status::timeout) {
                fprintf(stderr, "trade login timeout\n");
                spdlog::error("trade login timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = loginFuture.get();
                if (ret != 0) {
                    fprintf(stderr, "trade login retcode %d\n", ret);
                    spdlog::error("trade login retcode : {}", ret);
                    return -1;
                }
            }


            tradingday = m_tradingDay;
            m_interpreter = new CtpInterpreter(m_pTradeApi, m_ctpConnection, m_frontID, m_sessionID, m_maxOrderRef);
            int marketRetcode = this->_queryMarketData();
            sleep(1);
            auto insRetcode = this->queryTradeInstrument();
            sleep(1);
            auto pendingOrderRetCode = this->_queryOrder();
            for (auto& pendingOrder  : m_queryOrderStatusMap) {
                if (Utils::checkTerminal(pendingOrder.second.orderStatus) == false) {
                    CThostFtdcInputOrderActionField actionField;
                    memset(&actionField, 0, sizeof(CThostFtdcInputOrderActionField));
                    strcpy(actionField.InstrumentID, pendingOrder.second.instrument.data());
                    actionField.FrontID = pendingOrder.second.frontId;
                    actionField.SessionID = pendingOrder.second.sessionId;
                    strcpy(actionField.OrderRef, pendingOrder.second.orderRef.c_str());
                    strcpy(actionField.OrderSysID, pendingOrder.first.c_str());
                    actionField.ActionFlag = THOST_FTDC_AF_Delete;
                    strcpy( actionField.BrokerID, m_ctpConnection.brokerId.c_str());
                    strcpy( actionField.UserID, m_ctpConnection.username.c_str());
                    int ret = m_pTradeApi->ReqOrderAction(&actionField, 0);
                    int a =1;
                }
            }
            sleep(1);
            auto posRetcode = this->_queryAllPosition();
            m_isLogin = true;


            return insRetcode | posRetcode | marketRetcode || pendingOrderRetCode;
        }


        char CtpTrader::getOrderHedgeFlag( Types::HedgeType ht) {

            switch (ht) {
                case  Types::HedgeType::hedge:
                    return THOST_FTDC_HF_Hedge;
                case  Types::HedgeType::spec:
                    return THOST_FTDC_HF_Speculation;

            }

            return ',';
        }


        void CtpTrader::sendOrder( Types::OrderField const &input_orderField) {
            int nRetCode = 0;
            auto orderField = m_interpreter->sendOrder(input_orderField, nRetCode);
            if (orderField->orderStatus ==  Types::OrderStatus::failed) {
                auto event = convertToEvent(orderField, &m_eventDataList);
                m_driver->callback_asyncEventData(event, orderField->policyID);
                spdlog::error(
                        "send order failed={}, instrumentId={}, tradeOrderID={}, policyOrderID={}, orderPrice={}, assignId={}",
                        nRetCode, orderField->instrumentID.data(),
                        orderField->tOrderID, orderField->pOrderID, orderField->orderPrice, orderField->tOrderID);
                return;
            }

        }

        void CtpTrader::sendAbtrgOrder( Types::ArbitrageOrderField const &input_orderField) {
            int nRetCode = 0;
            auto orderField = m_interpreter->sendAbtrgOrder(input_orderField, nRetCode);
            if (orderField->orderStatus ==  Types::OrderStatus::failed) {
                auto event = convertToAbtrgEvent(orderField, &m_eventDataList);
                m_driver->callback_asyncEventData(event, orderField->policyID);
                spdlog::error(
                        "send order failed={}, instrumentId={}, instrumentId={}, tradeOrderID={}, policyOrderID={}, orderPrice={}, assignId={}",
                        nRetCode, orderField->instrumentIDs[0].data(), orderField->instrumentIDs[1].data(),
                        orderField->tOrderID, orderField->pOrderID, orderField->orderPrice, orderField->tOrderID);
                return;
            }
        }

        bool CtpTrader::cancelOrder( Types::OrderField const &inputOrder, int64_t &epoch_time) {
            m_interpreter->cancelOrder(inputOrder, epoch_time);
            return true;
        }

        bool CtpTrader::cancelAbtrgOrder( Types::ArbitrageOrderField const &inputAbtrgOrder,
                                         int64_t &epoch_time) {
            m_interpreter->cancelAbtrgOrder(inputAbtrgOrder, epoch_time);
            return true;
        }

        void CtpTrader::onQuerySymbol( Types::QuerySymbol const &querySymbol) {
             Types::OnQuerySymbol onQuerySymbol;
            onQuerySymbol.id = querySymbol.id;
            onQuerySymbol.tradingDay = m_tradingDay;
            //   onQuerySymbol.cancelOrderCounts = m_queryPosition.cancelOrderCounts;
            strcpy(onQuerySymbol.instrumentID.data(), querySymbol.instrumentID.data());
            auto itrQS = m_onQuerySymbolMap.find(querySymbol.instrumentID);
            if (itrQS == m_onQuerySymbolMap.end()) {
                Types::TradePosition tradePosition;
                memcpy(&onQuerySymbol.tradePosition, &tradePosition, sizeof(Types::TradePosition));
            } else {
                memcpy(&onQuerySymbol.tradePosition, &itrQS->second, sizeof(Types::TradePosition));
                m_driver->send(onQuerySymbol);
            }

//            if(0!= this->_queryOrder(m_queryPosition.instrumentid)){
//                assert(false);
//                return ;
//            }

        }


        int CtpTrader::_querySingleTrade(Types::Instrument_t const &queyInstrument) {
//            strcpy(m_queryMarketInstrument.data() ,queyInstrument.data());
//            CThostFtdcQryDepthMarketDataField qryDepthMarketDataField;
//            memset(&qryDepthMarketDataField, 0 ,sizeof(CThostFtdcQryDepthMarketDataField));
//            strcpy(qryDepthMarketDataField.InstrumentID, queyInstrument.data());
//            std::promise<int> instrumentPromise;
//            m_queryInstrumentPromise.swap(instrumentPromise);
//            auto reqRet = m_pTradeApi->ReqQryDepthMarketData(&qryDepthMarketDataField, m_requestID++);
//            auto instrumentFuture = m_queryInstrumentPromise.get_future();
//            auto status= instrumentFuture.wait_for(std::chrono::seconds(20));
//            if (status == std::future_status::deferred) {
//                spdlog::error( "query instrument {} deferred", queyInstrument.data());
//                return -1;
//            }
//            else if (status == std::future_status::timeout) {
//                spdlog::error( "query instrument {} timeout reqRet={}", queyInstrument.data(), reqRet);
//                return -2;
//            } else if (status == std::future_status::ready) {
//                auto ret = instrumentFuture.get();
//                if(ret!=0){
//                    spdlog::error( "query instrument  {} error retcode={}, reqRet={}", queyInstrument.data(), ret, reqRet);
//                    return  -3;
//                }
//            }
//
//            sleep(5);


            CThostFtdcQryTradeField qryTradeField;
            memset(&qryTradeField, 0, sizeof(CThostFtdcQryTradeField));
            strcpy(qryTradeField.BrokerID, m_ctpConnection.brokerId.c_str());
            strcpy(qryTradeField.InvestorID, m_ctpConnection.username.c_str());
            strcpy(qryTradeField.InstrumentID, queyInstrument.data());
            std::promise<int> traderPromise;
            m_queryTraderPromise.swap(traderPromise);
            //   m_QryRspTradeVec.clear();
            auto reqRet = m_pTradeApi->ReqQryTrade(&qryTradeField, 0);

            auto traderFuture = m_queryTraderPromise.get_future();
            auto status = traderFuture.wait_for(std::chrono::seconds(20));
            if (status == std::future_status::timeout) {
                spdlog::error("query trade {} timeout reqRet={}", qryTradeField.InstrumentID, reqRet);
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = traderFuture.get();
                if (ret != 0) {
                    spdlog::error("query trade  {} error retcode={}, reqRet={}", qryTradeField.InstrumentID, ret,
                                  reqRet);
                    return -1;
                } else {

                    return 0;

                }
            }

            spdlog::error("query trade end {} error  reqRet={}", qryTradeField.InstrumentID, reqRet);
            return -1;
        };


        int CtpTrader::_queryOrder() {

            CThostFtdcQryOrderField qryOrder;
            memset(&qryOrder, 0, sizeof(CThostFtdcQryOrderField));
          //  strcpy(qryOrder.InstrumentID, queryInstrument.data());
            strcpy(qryOrder.BrokerID, m_ctpConnection.brokerId.c_str());
            strcpy(qryOrder.InvestorID, m_ctpConnection.username.c_str());

            //         std::copy(std::begin(param.value), std::end(param.value), std::begin(symbol->tradePosition.instrument));
            std::promise<int> positionPromise;
            m_queryOrderPromise.swap(positionPromise);


            int reqCount{0};
            int retCode = -1;
            while (reqCount < 3 && retCode != 0) {
                retCode = m_pTradeApi->ReqQryOrder(&qryOrder, m_requestID++);
                reqCount++;
                sleep(1);
            }


            if (retCode != 0) {
                spdlog::error("ReqQryInvestorOrder  error retcode : {}", retCode);
            }
            auto positionFuture = m_queryOrderPromise.get_future();
            auto status = positionFuture.wait_for(std::chrono::seconds(10));
            if (status == std::future_status::deferred) {
                spdlog::error("deferred");
                return -1;
            } else if (status == std::future_status::timeout) {
                spdlog::error("query order  timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = positionFuture.get();
                if (ret != 0) {
                    spdlog::error("query order   error ret : {}", ret);
                    return -1;
                }

            }


            return 0;

        }


        int CtpTrader::_queryAllPosition() {

            CThostFtdcQryInvestorPositionField qryPosition;
            memset(&qryPosition, 0, sizeof(CThostFtdcQryInvestorPositionField));
            strcpy(qryPosition.BrokerID, m_ctpConnection.brokerId.c_str());
            strcpy(qryPosition.InvestorID, m_ctpConnection.username.c_str());

            //         std::copy(std::begin(param.value), std::end(param.value), std::begin(symbol->tradePosition.instrument));
            std::promise<int> positionPromise;
            m_queryPositionPromise.swap(positionPromise);

            int reqCount{0};
            int retCode = -1;
            while (reqCount < 3 && retCode != 0) {
                retCode = m_pTradeApi->ReqQryInvestorPosition(&qryPosition, m_requestID++);
                reqCount++;
                sleep(1);
            }


            if (retCode != 0) {
                spdlog::error("ReqQryInvestorPosition  error retcode : {}", retCode);
            }
            auto positionFuture = m_queryPositionPromise.get_future();
            auto status = positionFuture.wait_for(std::chrono::seconds(10));
            if (status == std::future_status::deferred) {
                spdlog::error("deferred");
                return -1;
            } else if (status == std::future_status::timeout) {
                spdlog::error("query position timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = positionFuture.get();
                if (ret != 0) {
                    spdlog::error("query position error ret : {}", ret);
                    return -1;
                }
            }

            for(auto ctp_itr : m_ctpPositionMap){
                Types::TradePosition tradePosition;
                tradePosition.T_buyHold = ctp_itr.second.buy_todayPosition;
                tradePosition.Y_buyHold =  ctp_itr.second.buy_position - ctp_itr.second.buy_todayPosition;
                tradePosition.T_sellHold = ctp_itr.second.sell_todayPosition;
                tradePosition.Y_sellHold =  ctp_itr.second.sell_position - ctp_itr.second.sell_todayPosition;
                tradePosition.openBuyVolume = ctp_itr.second.buy_openVolume;
                tradePosition.openSellVolume = ctp_itr.second.sell_openVolume;
                tradePosition.filledPosition = tradePosition.T_buyHold +  tradePosition.Y_buyHold - tradePosition.T_sellHold - tradePosition.Y_sellHold;
                m_onQuerySymbolMap[ctp_itr.first] = tradePosition;
//                fprintf(stderr, "m_onQuerySymbolMap %s , net=%d, T_buyHold=%d, Y_buyHold=%d, T_sellHold=%d, Y_sellHold=%d\n", ctp_itr.first.data(),
//                        tradePosition.filledPosition, tradePosition.T_buyHold, tradePosition.Y_buyHold, tradePosition.T_sellHold, tradePosition.Y_sellHold );
            }

            return retCode;
        }


        void CtpTrader::OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                      bool bIsLast) {

        };


        void CtpTrader::OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo,
                                           int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRtnOrder(CThostFtdcOrderField *ctpOrder) {
            if (ctpOrder->OrderRef[0] == m_ctpConnection.OrderPrefix[0] &&
                ctpOrder->OrderRef[1] == m_ctpConnection.OrderPrefix[1] &&
                ctpOrder->OrderRef[2] == m_ctpConnection.OrderPrefix[2] && m_isLogin == true) {
                //    fprintf(stderr, "OnRtnOrder instrument=%s, RequestID=%d, OrderRef=%s, OrderSysID=%s, OrderStatus=%c\n",ctpOrder->InstrumentID,
                //            ctpOrder->RequestID, ctpOrder->OrderRef,ctpOrder->OrderSysID, ctpOrder->OrderStatus);
                auto epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                if (ctpOrder->OrderType == THOST_FTDC_ORDT_Combination) {
                    auto abtrgOrder = m_interpreter->OnRtnAbtrgOrder(ctpOrder);
                    if (abtrgOrder != nullptr) {
                        auto receiveAbtrgOrder = m_receiveAbtrgOrderList.getNewMemory();
                        memcpy(receiveAbtrgOrder, abtrgOrder, sizeof( Types::ArbitrageOrderField));
                        receiveAbtrgOrder->epoch_time = epoch_time;
                        auto event = convertToAbtrgEvent(receiveAbtrgOrder, &m_eventDataList);
                        //         spdlog::debug("{} ctpRequestID:{}, CtpOrderSysID:{}, CtpInsertTime:{}, LimitPrice={}, policyOrderID:{}, tradeOrderID:{}, ctpOrdeStatus:{},  orderStatus:{}, epoch_time={}",
                        //                       __FUNCTION__, ctpOrder->RequestID , ctpOrder->OrderSysID ,ctpOrder->InsertTime, ctpOrder->LimitPrice,  receiveOrder->policyOrderID, receiveOrder->tradeOrderID,
                        //                         ctpOrder->OrderStatus,  Types::orderStatusMap[receiveOrder->orderStatus].data(), receiveOrder->epoch_time);

                        m_driver->callback_asyncEventData(event, receiveAbtrgOrder->policyID);
                        spdlog::info(
                                "{} {}  ctpRequestID:{}, CtpOrderSysID:{}, CtpInsertTime:{}, LimitPrice={}, VolumeTraded={}, policyOrderID:{}, tradeOrderID:{}, ctpOrdeStatus:{},  "
                                "orderStatus:{}, OrderRef={}, orderSysID={}, epoch_time={}",
                                __FUNCTION__, ctpOrder->InstrumentID, ctpOrder->RequestID, ctpOrder->OrderSysID,
                                ctpOrder->InsertTime, ctpOrder->LimitPrice, ctpOrder->VolumeTraded,
                                receiveAbtrgOrder->pOrderID,
                                receiveAbtrgOrder->tOrderID, ctpOrder->OrderStatus,
                                 Types::orderStatusMap[receiveAbtrgOrder->orderStatus].data(),
                                ctpOrder->OrderRef,
                                ctpOrder->OrderSysID, receiveAbtrgOrder->epoch_time);
                    }
                } else {
                    auto order = m_interpreter->OnRtnOrder(ctpOrder);
                    if (order != nullptr) {
                        auto receiveOrder = m_receiveOrderList.getNewMemory();
                        memcpy(receiveOrder, order, sizeof( Types::OrderField));
                        receiveOrder->epoch_time = epoch_time;
                        auto event = convertToEvent(receiveOrder, &m_eventDataList);
                        //         spdlog::debug("{} ctpRequestID:{}, CtpOrderSysID:{}, CtpInsertTime:{}, LimitPrice={}, policyOrderID:{}, tradeOrderID:{}, ctpOrdeStatus:{},  orderStatus:{}, epoch_time={}",
                        //                       __FUNCTION__, ctpOrder->RequestID , ctpOrder->OrderSysID ,ctpOrder->InsertTime, ctpOrder->LimitPrice,  receiveOrder->policyOrderID, receiveOrder->tradeOrderID,
                        //                         ctpOrder->OrderStatus,  Types::orderStatusMap[receiveOrder->orderStatus].data(), receiveOrder->epoch_time);

                        m_driver->callback_asyncEventData(event, receiveOrder->policyID);
                        spdlog::info(
                                "{} {}  ctpRequestID:{}, CtpOrderSysID:{}, CtpInsertTime:{}, LimitPrice={}, VolumeTraded={}, policyOrderID:{}, tradeOrderID:{}, ctpOrdeStatus:{},  "
                                "orderStatus:{}, OrderRef={}, orderSysID={}, epoch_time={}",
                                __FUNCTION__, ctpOrder->InstrumentID, ctpOrder->RequestID, ctpOrder->OrderSysID,
                                ctpOrder->InsertTime, ctpOrder->LimitPrice, ctpOrder->VolumeTraded,
                                receiveOrder->pOrderID,
                                receiveOrder->tOrderID, ctpOrder->OrderStatus,
                                 Types::orderStatusMap[receiveOrder->orderStatus].data(),
                                ctpOrder->OrderRef,
                                ctpOrder->OrderSysID, receiveOrder->epoch_time);
                    }
                }


            } else {
                /*    fprintf(stderr,
                          "OnRtnOrder instrument=%s, RequestID=%d, OrderRef=%s, OrderSysID=%s, OrderStatus=%c, CombHedgeFlag=%s, CombOffsetFlag=%s, LimitPrice=%f, "
                          "VolumeTraded=%d, OrderType=%c\n",
                          ctpOrder->InstrumentID, ctpOrder->RequestID, ctpOrder->OrderRef, ctpOrder->OrderSysID,
                          ctpOrder->OrderStatus, ctpOrder->CombHedgeFlag, ctpOrder->CombOffsetFlag,
                          ctpOrder->LimitPrice, ctpOrder->VolumeTraded, ctpOrder->OrderType); */
            }

        };

        void CtpTrader::OnRtnTrade(CThostFtdcTradeField *pTrade) {
//            fprintf(stderr, "OnRtnTrade instrument=%s,  OrderRef=%s, OrderSysID=%s,  HedgeFlag=%c, OffsetFlag=%c, LimitPrice=%f, VolumeTraded=%d, TradeType=%c\n",
//                    pTrade->InstrumentID, pTrade->OrderRef,pTrade->OrderSysID,  pTrade->HedgeFlag, pTrade->OffsetFlag,
//                    pTrade->Price, pTrade->Volume, pTrade->TradeType);
            auto epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            if (pTrade->TradeType == THOST_FTDC_TRDT_CombinationDerived) {
                if (pTrade->OrderRef[0] == m_ctpConnection.OrderPrefix[0] &&
                    pTrade->OrderRef[1] == m_ctpConnection.OrderPrefix[1] &&
                    pTrade->OrderRef[2] == m_ctpConnection.OrderPrefix[2]) {

                    auto abtrgOrder = m_interpreter->OnRtnAbtrgTrade(pTrade);
                    if (abtrgOrder != nullptr) {
                        auto receiveAbtrgOrder = m_receiveAbtrgOrderList.getNewMemory();
                        memcpy(receiveAbtrgOrder, abtrgOrder, sizeof( Types::ArbitrageOrderField));
                        receiveAbtrgOrder->epoch_time = epoch_time;
                        auto event = convertToAbtrgEvent(receiveAbtrgOrder, &m_eventDataList);
                        //         spdlog::debug("{} ctpRequestID:{}, CtpOrderSysID:{}, CtpInsertTime:{}, LimitPrice={}, policyOrderID:{}, tradeOrderID:{}, ctpOrdeStatus:{},  orderStatus:{}, epoch_time={}",
                        //                       __FUNCTION__, ctpOrder->RequestID , ctpOrder->OrderSysID ,ctpOrder->InsertTime, ctpOrder->LimitPrice,  receiveOrder->policyOrderID, receiveOrder->tradeOrderID,
                        //                         ctpOrder->OrderStatus,  Types::orderStatusMap[receiveOrder->orderStatus].data(), receiveOrder->epoch_time);

                        m_driver->callback_asyncEventData(event, receiveAbtrgOrder->policyID);
                        spdlog::info(
                                "{} {}  , CtpOrderSysID:{}, TradeTime:{}, Price={}, Volume={}, policyOrderID:{}, tradeOrderID:{}, "
                                "orderStatus:{}, OrderRef={}, orderSysID={}, epoch_time={}",
                                __FUNCTION__, pTrade->InstrumentID, pTrade->OrderSysID, pTrade->TradeTime,
                                pTrade->Price, pTrade->Volume, receiveAbtrgOrder->pOrderID,
                                receiveAbtrgOrder->tOrderID,
                                 Types::orderStatusMap[receiveAbtrgOrder->orderStatus].data(),
                                pTrade->OrderRef,
                                pTrade->OrderSysID, receiveAbtrgOrder->epoch_time);
                    }
                }

            }
        };

        void CtpTrader::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {

            if (pInputOrder != nullptr) {
                if (pInputOrder->OrderRef[0] == m_ctpConnection.OrderPrefix[0] &&
                    pInputOrder->OrderRef[1] == m_ctpConnection.OrderPrefix[1] &&
                    pInputOrder->OrderRef[2] == m_ctpConnection.OrderPrefix[2]) {
                    if (Utils::isAbtrgInstrument(pInputOrder->InstrumentID) == true) {
                        auto abtrgOrder = m_interpreter->OnErrRtnAbtrgOrderInsert(pInputOrder, pRspInfo);
                        if (abtrgOrder != nullptr) {
                            auto receiveAbtrgOrder = m_receiveAbtrgOrderList.getNewMemory();
                            memcpy(receiveAbtrgOrder, abtrgOrder, sizeof( Types::ArbitrageOrderField));
                            fprintf(stderr,
                                    "OnErrRtnOrderInsert instrument=%s, instrument=%s, RequestID=%d, requestID=%d, orderRef=%s\n",
                                    receiveAbtrgOrder->instrumentIDs[0].data(),
                                    receiveAbtrgOrder->instrumentIDs[1].data(), pInputOrder->RequestID,
                                    receiveAbtrgOrder->tOrderID, pInputOrder->OrderRef);
                            auto event = convertToAbtrgEvent(receiveAbtrgOrder, &m_eventDataList);
                            m_driver->callback_asyncEventData(event, receiveAbtrgOrder->policyID);
                            spdlog::error("OnErrRtnOrderInsert instrumentid={}, orderID={}, errorid={}, errormsg={}",
                                          pInputOrder->InstrumentID, pInputOrder->RequestID, pRspInfo->ErrorID,
                                          pRspInfo->ErrorMsg);
                        }
                    } else {
                        auto order = m_interpreter->OnErrRtnOrderInsert(pInputOrder, pRspInfo);
                        if (order != nullptr) {
                            auto receiveOrder = m_receiveOrderList.getNewMemory();
                            memcpy(receiveOrder, order, sizeof( Types::OrderField));
                            fprintf(stderr,
                                    "OnErrRtnOrderInsert instrument=%s, RequestID=%d, requestID=%d, orderRef=%s\n",
                                    receiveOrder->instrumentID.data(), pInputOrder->RequestID, receiveOrder->tOrderID,
                                    pInputOrder->OrderRef);
                            auto event = convertToEvent(receiveOrder, &m_eventDataList);
                            m_driver->callback_asyncEventData(event, receiveOrder->policyID);
                            spdlog::error("OnErrRtnOrderInsert instrumentid={}, orderID={}, errorid={}, errormsg={}",
                                          pInputOrder->InstrumentID, pInputOrder->RequestID, pRspInfo->ErrorID,
                                          pRspInfo->ErrorMsg);
                        }
                    }

                }

            }

            if (pRspInfo != nullptr && pRspInfo->ErrorID != 0) {
                //   auto order = m_orderList.getOrderById(pInputOrder->RequestID);
                //   order->orderStatus =  Types::OrderStatus::failed;
                //     m_driver->callbackOrder_async(order->callback, *order, order->orderThreadId);
                spdlog::error("OnErrRtnOrderInsert :  error {} {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            }
        };

        void
        CtpTrader::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {

            if (pRspInfo->ErrorID != 0) {

                spdlog::info("OnErrRtnOrderAction : OrderRef={}, OrderSysID={}, error {} {}", pOrderAction->OrderRef,
                             pOrderAction->OrderSysID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            }


        };

        void CtpTrader::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) {

        };

        void CtpTrader::OnRtnBulletin(CThostFtdcBulletinField *pBulletin) {

        };

        void CtpTrader::OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) {

        };

        void CtpTrader::OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder) {

        };

        void CtpTrader::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) {

        };

        void CtpTrader::OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder,
                                                CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction,
                                                CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote,
                                               CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnRtnQuote(CThostFtdcQuoteField *pQuote) {

        };

        void CtpTrader::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo) {

        };

        void
        CtpTrader::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {

        };

        void
        CtpTrader::OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken) {

        };

        void CtpTrader::OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction,
                                                 CThostFtdcRspInfoField *pRspInfo) {

        };


        void CtpTrader::OnRtnCombAction(CThostFtdcCombActionField *pCombAction) {

        };

        void CtpTrader::OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction,
                                                 CThostFtdcRspInfoField *pRspInfo) {

        };

        void
        CtpTrader::OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                                            int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void
        CtpTrader::OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos,
                                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQueryCFMMCTradingAccountToken(
                CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer) {
            printf("<OnRtnFromBankToFutureByBank>\n");

        };

        void CtpTrader::OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer) {

        };

        void CtpTrader::OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal) {

        };

        void CtpTrader::OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal) {

        };

        void CtpTrader::OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer) {

        };

        void CtpTrader::OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer) {

        };

        void CtpTrader::OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {

        };

        void CtpTrader::OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {

        };

        void CtpTrader::OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount) {

        };

        void CtpTrader::OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer,
                                                     CThostFtdcRspInfoField *pRspInfo) {


        };

        void CtpTrader::OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer,
                                                     CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal,
                                                                 CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal,
                                                                 CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount,
                                                         CThostFtdcRspInfoField *pRspInfo) {

        };

        void CtpTrader::OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal) {

        };

        void CtpTrader::OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal) {

        };

        void CtpTrader::OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer,
                                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer,
                                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

        };

        void CtpTrader::OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount,
                                                           CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                           bool bIsLast) {

        };

        void CtpTrader::OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount) {

        };

        void CtpTrader::OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount) {

        };

        void CtpTrader::OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount) {

        };

    }
}
