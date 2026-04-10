//
// Created by zhangyingwei on 2026/3/31.
//

#ifndef COSMOS_EXECUTE_H
#define COSMOS_EXECUTE_H

#include "RiskMonitor.h"
#include "../Driver/TestDriver.h"
#include "../Driver/RealtimeDriver.h"
#include "../Types/Type.h"
#include "../Types/Param.h"
#include "../Types/Symbol.h"
#include "../Types/Signal.h"
#include "../Utils/MemoryList.h"
#include "../Utils/LogUtils.h"


namespace Cosmos {
    namespace Engine {
        class Executor {
        private:
            Types::EngineParam m_engineParam;
            Utils::MemoryList<  Types::OrderField ,  Types::OrderBuffSize> m_orderList{0};
            int m_policyID{-1};
            std::string m_engineName;
            int m_tradingDay{0};
            //Driver::RealtimeDriver *m_driver;
             Driver::TestDriver *m_driver{nullptr};
            spdlog::logger* m_positionLog{nullptr};
            spdlog::logger* m_orderLog{nullptr};

            RiskMonitor* m_riskMonitor{nullptr};

        public:
            Executor(decltype(m_driver)  driver, std::string const& engineName, int policyID, int tradingDay,
                Types::EngineParam const& engineParam) : m_driver(driver), m_engineName(engineName),
            m_policyID(policyID), m_tradingDay(tradingDay), m_engineParam(engineParam){
                _setInitLog();
                m_riskMonitor = new RiskMonitor(engineName, policyID, m_tradingDay, m_engineParam);
            }

            void onOrderField(const Types::OrderField *inputOrder,
                                             Types::Symbol *symbol);
            Types::OrderField * getNewOrderField();
            spdlog::logger* getPositionLog();
            int syncPosition(Types::Symbol *symbol, const int64_t epoch_time);

        private:
            void _processFutureSignal(Types::Symbol *symbol, int posDiff, const int64_t epoch_time);
            void _processOptionSignal(Types::Symbol *symbol, int posDiff, const int64_t epoch_time);
            void _cancelOrder(const  Types::OrderField *pOrder,  Types::Symbol *symbol,
                                                const int64_t epoch_time);

            void _sendSignal( Types::Signal const &signal,    Types::Symbol *symbol);
            Types::PositionEffectType _getPet(int& tradeVolume, int T_hold, int Y_hold, int , bool);

            bool _isCancelPendingOrder(const Types::Symbol *symbol, int,  int64_t epoch_time);

            double _setFutureSignalPrice(const Types::Symbol *symbol, Types::OrderSide orderSide);
            double _setOptionSignalPrice(const Types::Symbol *symbol, Types::OrderSide orderSide);

            double getOptionPrioPrice(const Types::Symbol *symbol, char, Types::OrderSide orderSide);

            void _setInitLog();

            int _checkOFI(const Types::MarketData * pMD, double tickSize);

            void _setOrder( Types::Signal const& signal, const Types::Symbol *symbol,  Types::OrderField* order);
        };
    }
}


#endif //COSMOS_EXECUTE_H