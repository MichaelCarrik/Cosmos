//
// Created by zhangyw on 3/5/21.
//

#ifndef TrendFollow_CTPINTERPRETER_H
#define TrendFollow_CTPINTERPRETER_H

#include "../Types/CtpTradeConnection.h"

namespace TrendFollow {
    namespace Trader {
        class CtpInterpreter{
             Utils::MemoryList<CThostFtdcInputOrderField,  Types::OrderBuffSize> m_ctpOrderList{0};
             Utils::MemoryList<CThostFtdcInputOrderActionField,  Types::OrderBuffSize> m_ctpOrderActionList{0};
             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize>* m_orderList{nullptr};

             Utils::MemoryList< Types::ArbitrageOrderField,  Types::OrderBuffSize>* m_abtrgOrderList{nullptr};

             Types::CtpTradeConnection m_ctpConnection;
            CThostFtdcTraderApi *m_pTradeApi;
            int m_frontID{0};
            int m_sessionID{0};
        //    folly::SpinLock m_sendOrderLock;
            boost::detail::spinlock  m_sendOrderLock;

            char getOrderSide( Types::OrderSide orderSide){
                switch (orderSide)
                {
                    case  Types::OrderSide::buy:
                        return THOST_FTDC_D_Buy;
                        case  Types::OrderSide::sell:
                            return THOST_FTDC_D_Sell;
                }
                return ',';
            }

            char getOrderPositionEffect( Types::PositionEffectType pet){
                switch (pet)
                {
                    case  Types::PositionEffectType::T_close:
                        return THOST_FTDC_OF_CloseToday ;
                        case  Types::PositionEffectType::Y_close:
                            return THOST_FTDC_OF_CloseYesterday ;
                            case  Types::PositionEffectType::open:
                                return THOST_FTDC_OF_Open;
                }
                return ',';
            }

            void updateOrder_with(const CThostFtdcOrderField * info,  Types::OrderField *pOrder)
            {

                switch (info->OrderStatus)
                {
                    case THOST_FTDC_OSS_Accepted:
                        pOrder->orderStatus =  Types::OrderStatus::accept;
                        break;
                        case THOST_FTDC_OST_AllTraded:
                            pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
                            pOrder->filledVolume=  info->VolumeTraded;
                            pOrder->lastFilledPrice =  info->LimitPrice;
                            pOrder->orderStatus =  Types::OrderStatus::allTraded;
                            break;

                            case THOST_FTDC_OST_PartTradedNotQueueing:
                                case THOST_FTDC_OST_PartTradedQueueing  :

                                    pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
                                    pOrder->filledVolume=  info->VolumeTraded;
                                    pOrder->lastFilledPrice =  info->LimitPrice;
                                    pOrder->orderStatus =  Types::OrderStatus::partTraded;



                                    break;
                                    case  THOST_FTDC_OST_Canceled   :
                                        pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
                                        pOrder->filledVolume=  info->VolumeTraded;
                                        pOrder->lastFilledPrice =  info->LimitPrice;
                                        pOrder->orderStatus =  Types::OrderStatus::canceled;
                                        break;
                                        default:

                                            pOrder->orderStatus =  Types::OrderStatus::unknown;
                                            break;
                }
            }

            void updateAbtrgOrder_with(const CThostFtdcOrderField * info,  Types::ArbitrageOrderField *abtrgOrder)
            {

                switch (info->OrderStatus)
                {
                    case THOST_FTDC_OSS_Accepted:
                        abtrgOrder->orderStatus =  Types::OrderStatus::accept;
                        break;
//                        case THOST_FTDC_OST_AllTraded:
//                            pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
//                            pOrder->filledVolume=  info->VolumeTraded;
//                            pOrder->lastFilledPrice =  info->LimitPrice;
//                            pOrder->orderStatus =  Types::OrderStatus::allTraded;
//                            break;

//                            case THOST_FTDC_OST_PartTradedNotQueueing:
//                                case THOST_FTDC_OST_PartTradedQueueing  :
//
//                                    pOrder->lastFilledVolume = info->VolumeTraded - pOrder->filledVolume;
//                                    pOrder->filledVolume=  info->VolumeTraded;
//                                    pOrder->lastFilledPrice =  info->LimitPrice;
//                                    pOrder->orderStatus =  Types::OrderStatus::partTraded;
//
//
//
//                                    break;
                        case  THOST_FTDC_OST_Canceled   :
                            abtrgOrder->orderStatus =  Types::OrderStatus::canceled;
                            break;
                        default:

                            abtrgOrder->orderStatus =  Types::OrderStatus::unknown;
                            break;
                }
            }

            void updateAbtrgTrade_with(const CThostFtdcTradeField *pTrade,  Types::ArbitrageOrderField *abtrgOrder)
            {
                if(strcmp(pTrade->InstrumentID, abtrgOrder->instrumentIDs[0].data())==0){

                    abtrgOrder->lastFilledVolume[0] = pTrade->Volume ;
                    abtrgOrder->filledVolume[0] +=  pTrade->Volume;
                    abtrgOrder->lastFilledPrice[0] =  pTrade->Price;
                    abtrgOrder->orderStatus =  Types::OrderStatus::partTraded;
                    abtrgOrder->lastFilledVolume[1] = 0;

                }else if(strcmp(pTrade->InstrumentID, abtrgOrder->instrumentIDs[1].data())==0){
                    abtrgOrder->lastFilledVolume[1] = pTrade->Volume  ;
                    abtrgOrder->filledVolume[1] +=  pTrade->Volume;
                    abtrgOrder->lastFilledPrice[1] =  pTrade->Price;
                    abtrgOrder->orderStatus =  Types::OrderStatus::partTraded;
                    abtrgOrder->lastFilledVolume[0] = 0;
                }else {
                    fprintf(stderr, "%s,%s,%s\n", pTrade->InstrumentID, abtrgOrder->instrumentIDs[0].data(),  abtrgOrder->instrumentIDs[1].data());
                    assert(false);
                }

                if ( (abtrgOrder->filledVolume[0] == abtrgOrder->filledVolume[1]) && (abtrgOrder->filledVolume[0] == abtrgOrder->orderVolume)){
                    abtrgOrder->orderStatus =  Types::OrderStatus::allTraded;
                }
            }

        public:

            CtpInterpreter(   CThostFtdcTraderApi * api,     Types::CtpTradeConnection & inputCTC , int frontID, int sessionID, int maxOrderRef)
            :m_pTradeApi(api), m_ctpConnection(inputCTC), m_frontID(frontID), m_sessionID(sessionID){
                m_orderList = new Utils::MemoryList<Types::OrderField ,Types::OrderBuffSize> (maxOrderRef);
                m_abtrgOrderList= new Utils::MemoryList<Types::ArbitrageOrderField ,Types::OrderBuffSize> (maxOrderRef);
            }

             Types::OrderField * sendOrder( Types::OrderField const&input_orderField, int & nRetCode){

                auto pCtpOrder = m_ctpOrderList.getNewMemory();

                strcpy(pCtpOrder->BrokerID, m_ctpConnection.brokerId.c_str());
                strcpy(pCtpOrder->InvestorID, m_ctpConnection.username.c_str());
                strcpy(pCtpOrder->InstrumentID, input_orderField.instrumentID.data());
                pCtpOrder->Direction = getOrderSide(input_orderField.orderSide);
                pCtpOrder->CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

                if(input_orderField.hedgeType == Types::HedgeType::hedge){
                    pCtpOrder->CombHedgeFlag[0] = THOST_FTDC_HF_Hedge;
                }

                pCtpOrder->VolumeTotalOriginal = input_orderField.orderVolume;
                pCtpOrder->VolumeCondition = THOST_FTDC_VC_AV;
                if(  pCtpOrder->VolumeTotalOriginal >100){
                    assert(false);
                }

                pCtpOrder->TimeCondition = THOST_FTDC_TC_GFD;
                if(input_orderField.orderTimeType ==  Types::OrderTimeType::IOC){
                    pCtpOrder->TimeCondition = THOST_FTDC_TC_IOC;
                }

                pCtpOrder->OrderPriceType = THOST_FTDC_OPT_LimitPrice;
                pCtpOrder->LimitPrice = round(input_orderField.orderPrice*10000)/10000.0;


                pCtpOrder->ContingentCondition = THOST_FTDC_CC_Immediately;
                pCtpOrder->ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
                strcpy(pCtpOrder->BrokerID, m_ctpConnection.brokerId.c_str());
                strcpy(pCtpOrder->UserID, m_ctpConnection.username.c_str());
                strcpy(pCtpOrder->InvestorID, m_ctpConnection.username.c_str());
                pCtpOrder->CombOffsetFlag[0] = getOrderPositionEffect(input_orderField.pet);

                m_sendOrderLock.lock();
                int assignId=0;
                auto orderField = m_orderList->getNewMemory(assignId);
                memcpy(orderField, &input_orderField, sizeof( Types::OrderField ));
                sprintf(pCtpOrder->OrderRef,"%s%09d", m_ctpConnection.OrderPrefix.data(), assignId);
                pCtpOrder->RequestID = assignId;
                orderField->tOrderID = assignId;
                fprintf(stderr,"sendOrder : instrument=%s, requestID=%d orderRef=%s, orderPrice=%.3f, orderVolume=%d\n",
                        orderField->instrumentID.data(), orderField->tOrderID, pCtpOrder->OrderRef, pCtpOrder->LimitPrice,
                        pCtpOrder->VolumeTotalOriginal);
                nRetCode = m_pTradeApi->ReqOrderInsert(pCtpOrder, pCtpOrder->RequestID);
                m_sendOrderLock.unlock();
                if (nRetCode != 0) {
                    orderField->orderStatus =  Types::OrderStatus::failed;
                }
                return orderField;
            };

             Types::ArbitrageOrderField * sendAbtrgOrder( Types::ArbitrageOrderField const&inputAbtragOrderField, int & nRetCode){

                auto pCtpOrder = m_ctpOrderList.getNewMemory();

                strcpy(pCtpOrder->BrokerID, m_ctpConnection.brokerId.c_str());
                strcpy(pCtpOrder->InvestorID, m_ctpConnection.username.c_str());
                sprintf(pCtpOrder->InstrumentID, "%s %s&%s", inputAbtragOrderField.orderPrex.data(), inputAbtragOrderField.instrumentIDs[0].data(),
                        inputAbtragOrderField.instrumentIDs[1].data());
                pCtpOrder->Direction = getOrderSide(inputAbtragOrderField.orderSides[0]);
                pCtpOrder->CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

                pCtpOrder->VolumeTotalOriginal = inputAbtragOrderField.orderVolume;
                pCtpOrder->VolumeCondition = THOST_FTDC_VC_AV;

                pCtpOrder->TimeCondition = THOST_FTDC_TC_GFD;
                if(inputAbtragOrderField.orderTimeType ==  Types::OrderTimeType::IOC){
                    pCtpOrder->TimeCondition = THOST_FTDC_TC_IOC;
                }

                pCtpOrder->OrderPriceType = THOST_FTDC_OPT_LimitPrice;
                pCtpOrder->LimitPrice = round(inputAbtragOrderField.orderPrice*10000)/10000.0;


                pCtpOrder->ContingentCondition = THOST_FTDC_CC_Immediately;
                pCtpOrder->ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
                strcpy(pCtpOrder->BrokerID, m_ctpConnection.brokerId.c_str());
                strcpy(pCtpOrder->UserID, m_ctpConnection.username.c_str());
                strcpy(pCtpOrder->InvestorID, m_ctpConnection.username.c_str());
                pCtpOrder->CombOffsetFlag[0] = getOrderPositionEffect(inputAbtragOrderField.pets[0]);
                pCtpOrder->CombOffsetFlag[1] = getOrderPositionEffect(inputAbtragOrderField.pets[1]);
                if(pCtpOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open && pCtpOrder->CombOffsetFlag[1] != THOST_FTDC_OF_Open){
                    pCtpOrder->IsSwapOrder = true;
                }else if(pCtpOrder->CombOffsetFlag[0] != THOST_FTDC_OF_Open && pCtpOrder->CombOffsetFlag[1] == THOST_FTDC_OF_Open){
                    pCtpOrder->IsSwapOrder = true;
                }else{
                    pCtpOrder->IsSwapOrder = false;
                }


                m_sendOrderLock.lock();
                int assignId=0;
                auto abtrgOrderField = m_abtrgOrderList->getNewMemory(assignId);
                memcpy(abtrgOrderField, &inputAbtragOrderField, sizeof( Types::ArbitrageOrderField ));
                sprintf(pCtpOrder->OrderRef,"%s%09d", m_ctpConnection.OrderPrefix.data(), assignId);
                pCtpOrder->RequestID = assignId;
                abtrgOrderField->tOrderID = assignId;
           //     fprintf(stderr,"sendAbtrgOrder : instrument=%s, requestID=%d orderRef=%s\n", pCtpOrder->InstrumentID, abtrgOrderField->tOrderID, pCtpOrder->OrderRef);
            //    nRetCode = m_pTradeApi->ReqOrderInsert(pCtpOrder, pCtpOrder->RequestID);
                m_sendOrderLock.unlock();
                if (nRetCode != 0) {
                    abtrgOrderField->orderStatus =  Types::OrderStatus::failed;
                }
                return abtrgOrderField;
            };

            void cancelOrder( Types::OrderField const & inputOrder, int64_t& epoch_time) {
                auto order = m_orderList->getMemoryById(inputOrder.tOrderID);
                if (order != nullptr) {
                    int assignID;
                    auto field = m_ctpOrderActionList.getNewMemory(assignID);

                    strcpy(field->InstrumentID, inputOrder.instrumentID.data());
                    field->RequestID = inputOrder.tOrderID;
                    field->FrontID = m_frontID;
                    field->SessionID = m_sessionID;
                    strcpy(field->OrderRef, order->orderRef.data());
                    strcpy(field->OrderSysID, order->orderSysID.data());
                    field->ActionFlag = THOST_FTDC_AF_Delete;
                    strcpy(field->BrokerID, m_ctpConnection.brokerId.c_str());
                    strcpy(field->UserID, m_ctpConnection.username.c_str());
                    epoch_time= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    int ret = m_pTradeApi->ReqOrderAction(field, 0);
                    //                    spdlog::debug(
                    //                            "cancelOrder : frontid= {}, sessionid={}, OrderSysId={}, iRuquest={},  orderRef={}, ret={}, traderOrderID={}, policyOrderID={}",
                    //                            field->FrontID, field->SessionID, field->OrderSysID, field->RequestID, field->OrderRef,
                    //                            ret, inputOrder.tradeOrderID, inputOrder.policyOrderID);
                }
            }

            void cancelAbtrgOrder( Types::ArbitrageOrderField const & inputAbtrgOrder, int64_t& epoch_time) {
                auto abtrgOrder = m_abtrgOrderList->getMemoryById(inputAbtrgOrder.tOrderID);
                if (abtrgOrder != nullptr) {
                    int assignID;
                    auto field = m_ctpOrderActionList.getNewMemory(assignID);
                    sprintf(field->InstrumentID, "%s %s&%s", abtrgOrder->orderPrex.data(), abtrgOrder->instrumentIDs[0].data(),
                            abtrgOrder->instrumentIDs[1].data());
                    field->RequestID = abtrgOrder->tOrderID;
                    field->FrontID = m_frontID;
                    field->SessionID = m_sessionID;
                    strcpy(field->OrderRef, abtrgOrder->orderRef.data());
                    strcpy(field->OrderSysID, abtrgOrder->orderSysID.data());
                    field->ActionFlag = THOST_FTDC_AF_Delete;
                    strcpy(field->BrokerID, m_ctpConnection.brokerId.c_str());
                    strcpy(field->UserID, m_ctpConnection.username.c_str());
                    epoch_time= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    int ret = m_pTradeApi->ReqOrderAction(field, 0);
                    //                    spdlog::debug(
                    //                            "cancelOrder : frontid= {}, sessionid={}, OrderSysId={}, iRuquest={},  orderRef={}, ret={}, traderOrderID={}, policyOrderID={}",
                    //                            field->FrontID, field->SessionID, field->OrderSysID, field->RequestID, field->OrderRef,
                    //                            ret, inputOrder.tradeOrderID,
                }
            }
             Types::OrderField * OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
            {
                if( pRspInfo!= nullptr && pRspInfo->ErrorID !=0)
                {
                    spdlog::error("OnErrRtnOrderInsert :  error {} {}",   pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                    fprintf(stderr, "OnErrRtnOrderInsert :  error %d %s\n",   pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                }

                if (pInputOrder !=nullptr){
                    auto orderid= atol(pInputOrder->OrderRef) % 1000000;
                    spdlog::error("OnErrRtnOrderInsert : RequestID={}, OrderRef={}, InstrumentID={}, Direction={}",pInputOrder->RequestID, pInputOrder->OrderRef, pInputOrder->InstrumentID,
                                  pInputOrder->Direction);
                    fprintf(stderr, "OnErrRtnOrderInsert : RequestID=%d, OrderRef=%s, InstrumentID=%s, Direction=%c, CombOffsetFlag=%c\n",pInputOrder->RequestID, pInputOrder->OrderRef, pInputOrder->InstrumentID,
                                  pInputOrder->Direction, pInputOrder->CombOffsetFlag[0]);
                    auto order = m_orderList->getMemoryById(orderid);
                    if(order == nullptr){
                        return nullptr;
                    }
                    order->orderStatus =  Types::OrderStatus::failed;
                    return order;
                }
                return nullptr;
            };

             Types::ArbitrageOrderField * OnErrRtnAbtrgOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
            {
                if( pRspInfo!= nullptr && pRspInfo->ErrorID !=0)
                {

                    spdlog::error("OnErrRtnOrderInsert :  error {} {}",   pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                }

                if (pInputOrder !=nullptr){
                    auto orderid= atol(pInputOrder->OrderRef) % 1000000;

                    spdlog::error("OnErrRtnOrderInsert : RequestID={}, OrderRef={}",pInputOrder->RequestID, pInputOrder->OrderRef);
                    auto order = m_abtrgOrderList->getMemoryById(orderid);
                    order->orderStatus =  Types::OrderStatus::failed;
                    return order;
                }
                return nullptr;
            };

             Types::OrderField *  OnRtnOrder(CThostFtdcOrderField *ctpOrder){

                if (ctpOrder != nullptr && ctpOrder->FrontID ==m_frontID && ctpOrder->SessionID == m_sessionID && ctpOrder->OrderStatus != THOST_FTDC_OST_Unknown) {
                    auto orderid = atol(ctpOrder->OrderRef) % 1000000;
                    auto order = m_orderList->getMemoryById(orderid);

                    if (order != nullptr) {
                        assert(ctpOrder->RequestID == order->tOrderID);
                        strcpy(order->orderSysID.data(), ctpOrder->OrderSysID);
                        strcpy(order->orderRef.data(), ctpOrder->OrderRef);
                        updateOrder_with(ctpOrder, order);
                        return order;
                    }
                }
                return nullptr;
            }

             Types::ArbitrageOrderField *  OnRtnAbtrgOrder(CThostFtdcOrderField *ctpOrder){
                if (ctpOrder != nullptr && ctpOrder->FrontID ==m_frontID && ctpOrder->SessionID == m_sessionID && ctpOrder->OrderStatus != THOST_FTDC_OST_Unknown) {
//                    fprintf(stderr,"OnRtnAbtrgOrder : instrument=%s, requestID=%d orderRef=%s, OrderStatus=%c\n", ctpOrder->InstrumentID, ctpOrder->RequestID,  ctpOrder->OrderRef,
//                            ctpOrder->OrderStatus);
                    auto orderid = atol(ctpOrder->OrderRef) % 1000000;
                    auto abtrgOrder = m_abtrgOrderList->getMemoryById(orderid);
                    if (abtrgOrder != nullptr) {
                        assert(ctpOrder->RequestID == abtrgOrder->tOrderID);
                        strcpy(abtrgOrder->orderSysID.data(), ctpOrder->OrderSysID);
                        strcpy(abtrgOrder->orderRef.data(), ctpOrder->OrderRef);
                        updateAbtrgOrder_with(ctpOrder, abtrgOrder);
                        return abtrgOrder;
                    }
                }
                return nullptr;
            }

             Types::ArbitrageOrderField *  OnRtnAbtrgTrade (CThostFtdcTradeField *pTrade){
           //     fprintf(stderr,"OnRtnAbtrgTrade : instrument=%s, orderRef=%s, \n", pTrade->InstrumentID, pTrade->OrderRef);
                auto orderid = atol(pTrade->OrderRef) % 1000000;
                auto abtrgOrder = m_abtrgOrderList->getMemoryById(orderid);
                if (abtrgOrder != nullptr) {
                 //   assert(ctpOrder->RequestID == abtrgOrder->tOrderID);
                 //   strcpy(abtrgOrder->orderSysID.data(), ctpOrder->OrderSysID);
                //    strcpy(abtrgOrder->orderRef.data(), ctpOrder->OrderRef);
                    updateAbtrgTrade_with(pTrade, abtrgOrder);
                    return abtrgOrder;
                }

                return nullptr;
            }
        };
    }
}

#endif