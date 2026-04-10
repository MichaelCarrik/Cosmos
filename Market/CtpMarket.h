//
// Created by zhangyw on 7/21/19.
//

#ifndef TRADEBOTS_CTPMARKET_H
#define TRADEBOTS_CTPMARKET_H



#include "ThostFtdcMdApi.h"
#include "../Driver/RealtimeDriver.h"
#include "../Utils/MemoryList.h"
#include "future"
#include <set>
#include "../KData/KDataManager.h"

namespace Cosmos {
    namespace Market {

        class CtpMarket : public CThostFtdcMdSpi {
        public:
         //   PairsTrading::KData::KDataManager m_kDataManager;

        private:

            struct CtpMarketConnection {
                std::string username;
                std::string password;
                std::string brokerId;
                std::string marketFront;

            };


            CtpMarketConnection m_ctpMarketConnection;

            std::unordered_map< Types::Instrument_t , Types::PushMarket*, Types::InstrumentHash> m_subScribeInstruments;

        //    std::vector<std::vector<int>> m_subScribeCodeToPolicy;

            std::string m_configPath;

            std::promise<int> m_loginPromise;
            CThostFtdcMdApi *m_pMdApi;
            bool m_isLogin{false};
            bool m_isDay{false};

//            std::map< Types::Instrument_t ,  Types::MarketData*> m_lastMarketMap;

             Driver::RealtimeDriver *m_driver;

        public:
            CtpMarket(Driver::RealtimeDriver *driver, std::string &config_path) : m_configPath(config_path) {
                m_driver = driver;
             //   m_kDataManager.m_mysql = mySql;
            }

            ~CtpMarket() {
                if (m_pMdApi != nullptr) {
                    m_pMdApi->RegisterSpi(nullptr);
                    m_pMdApi->Release();
                }
            }

            int start(std::vector<Types::MarketData *> const& initMarketDataVector, bool);

            void SubScribeQuote(Types::SubScribeQuote const &subScribeQuot);
           // PairsTrading::KData::KDataManager* initKSeries(int tradingday, bool, std::set<PairsTrading::Types::Instrument_t> *);

            void OnFrontConnected();

            void OnFrontDisconnected(int nReason);

            void
            OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast);

            void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

            virtual void
            OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) {
                if (pRspInfo->ErrorID == 0) {
                //    spdlog::info("subscribe instrument={}  ok", pSpecificInstrument->InstrumentID);
                }
            }
            //subscribe market data;
            int SubscribeAllMarketData();
            //unsubscribe market data;
            int UnsubscribeMarketData();
        };
    };
}



#endif //TRADEBOTS_CTPMARKET_H
