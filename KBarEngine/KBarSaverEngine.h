//
// Created by zhangyw on 4/17/20.
//

#ifndef HFT_MM_OPENENGINE_H
#define HFT_MM_OPENENGINE_H


#include "../Utils/MemoryList.h"
#include "../Driver/RealtimeDriver.h"
#include "../Types/Param.h"
#include "../Types/KPeriod.h"
#include "../KData/KDataManager.h"
#include "../Utils/cppmysql.h"
#include <set>


namespace Cosmos{
    namespace KBarEngine{

        class KBarSaverEngine{

        private:
            Utils::CppMySQL3DB * m_mysql{nullptr};

             Driver::RealtimeDriver *m_driver;
            std::thread m_saveThread;
            folly::ProducerConsumerQueue< std::array<char,512>>  m_squalQueue{9999999};
             Utils::MemoryList< std::array<char,512>, 9999999> m_sqlPool{0};
            KData::KDataManager * m_kDataManager{nullptr};
            std::map< Types::Instrument_t, Types::InstrumentInfo> m_instrumentInfoMap;
            std::vector< Cosmos::Types::InstrumentInfo>* m_tradeInsInfoVec{nullptr};
        public:
            std::string m_engineName;
            double m_riskFreeR{0.023};
            int m_policyID{-1};
            int m_tradingDay{0};
            int m_isDay;
            bool m_isUseUnderlyPrice{true};

            KBarSaverEngine( Driver::RealtimeDriver *driver, std::string &_policyName ,   Utils::CppMySQL3DB * mysql,
                             std::vector< Cosmos::Types::InstrumentInfo>* tradeInsInfoVec, int tradingday ) :
                    m_engineName(_policyName), m_mysql(mysql), m_tradingDay(tradingday), m_tradeInsInfoVec{tradeInsInfoVec} {
                m_driver = driver;
                m_driver->add_receiver< Types::OnSubScribeQuote>(
                    m_driver->passn([this]( Types::OnSubScribeQuote const &onSubScribeQuote) {
                        onRtnSubScribeQuote(std::forward<decltype(onSubScribeQuote)>(onSubScribeQuote));
                    }));
            }

            virtual ~KBarSaverEngine() {}

            void onParams( Types::Param const &param);

            void onStart();


            void onRtnSubScribeQuote( Types::OnSubScribeQuote const & onSubScribeQuote);
            void initKSeries(int tradingday, bool isDay, Types::InstrumentInfo const &insInfo);

            void onEventData( Types::EventData const &);

            void saveKline( KData::KSeries * series, Types::KPeriod period);

            void _saveKline( KData::KSeries * series, int saveIndex,   Types::KPeriod period);

        };
    }
}





#endif //HFT_MM_OPENENGINE_H
