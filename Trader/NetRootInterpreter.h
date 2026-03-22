//
// Created by zhangyw on 3/8/21.
//

#ifndef Cosmos_NETROOTINTERPRETER_H
#define Cosmos_NETROOTINTERPRETER_H

#include <RootNetClientApi.h>
#include "Config.h"
#include "OrderCallBackHandler.h"



namespace Cosmos {
    namespace Trader {
        struct NetRootConnect{
            std::string accountID{""};
	    std::string accountPassword{""};
            std::string userid{""};
            std::string password{""};
            std::string frontHost{""};
            int port{0};
        };
        class NetRootInterpreter {
            folly::SpinLock m_sendOrderLock;
            std::future<long> m_sendOrderFuture;
            std::promise<long> m_sendOrdePromise;
            int m_sendOrderID{-1};

            std::unordered_map<long, int> m_orderRefToOrderIdMap;
            std::unordered_map<int, std::string> m_orderIdToContractNumbMap;
            NetRootConnect m_netRootConnect;
            OrderCallBackHandler m_orderCallBackHandler;

            RootNetClientApi * m_api{nullptr};
             Utils::MemoryList<OrderCancelInfo,  Types::OrderBuffSize> m_netRootOrderActionList{
                    0};
             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize> *m_orderList{
                    nullptr};

          //  DemoOrderCallBackHandler m_callBackOrderHandler;
          //  DemoAccountCallBackHandler m_callBackAccountHandler;

            BSFlag getOrderSide( Types::OrderSide orderSide) {
                switch (orderSide) {
                    case  Types::OrderSide::buy:
                        return BSFlag::Buy;
                    case  Types::OrderSide::sell:
                        return BSFlag::Sell;
                }
                return BSFlag::BSFlagNotSet;
            }

            OffSetFlag getOrderPositionEffect( Types::PositionEffectType pet,  Types::ExchangeType exchangeID) {
                if(exchangeID == Types::ExchangeType::CFFEX || exchangeID ==  Types::ExchangeType::DCE ||
		   exchangeID ==  Types::ExchangeType::ZCE ){
                    switch (pet) {
                        case  Types::PositionEffectType::T_close:
                            return OffSetFlag::CLOSE;
                        case  Types::PositionEffectType::Y_close:
                            return OffSetFlag::CLOSE;
                        case  Types::PositionEffectType::open:
                            return OffSetFlag::OPEN;
                    }
                    return OffSetFlag::CLOSE;
                }else{
                    switch (pet) {
                        case  Types::PositionEffectType::T_close:
                            return OffSetFlag::CLOSETD;
                        case  Types::PositionEffectType::Y_close:
                            return OffSetFlag::CLOSEYD;
                        case  Types::PositionEffectType::open:
                            return OffSetFlag::OPEN;
                    }
                    return OffSetFlag::CLOSEYD;
                }

            }

            ExchID getExchangeID(  Types::ExchangeType exchangeID){
                switch (exchangeID) {
                    case  Types::ExchangeType::CFFEX :
                        return ExchID::CFFEX;
                    case  Types::ExchangeType::SHFE :
                        return ExchID::SHFE;
                    case  Types::ExchangeType::DCE :
                        return ExchID::DCE;
                    case  Types::ExchangeType::ZCE :
                        return ExchID::CZCE;
                    case  Types::ExchangeType::INE :
                        return ExchID::SHIEE;

                }
                return  ExchID::ExchIDNotSet;

            }

            void updateOrder_with(const CThostFtdcOrderField *info,  Types::OrderField *pOrder) {

                switch (info->OrderStatus) {
                    case THOST_FTDC_OSS_Accepted:
                        pOrder->orderStatus =  Types::OrderStatus::accept;
                        break;
                    case THOST_FTDC_OST_AllTraded:
                        pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
                        pOrder->filledVolume = info->VolumeTraded;
                        pOrder->lastFilledPrice = info->LimitPrice;
                        pOrder->orderStatus =  Types::OrderStatus::allTraded;
                        break;

                    case THOST_FTDC_OST_PartTradedNotQueueing:
                    case THOST_FTDC_OST_PartTradedQueueing  :

                        pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
                        pOrder->filledVolume = info->VolumeTraded;
                        pOrder->lastFilledPrice = info->LimitPrice;
                        pOrder->orderStatus =  Types::OrderStatus::partTraded;


                        break;
                    case THOST_FTDC_OST_Canceled   :
                        pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
                        pOrder->filledVolume = info->VolumeTraded;
                        pOrder->lastFilledPrice = info->LimitPrice;
                        pOrder->orderStatus =  Types::OrderStatus::canceled;
                        break;
                    default:

                        pOrder->orderStatus =  Types::OrderStatus::unknown;
                        break;
                }
            }

            void updateMap(long orderRef) {
                auto itr = m_orderRefToOrderIdMap.find(orderRef);
                if (itr == m_orderRefToOrderIdMap.end()) {
                    if (m_sendOrderFuture.valid() == true && m_sendOrderID >= 0) {
                        m_orderRefToOrderIdMap[orderRef] = m_sendOrderID;
                        m_sendOrderID = -1;
                        m_sendOrdePromise.set_value(orderRef);
                    }
                }
            }


        public:
            NetRootInterpreter(CThostFtdcTraderApi * api, std::string & interpreterConfig , int maxOrderRef){

            //    m_callBackOrderHandler.m_orderIdToContractNumbMap = &m_orderIdToContractNumbMap;
                boost::property_tree::ptree pt;
                boost::property_tree::read_xml(interpreterConfig, pt);
                m_netRootConnect.accountID = pt.get_child("ThostUser2.ConnectConfig.AccountID").get<std::string>("<xmlattr>.value");
                m_netRootConnect.userid = pt.get_child("ThostUser2.ConnectConfig.Username").get<std::string>("<xmlattr>.value");
                m_netRootConnect.password = pt.get_child("ThostUser2.ConnectConfig.Password").get<std::string>("<xmlattr>.value");
                m_netRootConnect.frontHost = pt.get_child("ThostUser2.ConnectConfig.TradeFront").get<std::string>("<xmlattr>.value");
                m_netRootConnect.port = pt.get_child("ThostUser2.ConnectConfig.Port").get<int>("<xmlattr>.value");
	        m_netRootConnect.accountPassword = pt.get_child("ThostUser2.ConnectConfig.AccountPassword").get<std::string>("<xmlattr>.value");


                m_orderList = new Utils::MemoryList<Types::OrderField ,Types::OrderBuffSize> (maxOrderRef);
                m_api = new RootNetClientApi();
                MsgConnectInfo connInfo;
                std::string tempHost = m_netRootConnect.frontHost;
                auto ret = m_api->Connect(connInfo, tempHost, m_netRootConnect.port );
                if (ret != 0)
                {
                    fprintf(stderr, "NetRootInterpreter m_api->Connect failed : %s, host=%s, port=%d\n", m_api->GetLastError().c_str(), tempHost.c_str(), m_netRootConnect.port);
                    spdlog::error("NetRootInterpreter !m_api->Connect failed : {}", m_api->GetLastError().c_str());
                    assert(false);
		    //return;
                }

                ret = m_api->OptLogin(m_netRootConnect.userid ,   m_netRootConnect.password );
                if (ret != 0)
                {
                    fprintf(stderr, "NetRootInterpreter login failed : %s, userid=%s, password=%s\n", m_api->GetLastError().c_str(), m_netRootConnect.userid.c_str(),  m_netRootConnect.password.c_str());
                    spdlog::error("NetRootInterpreter login failed:{}", m_api->GetLastError().c_str());
                    assert(false);
		    //  return;
                }


                MsgAccountLogin acctLoginInfo;
                ret = m_api->AccountLogin(m_netRootConnect.accountID,  m_netRootConnect.accountPassword , acctLoginInfo);
                if (ret != 0)
                {
                    fprintf(stderr, "NetRootInterpreter money failed : %s, accountID=%s, accountPassword=%s\n", m_api->GetLastError().c_str(), m_netRootConnect.accountID.c_str(), m_netRootConnect.accountPassword.c_str());
                    spdlog::error(" NetRootInterpreter !money failed:{}", m_api->GetLastError().c_str());
                    assert(false);
		    //     return;
                }
                m_api->RegisterTradeSpi(&m_orderCallBackHandler);

            }
             Types::OrderField *
            sendOrder( Types::OrderField const &input_orderField, int &nRetCode) {

                m_sendOrderLock.lock();
                OrderNewInfo m_netRootOrder;
//		memset(&m_netRootOrder, 0, sizeof(OrderNewInfo));
                m_netRootOrder.acctId = m_netRootConnect.accountID;
                m_netRootOrder.exchId = getExchangeID(input_orderField.exchangeId);
                m_netRootOrder.stkId = input_orderField.instrumentID.data();
                std::string rmb("00");
                m_netRootOrder.currencyId = rmb;
                //       pNROrder->orderType = OrderType::Buy;
                m_netRootOrder.f_offSetFlag = getOrderPositionEffect(input_orderField.pet, input_orderField.exchangeId);
                m_netRootOrder.bsFlag = getOrderSide(input_orderField.orderSide);
                m_netRootOrder.businessMark = BusinessMark::OTO;
                m_netRootOrder.f_orderPriceType = OrderPriceType::LIMIT;
                m_netRootOrder.orderPrice = input_orderField.orderPrice;
                m_netRootOrder.orderQty = input_orderField.orderVolume;


                int assignId = 0;
                auto orderField = m_orderList->getNewMemory(assignId);
                memcpy(orderField, &input_orderField, sizeof( Types::OrderField));
           //     m_netRootOrder.contractNum = assignId;
                m_netRootOrder.batchNum = assignId;
                m_netRootOrder.clientId = std::to_string(assignId);
                orderField->tOrderID = assignId;
                fprintf(stderr,"sendOrder : instrument=%s, tOrderID=%d \n", orderField->instrumentID.data(), orderField->tOrderID);
                MsgOrderNew msgOrdernew;


                std::promise<long> tempPromise;
                std::swap(m_sendOrdePromise, tempPromise);
                m_sendOrderFuture = m_sendOrdePromise.get_future();
                m_sendOrderID = orderField->tOrderID;
                nRetCode = m_api->SyncOrderNew(m_netRootOrder, msgOrdernew);
                if (nRetCode == 0)
                {
                    auto status = m_sendOrderFuture.wait_for(std::chrono::seconds(30));
                    if (status == std::future_status::deferred) {
                        fprintf(stderr,
                                "netRoot sendOrder deferred, instrument=%s, orderPrice=%f, orderVolume=%d, orderSide=%s\n",
                                orderField->instrumentID.data(), orderField->orderPrice, orderField->orderVolume,
                                 Types::orderSideMap[orderField->orderSide].data());
                        nRetCode =-1;
                        assert(false && "netRoot sendOrder deferred,");
                    } else if (status == std::future_status::timeout) {
                        fprintf(stderr,
                                "netRoot sendOrder timeout, instrument=%s, orderPrice=%f, orderVolume=%d, orderSide=%s\n",
                                orderField->instrumentID.data(), orderField->orderPrice, orderField->orderVolume,
                                 Types::orderSideMap[orderField->orderSide].data());
                        nRetCode =-1;
                        assert(false && "netRoot sendOrder timeout");
                    } else if (status == std::future_status::ready) {
                        auto orderRef = m_sendOrderFuture.get();
                        m_orderIdToContractNumbMap[ orderField->tOrderID] =  msgOrdernew.contractNum;


                    }
                    m_sendOrderLock.unlock();
                }else if(nRetCode != 0){
                    fprintf(stderr,"netRoot sendOrder failed : %s\n",m_api->GetLastError().c_str());
                    nRetCode =-1;
		    sleep(1);
                    assert(false && "netRoot sendOrder failed");
                }


                if (nRetCode != 0) {
                    orderField->orderStatus =  Types::OrderStatus::failed;
                }
                return orderField;
            };

            void cancelOrder( Types::OrderField const &inputOrder, int64_t &epoch_time) {
                fprintf(stderr, "cancelOrder  instrument=%s, tOrderID=%d\n", inputOrder.instrumentID.data(), inputOrder.tOrderID);
                auto order = m_orderList->getMemoryById(inputOrder.tOrderID);

                auto itr = m_orderIdToContractNumbMap.find(order->tOrderID);
                if (itr != m_orderIdToContractNumbMap.end()) {
                    auto orderCancel = m_netRootOrderActionList.getNewMemory();
                    orderCancel->acctId = m_netRootConnect.accountID;
                    orderCancel->exchId = getExchangeID(inputOrder.exchangeId);
                    orderCancel->contractNum = itr->second;

                    auto ret = m_api->OrderCancel(*orderCancel);
                    if (ret != 0) {
                        fprintf(stderr, "netRoot cancelOrder failed : acctId=%s, exchId=%c, contractNum=%s, retCode=%d, errormsg=%s\n",
                                orderCancel->acctId .c_str(), orderCancel->exchId, orderCancel->contractNum.c_str(), ret, m_api->GetLastError().c_str());

                    }
                }


//                    spdlog::debug(
//                            "cancelOrder : frontid= {}, sessionid={}, OrderSysId={}, iRuquest={},  orderRef={}, ret={}, traderOrderID={}, policyOrderID={}",
//                            field->FrontID, field->SessionID, field->OrderSysID, field->RequestID, field->OrderRef,
//                            ret, inputOrder.tradeOrderID, inputOrder.policyOrderID);



            }

             Types::OrderField * OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {
                if (pRspInfo != nullptr && pRspInfo->ErrorID != 0) {

                    spdlog::error("OnErrRtnOrderInsert :  error {} {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                }

                if (pInputOrder != nullptr) {
                    auto orderRef = atol(pInputOrder->OrderRef) % 1000000;

                    updateMap(orderRef);

                    spdlog::error("OnErrRtnOrderInsert : RequestID={}, OrderRef={}", pInputOrder->RequestID,
                                  pInputOrder->OrderRef);

                    auto itr = m_orderRefToOrderIdMap.find(orderRef);
                    if (itr != m_orderRefToOrderIdMap.end()) {
                        auto order = m_orderList->getMemoryById(itr->second);

                        order->orderStatus =  Types::OrderStatus::failed;
                        return order;
                    }
                }
                return nullptr;
            };

             Types::OrderField *OnRtnOrder(const CThostFtdcOrderField *ctpOrder) {

                if (ctpOrder != nullptr && ctpOrder->OrderStatus != THOST_FTDC_OST_Unknown) {
                    auto orderRef = atol(ctpOrder->OrderRef);

                    updateMap(orderRef);
                //    fprintf(stderr,"OnRtnOrder : instrument=%s, OrderRef=%f \n", ctpOrder->InstrumentID, ctpOrder->OrderRef);
                    auto itr = m_orderRefToOrderIdMap.find(orderRef);
                    if (itr != m_orderRefToOrderIdMap.end()) {
                        auto order = m_orderList->getMemoryById(itr->second);
                    //    assert(ctpOrder->RequestID == order->tOrderID);
                        strcpy(order->orderSysID.data(), ctpOrder->OrderSysID);
                        strcpy(order->orderRef.data(), ctpOrder->OrderRef);
                        updateOrder_with(ctpOrder, order);
                        return order;
                    }
                }
                return nullptr;
            }
        };
    }
}


#endif //Cosmos_NETROOTINTERPRETER_H
