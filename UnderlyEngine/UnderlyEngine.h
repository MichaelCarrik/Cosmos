//
// Created by zhangyw on 4/17/20.
//

#ifndef HFT_MM_UNDERLYOPTIONENGINE_H
#define HFT_MM_UNDERLYOPTIONENGINE_H


#include "Executor.h"
#include "../Utils/MemoryList.h"
#include "../Driver/TestDriver.h"
#include "../Types/Param.h"
#include "../Types/KPeriod.h"
#include "../Types/Signal.h"
#include "../KData/KDataManager.h"
#include "../Utils/cppmysql.h"
#include "../Types/Symbol.h"
#include "../Types/QuerySymbol.h"
#include "../Policy/OptionPolicy/IOptionPolicy.h"
#include "../Policy/FuturePolicy/IFuturePolicy.h"
#include "../Policy/OptionPolicy/CoveredVulture.h"
#include "../Utils/Utils.h"



namespace Cosmos {
    namespace Engine {

        class UnderlyEngine {
        private:
            Utils::CppMySQL3DB * m_mySql{nullptr};
            bool m_isReal{true};
            std::vector< Policy::IFuturePolicy*> m_futurePolicyVec;
            std::vector< Policy::IOptionPolicy*> m_optionPolicyVec;
            double m_riskFreeR{0.023};
            std::unordered_map< Types::Instrument_t,   Types::Symbol * ,  Types::InstrumentHash> m_symbolMap;
            std::map<Types::Instrument_t, bool>  m_underlyInitMap;
            Types::Instrument_t m_mainFutureInstr{""};
            KData::KDataManager * m_kDataManager{nullptr};
       //     Utils::MemoryList<  Types::OrderField ,  Types::OrderBuffSize> m_orderList{0};
        //    spdlog::logger* m_orderLog{nullptr};

            int m_tradingDay{0};
            bool m_isDay;

            std::map<Types::Instrument_t, std::map<Types::KPeriod, bool>> m_KPtoHisSeriesMap;
            Types::EngineParam m_engineParam;
            const  std::vector <std::map<std::string, std::string>> * m_subPolicyParamsVec{nullptr};

            Executor *  m_executor{nullptr};

        public:
            std::string m_engineName;
          //    Driver::RealtimeDriver *m_driver;
            Driver::TestDriver *m_driver;
            int m_policyID{-1};

            UnderlyEngine(decltype(m_driver) driver, std::string const& engineName, int policyID, int tradingDay,
                Utils::CppMySQL3DB * mySql , bool isDay, bool isReal) :
                    m_engineName(engineName), m_policyID(policyID), m_tradingDay(tradingDay), m_mySql(mySql),
                    m_isDay(isDay), m_isReal(isReal){
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

            virtual ~UnderlyEngine() {}


            int _syncTargetMap(std::map<Types::Instrument_t, int> const & optionTargetPosMaps,
                                                    std::map<Types::Instrument_t, int> & targetMap);

            void _queryQuote(Types::Instrument_t const& queryInstrumentID);

            void _querySymbol(Types::Instrument_t const& queryInstrumentID);

            void _initUnderly(Types::Instrument_t const&, std::string const& policyType);

            void onInitParams( Types::InitParam const &);

            void onStart();

            void onRtnSubScribeQuote( Types::OnSubScribeQuote const &onSubScribeQuote);

            void onRtnQuerySymbolPosition( Types::OnQuerySymbol const &onQuerySymbol);

            void initKSeries(int tradingday, Types::KPeriod kPeriod,  bool isDay, Types::InstrumentInfo const &insInfo, bool isNeedHis, bool isReal);

            void onInstrumentInfo( Types::InstrumentInfo const &instrumentInfo);

            void onEventData( Types::EventData const &eventData) ;

            void runEvent(const  Types::MarketData *pMD, const int64_t epoch_time);

            Policy::IFuturePolicy * createFuturePolicy(std::string const& policyName , std::map<std::string, std::string> const & paramMap);

             Policy::IOptionPolicy * createOptionPolicy(std::string const& policyName , std::map<std::string, std::string> const & paramMap);

            int syncPosition( Types::Symbol *symbol, const int64_t epoch_time);

            void setKPtoHisSeriesMap(Types::Instrument_t const&,  Types::KPeriod , bool );

            void getFutureInfoByInstrumentID(Types::Instrument_t const& instrument, Types::InstrumentInfo *& futureInsInfo);

            void getOptionInfoByUnderly(Types::Instrument_t const& underlyID, Types::InstrumentInfo *& optionInsInfo);

        };


    }
}


#endif //HFT_MM_UNDERLYOPTIONENGINE_H
