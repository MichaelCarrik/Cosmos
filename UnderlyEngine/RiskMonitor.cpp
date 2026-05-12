//
// Created by zhangyingwei on 2026/4/2.
//

#include "RiskMonitor.h"


namespace Cosmos {
    namespace Engine {


        void RiskMonitor::onOrderField(const Types::OrderField *inputOrder, Types::Symbol *symbol) {
            if (inputOrder->policyID != m_policyID) {
                assert(false && "RiskMonitor::onOrderField policyID not match");
            }
            fprintf(stderr, "RiskMonitor::onEventData instrument=%s, updateTime=%s, %03d, pOrderID=%d, requestID=%d, orderRef=%s, orderSide=%s, "
                                  "orderPrice=%.3f, orderVolume=%d, orderStatus=%s\n",
                          inputOrder->instrumentID.data(), symbol->lastMD->updateTime.data(), symbol->lastMD->milliSeconds, inputOrder->pOrderID, inputOrder->tOrderID,
                          inputOrder->orderRef.data(), Types::orderSideMap[inputOrder->orderSide].data(),  inputOrder->orderPrice, inputOrder->orderVolume,
                          Types::orderStatusMap[inputOrder->orderStatus].data());

            symbol->underlySymbol->riskIndicator.updateRiskIndicator(inputOrder, symbol->instrumentInfo.productIDClass == Types::ProductClass::option);
        }

            bool RiskMonitor::isRiskForNoTrade(const Types::Symbol * underlySymbol, bool isOption) {
                if (underlySymbol->lastMD == nullptr or underlySymbol->lastMD->bidVolume[0] ==0 or underlySymbol->lastMD->askVolume[0] ==0 ) {
                    if (underlySymbol->lastMD != nullptr ) {
                           spdlog::info("[{}_{}], RiskMonitor::isRiskForNoTrade instrumentID={}, bidVolume[0]={}, askVolume[0]={}",
                               m_engineName, underlySymbol->lastMD->updateTime.data(),
                             underlySymbol->instrumentInfo.instrumentID.data(),
                            underlySymbol->lastMD->bidVolume[0], underlySymbol->lastMD->askVolume[0]);
                    }

                    return true;
                }

                auto riskInd = &underlySymbol->riskIndicator;


                int informVolume = (riskInd->sendOrderNumb + riskInd->cancelOrderNumb);
                if (isOption == true)
                {

                    informVolume = (riskInd->optionSendOrderNumb + riskInd->optionQuoteOrderNumb +
                             riskInd->optionCancelOrderNumb) ;
                }

                if (informVolume > m_engineParam.riskInformVolume)
                {
                    spdlog::info("[{}_{}], RiskMonitor::isRiskForNoTrade instrumentID={}, isOption={}, informVolume={}, riskInformVolume={}, ",
                     m_engineName, underlySymbol->lastMD->updateTime.data(),
                     underlySymbol->instrumentInfo.instrumentID.data(), isOption,
                    informVolume,  m_engineParam.riskInformVolume);
                    return true;
                }
                return false;
            }

            bool RiskMonitor::isRiskForOrder(const Types::Symbol * symbol, const Types::OrderField * orderField, bool isOption)
            {
                auto underlySymbol = symbol->underlySymbol;
                if (orderField->orderStatus == Types::OrderStatus::signal)
                {
                    int sectorSize=1;
                    auto sectorNumb = Utils::TradingHours::getPsTimeToNumb(underlySymbol->instrumentInfo.productID, orderField->insertPSTimes, sectorSize);
                    if (sectorNumb ==0) {
                        spdlog::info("[{}_{}], RiskMonitor::isRiskForOrder sectorNumb is zero, instrumentID={}, insertPSTimes={}, sectorSize={}", m_engineName,
                            underlySymbol->lastMD->updateTime.data(), underlySymbol->instrumentInfo.instrumentID.data(), orderField->insertPSTimes, sectorSize
                            );
                        return true;
                    }
                    auto riskMaxOrderNumber =  m_engineParam.riskMaxOrderRatio * sectorNumb /std::min(sectorNumb, 1);
                    if (underlySymbol->riskIndicator.sendOrderNumb > riskMaxOrderNumber) {
                        spdlog::info("[{}_{}], RiskMonitor::isRiskForOrder sendOrderNumb risk, instrumentID={}, sendOrderNumb={}, "
                        "riskMaxOrderNumber={}",m_engineName, underlySymbol->lastMD->updateTime.data(), underlySymbol->instrumentInfo.instrumentID.data(),
                        riskMaxOrderNumber, underlySymbol->riskIndicator.sendOrderNumb);
                       return true;
                    }

                    if (orderField->orderSide == Types::OrderSide::buy && orderField->orderVolume + underlySymbol->riskIndicator.openBuyVolume > m_engineParam.riskOpenVolume) {
                        spdlog::info("[{}_{}], RiskMonitor::isRiskForOrder openBuyVolume risk, instrumentID={}, openBuyVolume={}, "
                        "riskOpenVolume={}",m_engineName, underlySymbol->lastMD->updateTime.data(),
                          underlySymbol->instrumentInfo.instrumentID.data(),  underlySymbol->riskIndicator.openBuyVolume, m_engineParam.riskOpenVolume);
                        return true;
                    }else if (orderField->orderSide == Types::OrderSide::sell && orderField->orderVolume + underlySymbol->riskIndicator.openSellVolume > m_engineParam.riskOpenVolume) {
                        spdlog::info("[{}_{}], RiskMonitor::isRiskForOrder openBuyVolume risk, instrumentID={}, openSellVolume={}, "
                        "riskOpenVolume={}",m_engineName, underlySymbol->lastMD->updateTime.data(), underlySymbol->instrumentInfo.instrumentID.data(),
                        underlySymbol->riskIndicator.openSellVolume, m_engineParam.riskOpenVolume);
                        return true;
                    }

                    if (isOption == true && orderField->OI == Types::OrderIntension::OIHit) {
                        spdlog::info("[{}_{}], RiskMonitor::isRiskForOrder option order not permit hit , instrumentID={}",
                            m_engineName, underlySymbol->lastMD->updateTime.data(), underlySymbol->instrumentInfo.instrumentID.data());
                        return true;
                    }

                    if (underlySymbol->lastMD->psSecond - symbol->riskIndicator.lastSendOrderTime < 15  ) {
                        spdlog::info("[{}_{}], RiskMonitor::isRiskForOrder symbolLastSendTooClose , instrumentID={}",
                        m_engineName, underlySymbol->lastMD->updateTime.data(), symbol->instrumentInfo.instrumentID.data());
                        return true;
                    }
                    if (isOption == false && orderField->pet == Types::PositionEffectType::open && orderField->orderVolume < m_engineParam.futureMinOV ) {
                        spdlog::info("[{}_{}], RiskMonitor::isRiskForOrder open minVolume lessThan futureMinOV, instrumentID={}",
                                             m_engineName, underlySymbol->lastMD->updateTime.data(), symbol->instrumentInfo.instrumentID.data());
                        return true;
                    }
                }
                return false;
            }

            void RiskMonitor::modifyOrderByRisk(const Types::MarketData * pMD, Types::OrderField * orderField, bool isOption) {
                if (isOption == false) {
                    orderField->orderVolume = std::min(orderField->orderVolume, 2*m_engineParam.futureMinVolume);
                }else {
                    orderField->orderVolume = std::min(orderField->orderVolume, 2*m_engineParam.optionMinVolume);
                }

                if (orderField->orderSide == Types::OrderSide::buy && orderField->OI != Types::OrderIntension::OIHit && orderField->orderPrice >= pMD->askPrice[0] + Types::g_epsilon  ) {
                    orderField->orderPrice  = pMD->bidPrice[0];
                }else if(orderField->orderSide == Types::OrderSide::sell && orderField->OI != Types::OrderIntension::OIHit && orderField->orderPrice <= pMD->bidPrice[0] - Types::g_epsilon  ) {
                    orderField->orderPrice  = pMD->askPrice[0];
                }


            };

    }
}