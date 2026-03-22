//
// Created by zhangyw on 3/9/21.
//

#include "EESTrader.h"

namespace Cosmos {
    namespace Trader {
        int EESTrader::start(int & tradingday) {
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
            m_eitradConnection.port = pt.get_child("ThostUser2.ConnectConfig.port").get<int>("<xmlattr>.value");
//            std::string NRConfigPath = pt.get_child("ThostUser2.ConnectConfig.NRTradeConfig").get<std::string>(
//                    "<xmlattr>.value");

//            m_eitradConnection.brokerId = pt.get_child("ThostUser2.ConnectConfig.BrokerID").get<std::string>("<xmlattr>.name");


            TapAPIApplicationInfo stAppInfo;
            strcpy(stAppInfo.AuthCode, m_eitradConnection.authCode.c_str());
            printf("EiTradeSpi::start : authCode=%s\n", stAppInfo.AuthCode);
            strcpy(stAppInfo.KeyOperationLogPath, "");
            TAPIINT32 iResult = TAPIERROR_SUCCEED;
            ITapTradeAPI *pAPI = CreateTapTradeAPI(&stAppInfo, iResult);
            if (pAPI == nullptr) {
                spdlog::error("create trade api failed : iResult={} ,AuthCode={}", iResult, stAppInfo.AuthCode);
                return 1;
            }
            pAPI->SetAPINotify(this);

            this->SetAPI(pAPI);

            printf("host=%s, port=%d\n", m_eitradConnection.host.c_str(), m_eitradConnection.port);
            TAPIINT32 iErr = TAPIERROR_SUCCEED;
            iErr = m_pTradeApi->SetHostAddress(m_eitradConnection.host.c_str(), m_eitradConnection.port);
            if (TAPIERROR_SUCCEED != iErr) {
                spdlog::error("SetHostAddress Error: {}", iErr);
            }
            TapAPITradeLoginAuth stLoginAuth;
            memset(&stLoginAuth, 0, sizeof(stLoginAuth));
            strcpy(stLoginAuth.UserNo, m_eitradConnection.username.c_str());
            strcpy(stLoginAuth.AuthCode, m_eitradConnection.authCode.c_str());
            strcpy(stLoginAuth.AppID, m_eitradConnection.appId.c_str());
            strcpy(stLoginAuth.Password, m_eitradConnection.password.c_str());
            stLoginAuth.ISModifyPassword = APIYNFLAG_NO;
            stLoginAuth.ISDDA = APIYNFLAG_NO;
            fprintf(stderr,"username=%s, password=%s, authcode=%s, appid=%s\n",
                   stLoginAuth.UserNo, stLoginAuth.Password, stLoginAuth.AuthCode, stLoginAuth.AppID);
            std::promise<int> tempPromise;
            m_loginPromise.swap(tempPromise);
            iErr = m_pTradeApi->Login(&stLoginAuth);
            if (TAPIERROR_SUCCEED != iErr) {
                spdlog::error("Login trade Error: {}", iErr);

            }


            auto loginFuture = m_loginPromise.get_future();
            auto status = loginFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                printf("EETrade login deferred\n");
                spdlog::error("EETrade login deferred");
                return -1;
            } else if (status == std::future_status::timeout) {
                printf("EETrade login timeout\n");
                spdlog::error("EETrade login timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = loginFuture.get();
                if (ret != 0) {
                    printf("EETrade login retcode : %d\n", ret);
                    spdlog::error("EETrade login retcode : {}", ret);
                    return -1;
                }
            }

            tradingday = m_tradingDay;
        //    m_pTradeApi->QryCommodity(&m_uiSessionID);
            TapAPIOrderQryReq tapAPIOrderQryReq;
            tapAPIOrderQryReq.OrderQryType = TAPI_ORDER_QRY_TYPE_ALL;
            std::promise<int> temptempPromise;
            m_queryOrderPromise.swap(temptempPromise);
            m_pTradeApi->QryOrder(&m_uiSessionID , &tapAPIOrderQryReq);
            auto orderFuture = m_queryOrderPromise.get_future();
            status = orderFuture.wait_for(std::chrono::seconds(50));
            if (status == std::future_status::deferred) {
                printf("EETrade orderFuture deferred\n");
                spdlog::error("EETrade orderFuture deferred");
                return -1;
            } else if (status == std::future_status::timeout) {
                printf("EETrade orderFuture timeout\n");
                spdlog::error("EETrade orderFuture timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = orderFuture.get();
                if (ret != 0) {
                    printf("EETrade orderFuture retcode : %d\n", ret);
                    spdlog::error("EETrade orderFuture retcode : {}", ret);
                    return -1;
                }
            }


            return 0;
        };


        void TAP_CDECL EESTrader::OnRspLogin(TAPIINT32 errorCode, const TapAPITradeLoginRspInfo *loginRspInfo) {
            if (TAPIERROR_SUCCEED == errorCode) {
                m_tradingDay = (loginRspInfo->TradeDate[0] - '0') * 10000000 +
                               (loginRspInfo->TradeDate[1] - '0') * 1000000 +
                               (loginRspInfo->TradeDate[2] - '0') * 100000 +
                               (loginRspInfo->TradeDate[3] - '0') * 10000;
                m_tradingDay += (loginRspInfo->TradeDate[5] - '0') * 1000 + (loginRspInfo->TradeDate[6] - '0') * 100;
                m_tradingDay += (loginRspInfo->TradeDate[8] - '0') * 10 + (loginRspInfo->TradeDate[9] - '0');

                //   m_loginPromise.set_value(0);
            } else {
                spdlog::error("OnRspLogin: errorCode = {}", errorCode);
                m_loginPromise.set_value(-1);
            }
        }

        void TAP_CDECL EESTrader::OnAPIReady() {
            spdlog::info("OnAPIReady");
            m_loginPromise.set_value(0);
        }

        void EESTrader::sendOrder( Types::OrderField const &input_orderField) {

            auto *eesOrderField = m_EESOrderList.getNewMemory();

            strncpy(eesOrderField->AccountNo, m_eitradConnection.username.c_str(), sizeof(eesOrderField->AccountNo));
            strncpy(eesOrderField->ExchangeNo, "ZCE", sizeof(eesOrderField->ExchangeNo));
            eesOrderField->CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
            eesOrderField->CommodityNo[0] = input_orderField.instrumentID[0];
            eesOrderField->CommodityNo[1] = input_orderField.instrumentID[1];
            eesOrderField->ContractNo[0] = input_orderField.instrumentID[2];
            eesOrderField->ContractNo[1] = input_orderField.instrumentID[3];
            eesOrderField->ContractNo[2] = input_orderField.instrumentID[4];
            eesOrderField->OrderType = TAPI_ORDER_TYPE_LIMIT;

            if (input_orderField.orderSide == Types::OrderSide::buy) {
                eesOrderField->OrderSide = TAPI_SIDE_BUY;
            } else if (input_orderField.orderSide == Types::OrderSide::sell) {
                eesOrderField->OrderSide = TAPI_SIDE_SELL;
            }

            //  stNewOrder.HedgeFlag= TAPI_HEDGEFLAG_T;
            eesOrderField->PositionEffect = getOrderPositionEffect(input_orderField.pet);
            //     eesOrderField->PositionEffect2 = TAPI_PositionEffect_NONE;

            eesOrderField->HedgeFlag = TAPI_HEDGEFLAG_T;
            //     eesOrderField->HedgeFlag2 = TAPI_HEDGEFLAG_NONE;

            eesOrderField->OrderPrice = input_orderField.orderPrice;

            eesOrderField->OrderQty = input_orderField.orderVolume;

            //     eesOrderField->CallOrPutFlag = TAPI_CALLPUT_FLAG_NONE;
            //    eesOrderField->CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;

            eesOrderField->OrderSource = TAPI_ORDER_SOURCE_ESUNNY_API;

            //     eesOrderField->IsRiskOrder = APIYNFLAG_NO;
            if (input_orderField.orderTimeType == Types::OrderTimeType::IOC) {
                eesOrderField->TimeInForce = TAPI_ORDER_TIMEINFORCE_FAK;
            } else {
                eesOrderField->TimeInForce = TAPI_ORDER_TIMEINFORCE_GFD;
            }


//            eesOrderField->TacticsType = TAPI_TACTICS_TYPE_NONE;
//            eesOrderField->TriggerCondition = TAPI_TRIGGER_CONDITION_NONE;
//            eesOrderField->TriggerPriceType = TAPI_TRIGGER_PRICE_NONE;
//            eesOrderField->AddOneIsValid = APIYNFLAG_NO;
//
//            eesOrderField->MarketLevel = TAPI_MARKET_LEVEL_0;
//            eesOrderField->FutureAutoCloseFlag = APIYNFLAG_NO; // V9.0.2.0 20150520

            m_sendOrderLock.lock();
            int assignId = 0;
            auto orderField = m_orderList->getNewMemory(assignId);
            memcpy(orderField, &input_orderField, sizeof( Types::OrderField));
            eesOrderField->RefInt = assignId;
            orderField->tOrderID = assignId;
            //    fprintf(stderr,"sendOrder : instrument=%s, requestID=%d orderRef=%s\n", orderField->instrumentID.data(), orderField->tOrderID, pCtpOrder->OrderRef);
            auto nRetCode = m_pTradeApi->InsertOrder(&m_uiSessionID, eesOrderField);
            m_sendOrderLock.unlock();
            if (nRetCode != 0) {
                orderField->orderStatus =  Types::OrderStatus::failed;
                auto event = convertToEvent(orderField, &m_eventDataList);
                m_driver->callback_asyncEventData(event, orderField->policyID);
                spdlog::error(
                        "send order failed={}, instrumentId={}, tradeOrderID={}, policyOrderID={}, orderPrice={}, assignId={}",
                        nRetCode, orderField->instrumentID.data(),
                        orderField->tOrderID, orderField->pOrderID, orderField->orderPrice, orderField->tOrderID);
                return;
            }

        };

        void EESTrader::sendAbtrgOrder( Types::ArbitrageOrderField const &input_abtrgOrderField) {
            auto *eesOrderField = m_EESOrderList.getNewMemory();

            strncpy(eesOrderField->AccountNo, m_eitradConnection.username.c_str(), sizeof(eesOrderField->AccountNo));
            strncpy(eesOrderField->ExchangeNo, "ZCE", sizeof(eesOrderField->ExchangeNo));
            if (input_abtrgOrderField.orderPrex[0] == 'I') {
                eesOrderField->CommodityNo[0] = input_abtrgOrderField.instrumentIDs[0][0];
                eesOrderField->CommodityNo[1] = input_abtrgOrderField.instrumentIDs[0][1];
                eesOrderField->CommodityNo[2] = '&';
                eesOrderField->CommodityNo[3] = input_abtrgOrderField.instrumentIDs[1][0];
                eesOrderField->CommodityNo[4] = input_abtrgOrderField.instrumentIDs[1][1];
                eesOrderField->CommodityType = TAPI_COMMODITY_TYPE_SPREAD_COMMODITY;
            } else if (input_abtrgOrderField.orderPrex[0] == 'S') {
                eesOrderField->CommodityNo[0] = input_abtrgOrderField.instrumentIDs[0][0];
                eesOrderField->CommodityNo[1] = input_abtrgOrderField.instrumentIDs[0][1];
                eesOrderField->CommodityType = TAPI_COMMODITY_TYPE_SPREAD_MONTH;
            }


            eesOrderField->ContractNo[0] = input_abtrgOrderField.instrumentIDs[0][2];
            eesOrderField->ContractNo[1] = input_abtrgOrderField.instrumentIDs[0][3];
            eesOrderField->ContractNo[2] = input_abtrgOrderField.instrumentIDs[0][4];


            eesOrderField->ContractNo2[0] = input_abtrgOrderField.instrumentIDs[1][2];
            eesOrderField->ContractNo2[1] = input_abtrgOrderField.instrumentIDs[1][3];
            eesOrderField->ContractNo2[2] = input_abtrgOrderField.instrumentIDs[1][4];

           // eesOrderField->OrderType =   TAPI_ORDER_TYPE_SWAP; //TAPI_ORDER_TYPE_LIMIT;
	      eesOrderField->OrderType = TAPI_ORDER_TYPE_LIMIT;

            if (input_abtrgOrderField.orderSides[0] == Types::OrderSide::buy) {
                eesOrderField->OrderSide = TAPI_SIDE_BUY;
            } else if (input_abtrgOrderField.orderSides[0] == Types::OrderSide::sell) {
                eesOrderField->OrderSide = TAPI_SIDE_SELL;
            }


             eesOrderField->PositionEffect =  getOrderPositionEffect(input_abtrgOrderField.pets[0]);
             eesOrderField->PositionEffect2 =  getOrderPositionEffect(input_abtrgOrderField.pets[1]);
            //     eesOrderField->PositionEffect2 = TAPI_PositionEffect_NONE;
	     if(eesOrderField->PositionEffect == TAPI_PositionEffect_OPEN && eesOrderField->PositionEffect2 != TAPI_PositionEffect_OPEN){
	              eesOrderField->OrderType =  TAPI_ORDER_TYPE_SWAP;
	     }else if(eesOrderField->PositionEffect != TAPI_PositionEffect_OPEN && eesOrderField->PositionEffect2 == TAPI_PositionEffect_OPEN){
	              eesOrderField->OrderType =  TAPI_ORDER_TYPE_SWAP;
	     }

            eesOrderField->HedgeFlag = TAPI_HEDGEFLAG_T;
            eesOrderField->HedgeFlag2 = TAPI_HEDGEFLAG_T;
            //     eesOrderField->HedgeFlag2 = TAPI_HEDGEFLAG_NONE;

            eesOrderField->OrderPrice = input_abtrgOrderField.orderPrice;
            eesOrderField->OrderQty = input_abtrgOrderField.orderVolume;

            eesOrderField->CallOrPutFlag = TAPI_CALLPUT_FLAG_NONE;
            eesOrderField->CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;

            eesOrderField->OrderSource = TAPI_ORDER_SOURCE_ESUNNY_API;

            eesOrderField->IsRiskOrder = APIYNFLAG_NO;
            if (input_abtrgOrderField.orderTimeType == Types::OrderTimeType::IOC) {
                eesOrderField->TimeInForce = TAPI_ORDER_TIMEINFORCE_FAK;
            } else {
                eesOrderField->TimeInForce = TAPI_ORDER_TIMEINFORCE_GFD;
            }


            eesOrderField->TacticsType = TAPI_TACTICS_TYPE_NONE;
            eesOrderField->TriggerCondition = TAPI_TRIGGER_CONDITION_NONE;
            eesOrderField->TriggerPriceType = TAPI_TRIGGER_PRICE_NONE;
            eesOrderField->AddOneIsValid = APIYNFLAG_NO;

            eesOrderField->MarketLevel = TAPI_MARKET_LEVEL_0;
            eesOrderField->FutureAutoCloseFlag = APIYNFLAG_NO; // V9.0.2.0 20150520

            m_sendOrderLock.lock();
            int assignId = 0;
            auto abtrgOrderField = m_abtrgOrderList->getNewMemory(assignId);
            memcpy(abtrgOrderField, &input_abtrgOrderField, sizeof( Types::ArbitrageOrderField));
            eesOrderField->RefInt = assignId;
            abtrgOrderField->tOrderID = assignId;
            //    fprintf(stderr,"sendOrder : instrument=%s, requestID=%d orderRef=%s\n", orderField->instrumentID.data(), orderField->tOrderID, pCtpOrder->OrderRef);
          //  auto epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
          //          std::chrono::high_resolution_clock::now().time_since_epoch()).count();

	    auto nRetCode = m_pTradeApi->InsertOrder(&m_uiSessionID, eesOrderField);
          //  fprintf(stderr, "sendAbtrgOrder : %d\n",   std::chrono::duration_cast<std::chrono::microseconds>(
          //          std::chrono::high_resolution_clock::now().time_since_epoch()).count() - epoch_time);
   
                  m_sendOrderLock.unlock();
            if (nRetCode != 0) {
                abtrgOrderField->orderStatus =  Types::OrderStatus::failed;
                auto event = convertToAbtrgEvent(abtrgOrderField, &m_eventDataList);
                m_driver->callback_asyncEventData(event, abtrgOrderField->policyID);
                spdlog::error(
                        "send order failed={}, instrumentId={}&{}, tradeOrderID={}, policyOrderID={}, orderPrice={}, assignId={}",
                        nRetCode, abtrgOrderField->instrumentIDs[0].data(), abtrgOrderField->instrumentIDs[1].data(),
                        abtrgOrderField->tOrderID, abtrgOrderField->pOrderID, abtrgOrderField->orderPrice,
                        abtrgOrderField->tOrderID);
                return;
            }
        };

        void EESTrader::cancelOrder( Types::OrderField const &inputOrder, int64_t &epoch_time) {


            int assignID;
            auto eesCancelOrder = m_EESOrderActionList->getNewMemory(assignID);
            strcpy(eesCancelOrder->OrderNo, inputOrder.orderRef.data());
            epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto ret = m_pTradeApi->CancelOrder(&m_uiSessionID, eesCancelOrder);

            if (0 != ret) {
                spdlog::error(
                        "EESTradeSpi::cancelOrder error : tradeOrderNo={}, tradeOrderID={}, tradeOrder={} , ret={}",
                        inputOrder.orderRef.data(), inputOrder.tOrderID, inputOrder.pOrderID, ret);
            }
        };

        void EESTrader::cancelAbtrgOrder( Types::ArbitrageOrderField const & abtrgInputOrder, int64_t& epoch_time){
            int assignID;
            auto eesCancelOrder = m_EESOrderActionList->getNewMemory(assignID);
	    //   memset(eesCancelOrder, 0 ,sizeof(eesCancelOrder));
	  //  std::copy(std::begin(abtrgInputOrder.orderSysID), std::end(abtrgInputOrder.orderSysID), std::begin(eesCancelOrder->OrderNo));
          //  fprintf(stderr, "cancelAbtrgOrder : OrderNo=%s, assignID=%d, isNull=%d, addr=%p\n", eesCancelOrder->OrderNo, assignID, eesCancelOrder==nullptr, eesCancelOrder);
	    strcpy(eesCancelOrder->OrderNo, abtrgInputOrder.orderSysID.data());
	  //  fprintf(stderr, "eesCancelOrder end %s  id=%s\n", eesCancelOrder->OrderNo,  abtrgInputOrder.orderSysID.data());
        //    epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(

        //            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto ret = m_pTradeApi->CancelOrder(&m_uiSessionID, eesCancelOrder);
       //	    fprintf(stderr, "cancelAbtrgOrder : cancelOrder %ld consumer %d\n" ,epoch_time ,std::chrono::duration_cast<std::chrono::microseconds>(
       //             std::chrono::high_resolution_clock::now().time_since_epoch()).count()- epoch_time);

            if (0 != ret) {
                spdlog::error(
                        "EESTradeSpi::cancelOrder error : tradeOrderNo={}, tradeOrderID={}, tradeOrder={} , ret={}",
                        abtrgInputOrder.orderRef.data(), abtrgInputOrder.tOrderID, abtrgInputOrder.pOrderID, ret);
            }
	  //  fprintf(stderr, "eesCancelOrder end2\n");
	  //  return true;

        };

        void EESTrader::onQuerySymbol( Types::QuerySymbol const &querySymbol) {
            auto itrQS = m_onQuerySymbolMap.find(querySymbol.symbol->instrumentID);
            if (itrQS == m_onQuerySymbolMap.end()) {
                memset(&m_queryPosition, 0, sizeof(QueryPosition));
                strcpy(m_queryPosition.instrumentid.data(), querySymbol.symbol->instrumentID.data());
                if (0 != this->_queryPosition(m_queryPosition.instrumentid)) {
                    fprintf(stderr,"onQuerySymbol faild instrument=%s\n", m_queryPosition.instrumentid.data());
                    assert(false);
                    return;
                }
                Types::TradePosition tradePosition;
                memcpy(&tradePosition, &m_queryPosition.tradePosition, sizeof(m_queryPosition.tradePosition));
                char logSymbolPath[128];
                memset(logSymbolPath, 0, sizeof(128));
                sprintf(logSymbolPath, "./logs/position/%s.txt", querySymbol.symbol->instrumentID.data());


                if (tradePosition.filledPosition != 0) {

                    int net = atoi(Utils::GetLastValueFromFile(logSymbolPath, "filledPosition",
                                                               querySymbol.symbol->instrumentID.data()).c_str());
                    if (net != tradePosition.filledPosition) {
                        fprintf(stderr, "instrument =%s, net=%d, filledPosition=%d\n",
                                querySymbol.symbol->instrumentID.data(),
                                net, tradePosition.filledPosition);
                        //   assert(false);
                    }
                    tradePosition.averagePrice = atof(Utils::GetLastValueFromFile(logSymbolPath, "averagePrice",
                                                                                  querySymbol.symbol->instrumentID.data()).c_str());
                }
                tradePosition.profit = atof(
                        Utils::GetLastValueFromFile(logSymbolPath, "profit",
                                                    querySymbol.symbol->instrumentID.data()).c_str());
                auto otrItr = m_OTRMap.find(querySymbol.symbol->instrumentID);
                if (otrItr != m_OTRMap.end()){
                    tradePosition.infoOrderNumb = otrItr->second.infoOrderNumb;
                    tradePosition.filledOrderNumb = otrItr->second.filledOrderNumb;
                }

                m_onQuerySymbolMap[m_queryPosition.instrumentid] = tradePosition;
                itrQS = m_onQuerySymbolMap.find(querySymbol.symbol->instrumentID);
            }


             Types::OnQuerySymbol onQuerySymbol;
            memcpy(&onQuerySymbol.tradePosition, &itrQS->second, sizeof(m_queryPosition.tradePosition));
            onQuerySymbol.id = querySymbol.id;
            onQuerySymbol.tradingDay = m_tradingDay;
            //   onQuerySymbol.cancelOrderCounts = m_queryPosition.cancelOrderCounts;
            strcpy(onQuerySymbol.instrument.data(), querySymbol.symbol->instrumentID.data());
            m_driver->send(onQuerySymbol);
        };

        int EESTrader::_queryPosition( Types::Instrument_t queryInstrument) {

            TapAPIPositionQryReq tapAPIPositionQryReq;



            //         std::copy(std::begin(param.value), std::end(param.value), std::begin(symbol->tradePosition.instrument));
            std::promise<int> positionPromise;
            m_queryPositionPromise.swap(positionPromise);

            int reqCount{0};
            int retCode = -1;
            while (reqCount < 3 && retCode != 0) {
                retCode = m_pTradeApi->QryPosition(&m_uiSessionID, &tapAPIPositionQryReq);
                reqCount++;
                sleep(1);
            }


            if (retCode != 0) {
                spdlog::error("ReqQryInvestorPosition {} error retcode : {}", queryInstrument.data(), retCode);
            }
            auto positionFuture = m_queryPositionPromise.get_future();
            auto status = positionFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                spdlog::error("deferred");
                return -1;
            } else if (status == std::future_status::timeout) {
                spdlog::error("query position {} timeout", queryInstrument.data());
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = positionFuture.get();
                if (ret != 0) {
                    spdlog::error("query position  {} error ret : {}", queryInstrument.data(), ret);
                    return -1;
                }
            }
            return 0;
        }


        void EESTrader::OnRspQryPosition(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast,
                                         const TapAPIPositionInfo *info) {

            if (info != nullptr && errorCode == TAPIERROR_SUCCEED) {
                Types::Instrument_t infoInstrument{""};
                sprintf(infoInstrument.data(), "%s%s", info->CommodityNo, info->ContractNo);


                if (strcmp(m_queryPosition.instrumentid.data(), infoInstrument.data()) == 0) {
                    fprintf(stderr,
                            "OnRspQryPosition instrument=%s, direction=%c, Position=%d, isHistory=%c, isLast=%c\n",
                            infoInstrument.data(), info->MatchSide,
                            info->PositionQty, info->IsHistory, isLast);
                    if (info->MatchSide == TAPI_SIDE_BUY && info->IsHistory == 'N') {
                        m_queryPosition.tradePosition.T_buyHold += info->PositionQty;

                    } else if (info->MatchSide == TAPI_SIDE_BUY && info->IsHistory == 'Y') {
                        m_queryPosition.tradePosition.Y_buyHold += info->PositionQty;
                    } else if (info->MatchSide == TAPI_SIDE_SELL && info->IsHistory == 'N') {
                        m_queryPosition.tradePosition.T_sellHold += info->PositionQty;
                    } else if (info->MatchSide == TAPI_SIDE_SELL && info->IsHistory == 'Y') {
                        m_queryPosition.tradePosition.Y_sellHold += info->PositionQty;
                    }

                }


            } else if (info != nullptr && errorCode != TAPIERROR_SUCCEED) {
                spdlog::error("OnRspQryInvestorPosition : CommodityNo={}, ContractNo={}, errorCode={}",
                              info->CommodityNo, info->ContractNo, errorCode);
                m_queryPositionPromise.set_value(-2);
            }

            if (isLast == 'Y') {
                m_queryPosition.tradePosition.filledPosition =
                        m_queryPosition.tradePosition.Y_buyHold + m_queryPosition.tradePosition.T_buyHold -
                        m_queryPosition.tradePosition.T_sellHold - m_queryPosition.tradePosition.Y_sellHold;
                m_queryPositionPromise.set_value(0);
            }

        }


         void EESTrader::OnRspQryOrder(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo *info){
           if(info!=nullptr){ 
	     if(info->CommodityType == TAPI_COMMODITY_TYPE_SPREAD_COMMODITY){
                 Types::Instrument_t hedgeInstrument{""};
                 Types::Instrument_t  tradeInstrument{""};

                 hedgeInstrument[0] = info->CommodityNo[0] ;
                 hedgeInstrument[1] = info->CommodityNo[1];
                 hedgeInstrument[2] = info->ContractNo[0] ;
                 hedgeInstrument[3] = info->ContractNo[1];
                 hedgeInstrument[4] = info->ContractNo[2];

                 tradeInstrument[0] = info->CommodityNo[3] ;
                 tradeInstrument[1] = info->CommodityNo[4];
                 tradeInstrument[2] = info->ContractNo2[0] ;
                 tradeInstrument[3] = info->ContractNo2[1];
                 tradeInstrument[4] = info->ContractNo2[2];

                 setOTR(hedgeInstrument, info->OrderState);
                 setOTR(tradeInstrument, info->OrderState);
             }
             else if(info->CommodityType == TAPI_COMMODITY_TYPE_SPREAD_MONTH){
                 Types::Instrument_t hedgeInstrument{""};
                 Types::Instrument_t  tradeInstrument{""};

                 hedgeInstrument[0] = info->CommodityNo[0] ;
                 hedgeInstrument[1] = info->CommodityNo[1];
                 hedgeInstrument[2] = info->ContractNo[0] ;
                 hedgeInstrument[3] = info->ContractNo[1];
                 hedgeInstrument[4] = info->ContractNo[2];

                 tradeInstrument[0] = info->CommodityNo[0] ;
                 tradeInstrument[1] = info->CommodityNo[1];
                 tradeInstrument[2] = info->ContractNo2[0] ;
                 tradeInstrument[3] = info->ContractNo2[1];
                 tradeInstrument[4] = info->ContractNo2[2];

                 setOTR(hedgeInstrument, info->OrderState);
                 setOTR(tradeInstrument, info->OrderState);
             }else if(info->CommodityType == TAPI_COMMODITY_TYPE_FUTURES){
                 Types::Instrument_t hedgeInstrument{""};
                 hedgeInstrument[0] = info->CommodityNo[0] ;
                 hedgeInstrument[1] = info->CommodityNo[1];
                 hedgeInstrument[2] = info->ContractNo[0] ;
                 hedgeInstrument[3] = info->ContractNo[1];
                 hedgeInstrument[4] = info->ContractNo[2];
                 setOTR(hedgeInstrument, info->OrderState);
             }
           //  fprintf(stderr, "%s,%s,%s,%s,%c\n", info->CommodityNo, info->ContractNo, info->ContractNo2, info->OrderNo, info->OrderState);
            }
             if(isLast == 'Y'){
                 m_queryOrderPromise.set_value(0);
             }
        } ;

        void EESTrader::setOTR(Types::Instrument_t& instrument,  char orderStatus){
             auto itr = m_OTRMap.find(instrument);
             if (itr == m_OTRMap.end()){
                 OTR otr;
                 m_OTRMap[instrument] = otr;
                 itr = m_OTRMap.find(instrument);
             }

             if(orderStatus == TAPI_ORDER_STATE_CANCELED){
                 itr->second.infoOrderNumb +=2;

             }else if(orderStatus == TAPI_ORDER_STATE_LEFTDELETED || orderStatus ==TAPI_ORDER_STATE_PARTFINISHED || orderStatus == TAPI_ORDER_STATE_FINISHED){
                 itr->second.infoOrderNumb +=1;
                 itr->second.filledOrderNumb +=1;
             }
        };


        void EESTrader::OnRtnOrder(const TapAPIOrderInfoNotice *info){
            if(info->OrderInfo->CommodityType == TAPI_COMMODITY_TYPE_FUTURES){

            }else {
                auto epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                auto abtrgOrder = m_abtrgOrderList->getMemoryById(info->OrderInfo->RefInt);
                update_AbtrgOrder(info, abtrgOrder);
                if(abtrgOrder->orderStatus != Types::OrderStatus::unknown){
                    auto receiveAbtrgOrder = m_receiveAbtrgOrderList.getNewMemory();
                    memcpy(receiveAbtrgOrder, abtrgOrder, sizeof( Types::ArbitrageOrderField));
                    receiveAbtrgOrder->epoch_time = epoch_time;
                    auto event = convertToAbtrgEvent(receiveAbtrgOrder, &m_eventDataList);
                    //         spdlog::debug("{} ctpRequestID:{}, CtpOrderSysID:{}, CtpInsertTime:{}, LimitPrice={}, policyOrderID:{}, tradeOrderID:{}, ctpOrdeStatus:{},  orderStatus:{}, epoch_time={}",
                    //                       __FUNCTION__, ctpOrder->RequestID , ctpOrder->OrderSysID ,ctpOrder->InsertTime, ctpOrder->LimitPrice,  receiveOrder->policyOrderID, receiveOrder->tradeOrderID,
                    //                         ctpOrder->OrderStatus,  Types::orderStatusMap[receiveOrder->orderStatus].data(), receiveOrder->epoch_time);

                    m_driver->callback_asyncEventData(event, receiveAbtrgOrder->policyID);
                   /* spdlog::info(
                            "{} CommodityNo={} ContractNo:{}, ContractNo2:{}, CommodityType:{}, RefInt={}, pOrderID={}, tOrderID:{}, OrderState:{}, orderStatus:{},  "
                            "OrderNo:{}, OrderMatchQty={},  OrderMatchQty2={}, epoch_time={}",
                            __FUNCTION__, info->OrderInfo->CommodityNo, info->OrderInfo->ContractNo, info->OrderInfo->ContractNo2, info->OrderInfo->CommodityType, info->OrderInfo->RefInt,
                            receiveAbtrgOrder->pOrderID,receiveAbtrgOrder->tOrderID, info->OrderInfo->OrderState, Types::orderStatusMap[receiveAbtrgOrder->orderStatus].data(),
                            info->OrderInfo->OrderNo,  info->OrderInfo->OrderMatchQty, info->OrderInfo->OrderMatchQty2 ,receiveAbtrgOrder->epoch_time); */
                }

            }
           /* fprintf(stderr, "OnRtnOrder errorCode=%d, status=%c, orderNo=%s, tradeNo=%s,  CommodityNo=%s, ContractNo=%s,ContractNo2=%s, ErrorCode=%d, ErrorText=%s, "
                            "OrderMatchPrice=%.3f, OrderMatchQty=%d, OrderMatchPrice2=%.3f, OrderMatchQty2=%d\n",
                    info->ErrorCode, info->OrderInfo->OrderState, info->OrderInfo->OrderNo, info->OrderInfo->TradeNo, info->OrderInfo->CommodityNo,
                    info->OrderInfo->ContractNo, info->OrderInfo->ContractNo2, info->OrderInfo->ErrorCode, info->OrderInfo->ErrorText,
                    info->OrderInfo->OrderMatchPrice, info->OrderInfo->OrderMatchQty, info->OrderInfo->OrderMatchPrice2, info->OrderInfo->OrderMatchQty2 ); */
        };

        void EESTrader::update_AbtrgOrder(const TapAPIOrderInfoNotice * info, Types::ArbitrageOrderField *abtrgOrder)
        {

            switch (info->OrderInfo->OrderState)
            {
                case TAPI_ORDER_STATE_QUEUED:
                    std::copy(std::begin(info->OrderInfo->OrderNo), std::end(info->OrderInfo->OrderNo), std::begin(abtrgOrder->orderSysID) );
                    abtrgOrder->orderStatus = Types::OrderStatus::accept;
                    break;
                case TAPI_ORDER_STATE_FAIL:
                    spdlog::error("Order failed : instrumentID={}{}, RefInt={}, TradeNo={}, ErrorCode={}, ErrorText={}",
                                  info->OrderInfo->CommodityNo, info->OrderInfo->ContractNo, info->OrderInfo->RefInt,
                                  info->OrderInfo->TradeNo,info->OrderInfo->ErrorCode, info->OrderInfo->ErrorText);
                    abtrgOrder->orderStatus = Types::OrderStatus::failed;
                    break;


                case TAPI_ORDER_STATE_FINISHED:
                    abtrgOrder->lastFilledVolume[0] = info->OrderInfo->OrderMatchQty - abtrgOrder->filledVolume[0];
                    abtrgOrder->lastFilledVolume[1] =  abtrgOrder->lastFilledVolume[0];//info->OrderInfo->OrderMatchQty2 - abtrgOrder->filledVolume[1];
                    abtrgOrder->filledVolume[0]= info->OrderInfo->OrderMatchQty;
                    abtrgOrder->filledVolume[1]=  abtrgOrder->filledVolume[0];//info->OrderInfo->OrderMatchQty2;
                    abtrgOrder->lastFilledPrice[0] = info->OrderInfo->OrderMatchPrice;
                    abtrgOrder->lastFilledPrice[1] = info->OrderInfo->OrderMatchPrice2;
                    abtrgOrder->orderStatus = Types::OrderStatus::allTraded;
                    break;

                case TAPI_ORDER_STATE_PARTFINISHED  :
                    abtrgOrder->lastFilledVolume[0] = info->OrderInfo->OrderMatchQty - abtrgOrder->filledVolume[0];
                    abtrgOrder->lastFilledVolume[1] = abtrgOrder->lastFilledVolume[0];//info->OrderInfo->OrderMatchQty2 - abtrgOrder->filledVolume[1];
                    abtrgOrder->filledVolume[0]= info->OrderInfo->OrderMatchQty;
                    abtrgOrder->filledVolume[1]=  abtrgOrder->filledVolume[0];//info->OrderInfo->OrderMatchQty2;
                    abtrgOrder->lastFilledPrice[0] = info->OrderInfo->OrderMatchPrice;
                    abtrgOrder->lastFilledPrice[1] = info->OrderInfo->OrderMatchPrice2;
                    abtrgOrder->orderStatus =  Types::OrderStatus::partTraded;
                    break;
                case  TAPI_ORDER_STATE_CANCELED   :
                    abtrgOrder->orderStatus = Types::OrderStatus::canceled;
                    break;
		case  TAPI_ORDER_STATE_LEFTDELETED   :
                    abtrgOrder->orderStatus = Types::OrderStatus::canceled;
                    break;   
                default:
                    abtrgOrder->orderStatus = Types::OrderStatus::unknown;
                    break;
            }
        }


        char EESTrader::getOrderPositionEffect(Types::PositionEffectType pet) {

            switch (pet) {
                case Types::PositionEffectType::T_close:
                    return TAPI_PositionEffect_COVER_TODAY;
                case Types::PositionEffectType::Y_close:
                    return TAPI_PositionEffect_COVER;
                case Types::PositionEffectType::open:
                    return TAPI_PositionEffect_OPEN;
                default:
                    return TAPI_PositionEffect_OPEN;
            }

        }
    }
}
