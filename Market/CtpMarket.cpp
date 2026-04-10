//
// Created by zhangyw on 7/21/19.
//

#include "CtpMarket.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "../Utils/TradingHours.h"
#include <../Utils/Utils.h>
#include "iostream"

namespace Cosmos {
    namespace Market {
        int CtpMarket::start(std::vector<Types::MarketData *> const &initMarketDataVector, bool isDay) {
            m_isDay = isDay;
            boost::property_tree::ptree pt;
            boost::property_tree::read_xml(m_configPath, pt);
            m_ctpMarketConnection.username = pt.get_child("ThostUser2.ConnectConfig.Username").get<std::string>(
                "<xmlattr>.value");
            m_ctpMarketConnection.password = pt.get_child("ThostUser2.ConnectConfig.Password").get<std::string>(
                "<xmlattr>.value");
            m_ctpMarketConnection.marketFront = pt.get_child("ThostUser2.ConnectConfig.MarketFront").get<std::string>(
                "<xmlattr>.address");
            m_ctpMarketConnection.brokerId = pt.get_child("ThostUser2.ConnectConfig.BrokerID").get<std::string>(
                "<xmlattr>.name");

            m_pMdApi = CThostFtdcMdApi::CreateFtdcMdApi("", false);

            m_pMdApi->RegisterSpi(this);
            //   m_pMdApi->SubscribePublicTopic(THOST_TERT_RESTART);

            std::array<char, 128> marketFront{""};
            std::copy(std::begin(m_ctpMarketConnection.marketFront), std::end(m_ctpMarketConnection.marketFront),
                      std::begin(marketFront));

            m_pMdApi->RegisterFront(marketFront.data());

            m_pMdApi->Init();

            for (auto &initMarket: initMarketDataVector) {
                auto itr = m_subScribeInstruments.find(initMarket->instrumentID);
                if (itr != m_subScribeInstruments.end()) {
                    if (Utils::FTTrait::FT_TRADING == Utils::TradingHours::getProductTrait(
                            initMarket->productID, initMarket->psSecond, m_isDay) ||
                        Utils::FTTrait::FT_AUCTION == Utils::TradingHours::getProductTrait(
                             initMarket->productID, initMarket->psSecond,  m_isDay) ||
                            (initMarket->psSecond >= 15 * 3600  &&  initMarket->psSecond <= 17 * 3600)) {
                        auto event = itr->second->eventDataList.getNewMemory();
                        event->point = initMarket;
                        event->eventType = Types::EventType::marketEvent;
                        for (auto i: itr->second->subscribePolicy) {
                            if (initMarket == nullptr) {
                                assert(false && "market is nullptr");
                            }

                            // fprintf(stderr, "initMarket %s %s\n", initMarket->instrumentID.data(),
                            //         initMarket->updateTime.data());

                            m_driver->callback_asyncEventData(event, i);
                        }
                    }
                }
            }

            auto loginFuture = m_loginPromise.get_future();
            auto status = loginFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                std::cout << "deferred\n";
            } else if (status == std::future_status::timeout) {
                spdlog::error("market login timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                auto ret = loginFuture.get();
                if (ret != 0) {
                    spdlog::error("market login retcode : {}", ret);
                }
                this->SubscribeAllMarketData();
            }
            m_isLogin = true;
            return 0;
        }


        void CtpMarket::OnFrontConnected() {
            spdlog::info("market connected");
            CThostFtdcReqUserLoginField reqUserLogin;
            memset(&reqUserLogin, 0, sizeof(reqUserLogin));

            strcpy(reqUserLogin.BrokerID, m_ctpMarketConnection.brokerId.c_str());
            strcpy(reqUserLogin.UserID, m_ctpMarketConnection.username.c_str());
            strcpy(reqUserLogin.Password, m_ctpMarketConnection.password.c_str());

            //send login request;
            auto retCode = m_pMdApi->ReqUserLogin(&reqUserLogin, 0);
            if (retCode != 0) {
                m_loginPromise.set_value(2);
                fprintf(stderr, "OnFront connected error=%d\n", retCode);
                spdlog::error("market ReqUserLogin error={}", retCode);
            }
        }

        void CtpMarket::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID == 0 and m_isLogin == false) {
                spdlog::info("market login successfully");
                m_loginPromise.set_value(0);
            } else if (pRspInfo and m_isLogin == false) {
                spdlog::error("market login errpr errorid={},  errorMsg={}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                m_loginPromise.set_value(3);
            }
        };

        void CtpMarket::OnFrontDisconnected(int nReason) {
            spdlog::error("market front disconnected, reason: {}", nReason);
            //     m_loginPromise.set_value(1);
        };

        void CtpMarket::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
            if (pDepthMarketData != nullptr) {
                Types::Instrument_t instemp{""};
                strcpy(instemp.data(), pDepthMarketData->InstrumentID);
                auto itr = m_subScribeInstruments.find(instemp);
                auto event = itr->second->eventDataList.getNewMemory();
                auto marketData = itr->second->marketDataList.getNewMemory();
                marketData->epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                Utils::convertToMarketDaTa(pDepthMarketData, marketData);
                marketData->isInit = 0;
                if (Utils::FTTrait::FT_TRADING == Utils::TradingHours::getProductTrait(
                        marketData->productID, marketData->psSecond, m_isDay ) ||
                    Utils::FTTrait::FT_AUCTION == Utils::TradingHours::getProductTrait(
                        marketData->productID, marketData->psSecond, m_isDay ) ||
                    (marketData->psSecond >= 15 * 3600  &&  marketData->psSecond <= 17 * 3600)) {
                    event->point = marketData;
                    event->eventType = Types::EventType::marketEvent;
                    for (auto i: itr->second->subscribePolicy) {
                        if (marketData == nullptr) {
                            assert(false && "market is nullptr");
                        }
                        m_driver->callback_asyncEventData(event, i);
                    }
                }
            }
        }

        int CtpMarket::SubscribeAllMarketData() {
            char **pNewCodes = new char *[m_subScribeInstruments.size()];
            int count = 0;
            int nRet = -1;
            for (auto instrument: m_subScribeInstruments) {
                pNewCodes[count] = new char[32];
                strcpy(pNewCodes[count], instrument.first.data());
                count++;
            }

            if (count > 0) {
                nRet = m_pMdApi->SubscribeMarketData(pNewCodes, count);
                if (nRet != 0) {
                    spdlog::error("subscribe mds return non-zero, errorid: {}\n", nRet);
                }
                for (int i = 0; i < count; i++) {
                    delete[] pNewCodes[i];
                }
            }
            delete[] pNewCodes;
            return nRet;
        }

        void CtpMarket::SubScribeQuote(Types::SubScribeQuote const &subScribeQuote) {
            //       fprintf(stderr, "FemasMarket::SubScribeQuote %d %s\n", subScribeQuote.policyID, subScribeQuote.instrumentID.data());
            Types::Instrument_t instrument{""};
            strcpy(instrument.data(), subScribeQuote.instrumentID.data());

            auto itr = m_subScribeInstruments.find(instrument);
            if (itr == m_subScribeInstruments.end()) {
                Types::PushMarket *pushMarket = new Types::PushMarket(0);
                m_subScribeInstruments[instrument] = pushMarket;
            }

            m_subScribeInstruments[instrument]->subscribePolicy.emplace_back(subScribeQuote.policyID);
            Types::OnSubScribeQuote onSubScribeQuote;
            onSubScribeQuote.policyID = subScribeQuote.policyID;
            strcpy(onSubScribeQuote.instrumentID.data(), instrument.data());

            m_driver->send(onSubScribeQuote);
        }

        int CtpMarket::UnsubscribeMarketData() {
            return 0;
        }
    }
}
