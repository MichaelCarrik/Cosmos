//
// Created by zhangyingwei on 2026/4/2.
//

#include "RiskMonitor.h"


namespace Cosmos {
    namespace Engine {

            bool RiskMonitor::isRiskForNoTrade(const Types::Symbol * underlySymbol, bool isOption) {
                if (underlySymbol->lastMD->bidVolume[0] ==0 or underlySymbol->lastMD->bidVolume[0] ==0 ) {
                    return true;
                }

                auto riskInd = &underlySymbol->riskIndicator;


                int OTR = (riskInd->sendOrderNumb + riskInd->cancelOrderNumb) / std::min(riskInd->filledOrderNumb, 1);
                if (isOption == true)
                {
                    OTR = (riskInd->optionSendOrderNumb + riskInd->optionQuoteOrderNumb +
                             riskInd->optionCancelOrderNumb) / std::min(riskInd->optionFilledOrderNumb, 1);
                }

                if (OTR > m_engineParam.riskOTR)
                {
                    return true;
                }
                return false;
            }

            bool RiskMonitor::isRiskForOrder(const Types::Symbol * underlySymbol, const Types::OrderField * orderField)
            {
                if (orderField->orderStatus == Types::OrderStatus::signal)
                {
                    int sectorSize=1;
                    auto sectorNumb = Utils::TradingHours::getPsTimeToNumb(underlySymbol->instrumentInfo.productID, orderField->insertPSTimes, sectorSize);
                    auto riskMaxOrderNumber =  m_engineParam.riskMaxOrderNumb * sectorNumb /std::min(sectorNumb, 1);
                    if (underlySymbol->riskIndicator.sendOrderNumb > riskMaxOrderNumber) {
                       return true;
                    }

                    if (orderField->orderSide == Types::OrderSide::buy && orderField->orderVolume + underlySymbol->riskIndicator.openBuyVolume > m_engineParam.riskOpenVolume) {
                        return true;
                    }else if (orderField->orderSide == Types::OrderSide::sell && orderField->orderVolume + underlySymbol->riskIndicator.openSellVolume > m_engineParam.riskOpenVolume) {
                        return true;
                    }
                }
                return false;
            }

            void RiskMonitor::modifyOrderByRisk(const Types::MarketData * pMD, Types::OrderField * orderField, bool isOption) {
                if (isOption == false) {
                    orderField->orderVolume = std::min(orderField->orderVolume, m_engineParam.futureMinVolume);
                }else {
                    orderField->orderVolume = std::min(orderField->orderVolume, m_engineParam.optionMinVolume);
                }

                if (orderField->orderSide == Types::OrderSide::buy && orderField->OI != Types::OrderIntension::OIHit && orderField->orderPrice >= pMD->askPrice[0] + Types::g_epsilon  ) {
                    orderField->orderPrice  = pMD->bidPrice[0];
                }else if(orderField->orderSide == Types::OrderSide::sell && orderField->OI != Types::OrderIntension::OIHit && orderField->orderPrice <= pMD->bidPrice[0] - Types::g_epsilon  ) {
                    orderField->orderPrice  = pMD->askPrice[0];
                }


            };

    }
}