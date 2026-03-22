//
// Created by zhangyw on 4/17/20.
//

#ifndef HFT_MM_UNDERLYOPTIONENGINE_H
#define HFT_MM_UNDERLYOPTIONENGINE_H


#include "../Utils/MemoryList.h"
//#include "../Driver/RealtimeDriver.h"
#include "../Driver/TestDriver.h"
#include "../Types/Param.h"
#include "../Types/KPeriod.h"
#include "../Types/Signal.h"
#include "../Types/LogStruct.h"
#include "../KData/KDataManager.h"
#include "../Utils/cppmysql.h"
#include "../KData/UpdateGreeks.h"
#include "../Types/Symbol.h"
#include "../Types/QuerySymbol.h"
#include "../Types/UnderlyInsMap.h"
#include "../Policy/IOptionPolicy.h"
#include "../Utils/Utils.h"
#include <set>
#include <fstream>
#include <algorithm>

namespace Cosmos {
    namespace UnderlyOptionEngine {
        class UnderlyOptionEngine {
        private:
            bool m_isReal{true};
             Driver::RealtimeDriver *m_driver;
           //  Driver::TestDriver *m_driver;
            std::unordered_map< Types::Instrument_t,   Types::Symbol * ,  Types::InstrumentHash> m_symbolMap;
            KData::KDataManager m_kDataManager;
            std::map< Types::Instrument_t, std::unordered_map< Types::KPeriod, int *> *> m_allKLineIndexs;
           //  Types::Instrument_t  m_underlyInstrument{""};
             Utils::MemoryList<  Types::OrderField ,  Types::OrderBuffSize> m_orderList{0};
            std::shared_ptr<spdlog::logger> m_orderLog;
            std::shared_ptr<spdlog::logger> m_positionLog;
            int m_putResendTimeout;
            int m_resendCount;
            bool m_preCloseToday{true};
            Types::HedgeType m_hedgeType{Types::HedgeType::spec};
            int m_affiThreadId{-1};
            int m_rebackTickCounts{5};
            int m_minVolume{1};
            double m_tickSize;
            double m_multi;
            int m_maxPosition{0};
            std::map<Types::KPeriod, bool> m_policyKPMap;

             Utils::CppMySQL3DB * m_mySql{nullptr};

            int m_expireDate{0};

            KData::UpdateGreeks m_updateGreeks;
         //   RiskOrderControl  m_riskOrderControl;
            int m_lastPS{0};
        public:
            std::string m_engineName;
            std::vector< Policy::IOptionPolicy*> m_policyVec;
            double m_riskFreeR{0.023};
            Types::UnderlyInsMap m_underlyInsMap;
//            Types::Instrument_t m_underlyID{""};
//            Types::Instrument_t m_farUnderlyID{""};
            int m_policyID{-1};
            int m_tradingDay{0};
            bool m_isDay;

            UnderlyOptionEngine(decltype(m_driver) driver, std::string &_policyName,   Utils::CppMySQL3DB * mySql , bool isDay) :
                    m_engineName(_policyName), m_mySql(mySql), m_isDay(isDay) {




                m_kDataManager.m_mysql = m_mySql;

                m_driver = driver;
                m_driver->template add_receiver< Types::OnQuerySymbol>(
                        m_driver->passn([this]( Types::OnQuerySymbol const & onQuerySymbol) {
                            onRtnQuerySymbolPosition(std::forward<decltype(onQuerySymbol)>(onQuerySymbol));
                        }));

                m_driver->add_receiver< Types::OnSubScribeQuote>(
                        m_driver->passn([this]( Types::OnSubScribeQuote const &onSubScribeQuote) {
                            onRtnSubScribeQuote(std::forward<decltype(onSubScribeQuote)>(onSubScribeQuote));
                        }));

                m_driver->template add_receiver< Types::InstrumentInfo>(
                        m_driver->passn([this]( Types::InstrumentInfo const &instrumentInfo) {
                            onInstrumentInfo(std::forward<decltype(instrumentInfo)>(instrumentInfo));
                        }));
            }

            virtual ~UnderlyOptionEngine() {}
            void setInitLog(Types::LogStruct &logStruct);

            int _syncTargetMap(std::map<Types::Instrument_t, int> const & optionTargetPosMaps,
                                                    std::map<Types::Instrument_t, int> & targetMap);

            void _queryQuote(Types::Instrument_t const& queryInstrumentID);

            void _querySymbol(Types::Instrument_t const& queryInstrumentID);

            void _initUnderly(std::string const& underlyIDStr);

            void onParams( Types::Param const &param);

            void onStart();

            void onRtnSubScribeQuote( Types::OnSubScribeQuote const &onSubScribeQuote);

            void onRtnQuerySymbolPosition( Types::OnQuerySymbol const &onQuerySymbol);

            void initKSeries(int tradingday, Types::KPeriod kPeriod,  bool isDay, Types::InstrumentInfo const &insInfo, bool isNeedHis, bool isReal) {
                Utils::TradingHours::initInstrumentTradingHours(insInfo.instrumentID);
                std::unordered_map< Types::KPeriod,  KData::KSeries *> *KSeries_map;
                std::vector< KData::KData *> hisKline;
                if(isNeedHis==true){
                    m_kDataManager.getHisKbars(insInfo.instrumentID, insInfo.productIDClass, kPeriod, hisKline, tradingday,
                                               isReal);
                }

                m_kDataManager.initKSeries(insInfo, kPeriod, tradingday, m_riskFreeR,
                                           hisKline, isDay);
            }

            void onInstrumentInfo( Types::InstrumentInfo const &instrumentInfo);

            void onEventData( Types::EventData const &eventData) ;

            void runEvent(const  Types::MarketData *pMD, const int64_t epoch_time);

             Policy::IOptionPolicy * createPolicy(std::map<std::string, std::string> const & paramMap, std::string const& policyName);

            int syncPosition( Types::Symbol *symbol, const int64_t epoch_time);
            bool isPendingOrderTimeout(const Types::Symbol *symbol,   int64_t epoch_time);
            void processSignal(Types::Symbol *symbol, int posDiff, const int64_t epoch_time);
            void cancelOrder(const  Types::OrderField *pOrder,  Types::Symbol *symbol,
                                                  const int64_t epoch_time);
            Types::SignalIntension getIntension(const Types::Symbol *symbol);
            void sendSignal( Types::Signal const &signal,    Types::Symbol *symbol);

            template<typename T>
            void setOrder( Types::Signal const& signal,  Types::OrderField* order, T *symbol) {

                order->lastFilledVolume = 0;
                order->lastFilledPrice =0.0;
                order->filledVolume = 0;
                memset(order->orderSysID.data() , 0, sizeof(order->orderSysID));
                order->tOrderID=0;

                order->policyID = m_policyID;

                order->orderPrice = signal.price;
                order->orderVolume = signal.volume;
                order->intension = signal.intension;
                order->orderSide = signal.signalSide;
                order->orderTimeType = signal.orderTimeType;

                order->hedgeType = m_hedgeType;

                if(order->orderSide ==  Types::OrderSide::buy){
                    order->pet = this->getPet(order->orderVolume, symbol->tradePosition.T_sellHold, symbol->tradePosition.Y_sellHold, 0, m_preCloseToday);
                }else{
                    order->pet = this->getPet(order->orderVolume, symbol->tradePosition.T_buyHold, symbol->tradePosition.Y_buyHold, 0, m_preCloseToday);
                }

                order->instrumentID = symbol->instrumentInfo.instrumentID;
                order->exchangeId = symbol->exchangeId;
                order->orderStatus =  Types::OrderStatus::signal;
                order->isTerminal= false;
            };

            Types::PositionEffectType getPet(int& tradeVolume, int T_hold, int Y_hold, int , bool);
            void updateMMOrder(const  Types::OrderField *inputOrder,  Types::Symbol *symbol);

        };


    }
}


#endif //HFT_MM_UNDERLYOPTIONENGINE_H
