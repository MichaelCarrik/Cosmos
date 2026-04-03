//
// Created by zhangyingwei on 2026/3/31.
//

#include "Executor.h"


namespace Cosmos {
    namespace Engine {
        int Executor::syncPosition(Types::Symbol *symbol, const int64_t epoch_time) {
            int pendingPosition = symbol->getPendingPosition();
            int posDiff = symbol->targetPosition - pendingPosition - symbol->tradePosition.filledPosition;


            if (true == m_riskMonitor->isRiskForNoTrade(symbol->underlySymbol, symbol->instrumentInfo.productIDClass == Types::ProductClass::option)) {

                if (symbol->order->isTerminal == false) {
                     _cancelOrder(symbol->order, symbol, epoch_time);
                    return 0;
                }
            }

            if (symbol->order->isTerminal == false) {
                //&& isPendingOrderTimeout(symbol, epoch_time) == false) {
                if (_isCancelPendingOrder(symbol, posDiff, epoch_time) == true) {

                }
                return 0;
            }

            if (posDiff != 0 && symbol->order->isTerminal == true) {
                spdlog::info(
                    "syncPosition m_engineName={}, updateTime={}, symbolName={}, posDiff={}, targetPosition={}, "
                    "pendingPosition={}, filledPosition={}, orderIsTerminal={}, ",
                    m_engineName, symbol->lastMD->psSecond, symbol->instrumentInfo.instrumentID.data(), posDiff,
                    symbol->targetPosition, pendingPosition, symbol->tradePosition.filledPosition,
                    symbol->order->isTerminal);

                _processFutureSignal(symbol, posDiff, epoch_time);
            }

            return 1;
        }

        void Executor::updateSymbol(const Types::OrderField *inputOrder,
                                    Types::Symbol *symbol) {
            if (inputOrder->policyID != m_policyID) {
                assert(false && "OpenEngine::updateOrder policyID not match");
            }
            auto order = m_orderList.getMemoryById(inputOrder->pOrderID);
            Utils::updateOrder(order, inputOrder);

            if (inputOrder->orderStatus == Types::OrderStatus::failed) {
            } else if (order->orderStatus == Types::OrderStatus::partTraded ||
                       order->orderStatus == Types::OrderStatus::allTraded) {
                symbol->tradePosition.update_filled_position(order->orderSide, order->pet,
                                                             order->orderPrice, order->lastFilledVolume);
                symbol->posWrite(true, m_tradingDay, symbol->lastMD->updateTime,
                                 symbol->lastMD->milliSeconds, order->tOrderID);
            }

            Utils::logOrder(inputOrder, m_orderLog, symbol, inputOrder->orderStatus,
                            m_tradingDay, inputOrder->epoch_time);
        }


        Types::OrderField *Executor::getNewOrderField() {
            return m_orderList.getNewMemory();
        };

        spdlog::logger *Executor::getPositionLog() {
            return m_positionLog;
        };


        double Executor::_setFutureSignalPrice(const Types::Symbol *symbol, Types::OrderSide orderSide) {
            if (m_engineParam.futureEI == Types::ExecuteIntension::EIPut) {
                if (orderSide == Types::OrderSide::buy) {
                    return symbol->lastMD->bidPrice[0];
                } else if (orderSide == Types::OrderSide::sell) {
                    return symbol->lastMD->askPrice[0];
                }
            } else if (m_engineParam.futureEI == Types::ExecuteIntension::EIMid) {
                int ofi = _checkOFI(symbol->lastMD, symbol->instrumentInfo.tickSize);
                if (orderSide == Types::OrderSide::buy) {
                    int ofi = _checkOFI(symbol->lastMD, symbol->instrumentInfo.tickSize);
                    if (ofi == 0) {
                        return floor(symbol->lastMD->midPrice / symbol->instrumentInfo.tickSize) *
                               symbol->instrumentInfo.tickSize;
                    } else if (ofi < 0) {
                        return symbol->lastMD->askPrice[0];
                    } else {
                        return symbol->lastMD->bidPrice[0];
                    }
                } else if (orderSide == Types::OrderSide::sell) {
                    if (ofi == 0) {
                        return ceil(symbol->lastMD->midPrice / symbol->instrumentInfo.tickSize) *
                               symbol->instrumentInfo.tickSize;
                    } else if (ofi > 0) {
                        return symbol->lastMD->bidPrice[0];
                    } else {
                        return symbol->lastMD->askPrice[0];
                    }
                }
            } else if (m_engineParam.futureEI == Types::ExecuteIntension::EIHit) {
                if (orderSide == Types::OrderSide::buy) {
                    return symbol->lastMD->askPrice[0];
                } else if (orderSide == Types::OrderSide::sell) {
                    return symbol->lastMD->bidPrice[0];
                }
            }

            assert(false);
        }

        double Executor::_setOptionSignalPrice(const Types::Symbol *symbol, Types::OrderSide orderSide) {
            if (m_engineParam.futureEI == Types::ExecuteIntension::EIPut) {
                if (orderSide == Types::OrderSide::buy) {
                    return symbol->lastMD->bidPrice[0];
                } else if (orderSide == Types::OrderSide::sell) {
                    return symbol->lastMD->askPrice[0];
                }
            } else if (m_engineParam.futureEI == Types::ExecuteIntension::EIMid) {
                int ofi = _checkOFI(symbol->lastMD, symbol->instrumentInfo.tickSize);
                if (orderSide == Types::OrderSide::buy) {
                    if (ofi == 0) { //tickSize large 1
                        return getOptionPrioPrice(symbol, symbol->instrumentInfo.optionType, orderSide);
                    }
                    return symbol->lastMD->bidPrice[0];
                } else if (orderSide == Types::OrderSide::sell) {
                    if (ofi == 0) {
                        return getOptionPrioPrice(symbol, symbol->instrumentInfo.optionType, orderSide);
                    }
                    return symbol->lastMD->askPrice[0];
                }
            }


            assert(false);
        }


        double Executor::getOptionPrioPrice(const Types::Symbol *symbol, char optionType, Types::OrderSide orderSide) {
            if (symbol->m_kSeriesMap.begin() != symbol->m_kSeriesMap.end()) {
                auto kseries = symbol->m_kSeriesMap.begin()->second;
                int strikPrice = static_cast<int>(symbol->instrumentInfo.strikePrice);
                auto itrCallPut = kseries->m_callPutSeriesMap->find(strikPrice);
                if (itrCallPut != kseries->m_callPutSeriesMap->end()) {
                    auto callMD = itrCallPut->second->callSeries->m_lastPMD;
                    auto putMD = itrCallPut->second->putSeries->m_lastPMD;
                    auto underlyMD = kseries->m_underlySeries->m_lastPMD;
                    if (optionType == 'C' && orderSide == Types::OrderSide::buy) {
                        auto callTheoryBidPrice = underlyMD->bidPrice[0] + putMD->bidPrice[0] - strikPrice;
                        return std::max(callTheoryBidPrice, symbol->lastMD->bidPrice[0]);
                    }else if (optionType == 'P' && orderSide == Types::OrderSide::buy) {
                        auto putTheoryBidPrice = callMD->bidPrice[0] -underlyMD->askPrice[0] + strikPrice;
                        return std::max(putTheoryBidPrice, symbol->lastMD->bidPrice[0]);
                    }else if (optionType == 'C' && orderSide == Types::OrderSide::sell) {
                        auto callTheoryAskPrice = underlyMD->askPrice[0] + putMD->askPrice[0] - strikPrice;
                        return std::min(callTheoryAskPrice, symbol->lastMD->askPrice[0]);
                    }else if (optionType == 'P' && orderSide == Types::OrderSide::sell) {
                        auto putTheoryAskPrice = callMD->askPrice[0] -underlyMD->bidPrice[0] + strikPrice;
                        return std::min(putTheoryAskPrice, symbol->lastMD->askPrice[0]);
                    }
                }

                if (orderSide == Types::OrderSide::buy) {
                    return symbol->lastMD->bidPrice[0];
                }else if (orderSide == Types::OrderSide::sell) {
                    return symbol->lastMD->askPrice[0];
                }
            };

            assert(false);
        }


        int Executor::_checkOFI(const Types::MarketData *pMD, double tickSize) {
            if (round((pMD->askPrice[0] - pMD->bidPrice[0]) / tickSize) == 1) {
                return pMD->askVolume[0] - pMD->bidVolume[0];
            }
            return 0;
        }

        void Executor::_processFutureSignal(Types::Symbol *symbol, int posDiff, const int64_t epoch_time) {
            Types::Signal signal;

            if (posDiff > 0) {
                signal.signalSide = Types::OrderSide::buy;
                signal.price = _setFutureSignalPrice(symbol, signal.signalSide);
                signal.orderTimeType = Types::OrderTimeType::COMMON;
                signal.volume = std::min(m_engineParam.futureMinVolume,  abs(posDiff)); // abs(posDiff);
                signal.epoch_time = epoch_time;
                _sendSignal(signal, symbol);
            } else if (posDiff < 0) {
                signal.signalSide = Types::OrderSide::sell;
                signal.price = _setFutureSignalPrice(symbol, signal.signalSide);
                signal.orderTimeType = Types::OrderTimeType::COMMON;
                signal.volume = std::min(m_engineParam.futureMinVolume,  abs(posDiff)); //abs(posDiff);
                signal.epoch_time = epoch_time;
                _sendSignal(signal, symbol);
            }
        }

        void Executor::_processOptionSignal(Types::Symbol *symbol, int posDiff, const int64_t epoch_time) {
            Types::Signal signal;

            if (posDiff > 0) {
                signal.signalSide = Types::OrderSide::buy;
                signal.price = _setOptionSignalPrice(symbol, signal.signalSide);
                signal.orderTimeType = Types::OrderTimeType::COMMON;
                signal.volume = std::min(m_engineParam.optionMinVolume, abs(posDiff)); // abs(posDiff);

                signal.epoch_time = epoch_time;
                _sendSignal(signal, symbol);
            } else if (posDiff < 0) {
                signal.signalSide = Types::OrderSide::sell;
                signal.price = _setOptionSignalPrice(symbol, signal.signalSide);
                signal.orderTimeType = Types::OrderTimeType::COMMON;
                signal.volume = std::min(m_engineParam.optionMinVolume,  abs(posDiff)); //abs(posDiff);
                signal.epoch_time = epoch_time;
                _sendSignal(signal, symbol);
            }
        }

        void Executor::_sendSignal(Types::Signal const &signal, Types::Symbol *symbol) {

            int assignid = 0;
            auto order = m_orderList.getNewMemory(assignid);
            order->pOrderID = assignid;

            _setOrder(signal, symbol, order);
            m_riskMonitor->modifyOrderByRisk(symbol->lastMD, order, symbol->instrumentInfo.productIDClass == Types::ProductClass::option);
            if (true == m_riskMonitor->isRiskForOrder(symbol->underlySymbol, order)) {
                order->OI = Types::OrderIntension::OINoT;
            }

            Utils::logOrder(order, m_orderLog, symbol, Types::OrderStatus::signal,
                            m_tradingDay, signal.epoch_time);
            if (order->OI != Types::OrderIntension::OINoT) {
                symbol->order = order;
                m_driver->sendOrder(*order);
            }
        }

        void Executor::_cancelOrder(const Types::OrderField *pOrder, Types::Symbol *symbol,
                                    const int64_t epoch_time) {
            if (pOrder != nullptr && !pOrder->isTerminal) {
                int64_t sendTime = 0;
                Utils::logOrder(pOrder, m_orderLog, symbol, Types::OrderStatus::cancel, m_tradingDay,
                                epoch_time);
                m_driver->cancelOrder(*pOrder, sendTime);
                //    symbol->lastOrderPsTime = symbol->lastMD->psSecond;
            }
        }

        bool Executor::_isCancelPendingOrder(const Types::Symbol *symbol, int diffPos, int64_t epoch_time) {
            //            	fprintf(stderr, "isPendingOrderTimeout, symbolName=%s, psSecond=%d, lastOrderPsTime=%d, m_putResendTimeout=%d\n",
            //            			  symbol->instrumentInfo.instrumentID.data(), symbol->lastMD->psSecond,symbol->lastOrderPsTime, m_putResendTimeout);

            if ((diffPos > 0 && symbol->order->orderSide == Types::OrderSide::sell) ||
                (diffPos < 0 && symbol->order->orderSide == Types::OrderSide::buy)) {
                return true;
            }


            int timeOut = m_engineParam.putResendTimeout;
            if (symbol->order->OI != Types::OrderIntension::OIPut) {
                timeOut = m_engineParam.hitResendTimeout;
            }

            if (symbol->lastMD->psSecond - symbol->order->insertPSTimes > timeOut) {
                return true;
            } else if (symbol->order->OI == Types::OrderIntension::OIMid) {
                int ofi = _checkOFI(symbol->lastMD, symbol->instrumentInfo.tickSize);
                if (symbol->order->orderSide == Types::OrderSide::buy && ofi > 0 and
                    abs(symbol->order->orderPrice - symbol->lastMD->bidPrice[0]) < Types::g_epsilon) {
                    return true;
                } else if (symbol->order->orderSide == Types::OrderSide::sell && ofi < 0 and
                           abs(symbol->order->orderPrice - symbol->lastMD->askPrice[0]) < Types::g_epsilon) {
                    return true;
                }
            }
            return false;
        }

        void Executor::_setInitLog() {
            std::string record = "record";
            m_orderLog = Utils::initLogs(record, this->m_engineName, record);
            std::string position = "position";
            m_positionLog = Utils::initLogs(position, this->m_engineName, position);
        };

        Types::PositionEffectType Executor::_getPet(int &tradeVolume, int T_hold, int Y_hold, int minPosition,
                                                    bool prioCloseToday) {
            if (prioCloseToday == 1 && T_hold >= tradeVolume && (T_hold + Y_hold - tradeVolume >= minPosition)) {
                return Types::PositionEffectType::T_close;
            } else if (Y_hold >= tradeVolume && (T_hold + Y_hold - tradeVolume >= minPosition)) {
                return Types::PositionEffectType::Y_close;
            } else if (T_hold > 0 && T_hold < tradeVolume) {
                tradeVolume = T_hold;
                return Types::PositionEffectType::T_close;
            } else if (Y_hold > 0 && Y_hold < tradeVolume) {
                tradeVolume = Y_hold;
                return Types::PositionEffectType::Y_close;
            }
            return Types::PositionEffectType::open;
        }

        void Executor::_setOrder( Types::Signal const& signal, const Types::Symbol *symbol,  Types::OrderField* order) {
            order->lastFilledVolume = 0;
            order->lastFilledPrice =0.0;
            order->filledVolume = 0;
            memset(order->orderSysID.data() , 0, sizeof(order->orderSysID));
            order->tOrderID=0;
            order->policyID = m_policyID;
            order->orderPrice = signal.price;
            order->orderVolume = signal.volume;
            order->OI = signal.OI;
            order->orderSide = signal.signalSide;
            order->orderTimeType = signal.orderTimeType;
            order->insertPSTimes = symbol->lastMD->psSecond;
            order->hedgeType = m_engineParam.hedgeType;
            if(order->orderSide ==  Types::OrderSide::buy){
                order->pet = this->_getPet(order->orderVolume, symbol->tradePosition.T_sellHold, symbol->tradePosition.Y_sellHold, 0, m_engineParam.futurePreCloseToday);
            }else{
                order->pet = this->_getPet(order->orderVolume, symbol->tradePosition.T_buyHold, symbol->tradePosition.Y_buyHold, 0, m_engineParam.futurePreCloseToday);
            }
            order->instrumentID = symbol->instrumentInfo.instrumentID;
            //   order->exchangeId = symbol->exchangeId;
            order->orderStatus =  Types::OrderStatus::signal;
            order->isTerminal= false;
        };
    }
}
