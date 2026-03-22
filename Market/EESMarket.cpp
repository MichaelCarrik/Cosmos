//
// Created by zhangyw on 3/9/21.
//

#include "EESMarket.h"

namespace Cosmos {
    namespace Market {
        int EESMarket::start() {
            boost::property_tree::ptree pt;
            boost::property_tree::read_xml(m_configPath, pt);


            m_marketConnect.username = pt.get_child("ThostUser2.ConnectConfig.Username").get<std::string>(
                    "<xmlattr>.value");
            m_marketConnect.password = pt.get_child("ThostUser2.ConnectConfig.Password").get<std::string>(
                    "<xmlattr>.value");
            m_marketConnect.host = pt.get_child("ThostUser2.ConnectConfig.host").get<std::string>("<xmlattr>.value");
            m_marketConnect.port = pt.get_child("ThostUser2.ConnectConfig.port").get<int>("<xmlattr>.value");
            TAPIINT32 iResult = TAPIERROR_SUCCEED;
            TapAPIApplicationInfo stAppInfo;
            memset(&stAppInfo, 0, sizeof(TapAPIApplicationInfo));

            ITapQuoteAPI *pAPI = CreateTapQuoteAPI(&stAppInfo, iResult);
            if (nullptr == pAPI) {
                spdlog::error("EESMarket create api error={}", iResult);
                return -1;
            }

            //设定ITapQuoteAPINotify的实现类，用于异步消息的接收

            pAPI->SetAPINotify(this);


            //启动测试，订阅行情
            this->SetAPI(pAPI);

            TAPIINT32 iErr = TAPIERROR_SUCCEED;
            iErr = pAPI->SetHostAddress(m_marketConnect.host.c_str(), m_marketConnect.port);
            if (TAPIERROR_SUCCEED != iErr) {
                spdlog::error("EESMarket SetHostAddress Error: {}", iErr);
            }
            TapAPIQuoteLoginAuth stLoginAuth;
            memset(&stLoginAuth, 0, sizeof(stLoginAuth));
            strcpy(stLoginAuth.UserNo, m_marketConnect.username.c_str());
            strcpy(stLoginAuth.Password, m_marketConnect.password.c_str());
            stLoginAuth.ISModifyPassword = APIYNFLAG_NO;
            stLoginAuth.ISDDA = APIYNFLAG_NO;
            iErr = m_pMdApi->Login(&stLoginAuth);
            if (TAPIERROR_SUCCEED != iErr) {
                spdlog::error("Login Market Error : {}", iErr);

                return -1;
            }
            int ret = -1;
            auto loginFuture = m_loginPromise.get_future();
            auto status = loginFuture.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::deferred) {
                ret = -2;
            } else if (status == std::future_status::timeout) {
                spdlog::error("market login timeout");
                return -1;
            } else if (status == std::future_status::ready) {
                ret = loginFuture.get();
                if (ret != 0) {
                    spdlog::error("market login retcode : {}", ret);
                }
                this->subscribeAllMarketData();

            }

            SetTapQuoteAPILogLevel(APILOGLEVEL_NONE);
            return ret;

        }

        void EESMarket::SetAPI(ITapQuoteAPI *pAPI) {
            m_pMdApi = pAPI;
        }

        void TAP_CDECL
        EESMarket::OnDisconnect(TAPIINT32 reasonCode) {
            spdlog::error("market front disconnected, reason: {}", reasonCode);

        }

        void TAP_CDECL
        EESMarket::OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo *info) {
            if (TAPIERROR_SUCCEED == errorCode) {
                spdlog::info("market login successfully");
            } else {
                m_loginPromise.set_value(1);
                spdlog::error("market login error ");;
            }
        }

        void TAP_CDECL
        EESMarket::OnAPIReady() {
            spdlog::info("OnAPIReady market  initial finish");
            m_loginPromise.set_value(0);
        }

        int EESMarket::subscribeAllMarketData() {

            for (auto &itr: m_subScribeInstruments) {
                TapAPIContract stContract;
                memset(&stContract, 0, sizeof(stContract));
                strncpy(stContract.Commodity.ExchangeNo, "ZCE", sizeof(stContract.Commodity.ExchangeNo));
                if (itr.second->quoteType == Types::QuoteType::MarketData) {
                    stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
                    std::copy(std::begin(itr.first), std::begin(itr.first) + 2,
                              std::begin((stContract.Commodity.CommodityNo)));
                    std::copy(std::begin(itr.first) + 2, std::end(itr.first), std::begin((stContract.ContractNo1)));

                    stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
                    stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;
                    m_uiSessionID = 0;

                    m_pMdApi->SubscribeQuote(&m_uiSessionID, &stContract);
                    auto retCode = m_pMdApi->SubscribeQuote(&m_uiSessionID, &stContract);
                    if (retCode != 0) {
                        spdlog::error("subscribe {}{} mds return error errorid: {}", stContract.Commodity.CommodityNo,
                                      stContract.ContractNo1, retCode);
                    }
                } else if (itr.second->quoteType == Types::QuoteType::MonthMarket) {
                    stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_SPREAD_MONTH;
                    stContract.Commodity.CommodityNo[0] = itr.first[4];
                    stContract.Commodity.CommodityNo[1] = itr.first[5];
                    stContract.ContractNo1[0] = itr.first[6];
                    stContract.ContractNo1[1] = itr.first[7];
                    stContract.ContractNo1[2] = itr.first[8];

                    stContract.ContractNo2[0] = itr.first[12];
                    stContract.ContractNo2[1] = itr.first[13];
                    stContract.ContractNo2[2] = itr.first[14];


                    stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
                    stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;
                    m_uiSessionID = 0;

                    m_pMdApi->SubscribeQuote(&m_uiSessionID, &stContract);
                    auto retCode = m_pMdApi->SubscribeQuote(&m_uiSessionID, &stContract);
                    if (retCode != 0) {
                        spdlog::error("subscribe {}{} {} mds return error errorid: {}",
                                      stContract.Commodity.CommodityNo,
                                      stContract.ContractNo1, stContract.ContractNo2, retCode);
                    }
                } else if (itr.second->quoteType == Types::QuoteType::CrossMarket) {
                    stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_SPREAD_COMMODITY;
                    stContract.Commodity.CommodityNo[0] = itr.first[4];
                    stContract.Commodity.CommodityNo[1] = itr.first[5];
                    stContract.Commodity.CommodityNo[2] = itr.first[9];
                    stContract.Commodity.CommodityNo[3] = itr.first[10];
                    stContract.Commodity.CommodityNo[4] = itr.first[11];

                    stContract.ContractNo1[0] = itr.first[6];
                    stContract.ContractNo1[1] = itr.first[7];
                    stContract.ContractNo1[2] = itr.first[8];

                    stContract.ContractNo2[0] = itr.first[12];
                    stContract.ContractNo2[1] = itr.first[13];
                    stContract.ContractNo2[2] = itr.first[14];


                    stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
                    stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;
                    m_uiSessionID = 0;

                    m_pMdApi->SubscribeQuote(&m_uiSessionID, &stContract);
                    auto retCode = m_pMdApi->SubscribeQuote(&m_uiSessionID, &stContract);
                    if (retCode != 0) {
                        spdlog::error("subscribe {}{} {} mds return error errorid: {}",
                                      stContract.Commodity.CommodityNo,
                                      stContract.ContractNo1, stContract.ContractNo2, retCode);
                    }
                }

            }
            return 0;
        }


        void EESMarket::SubScribeQuote( Types::SubScribeQuote const &subScribeQuote) {
            fprintf(stderr, "EESMarket::SubScribeQuote %d %s\n", subScribeQuote.policyID, subScribeQuote.instrumentID.data());
             Types::Instrument_t instrument{""};
            strcpy(instrument.data(), subScribeQuote.instrumentID.data());

             Utils::TradingHours::initInstrumentTradingHours(instrument);
            auto itr = m_subScribeInstruments.find(instrument);
            if (itr == m_subScribeInstruments.end()) {
                 Types::PushMarket *pushMarket = new  Types::PushMarket(0);
                pushMarket->quoteType = subScribeQuote.quoteType;
                m_subScribeInstruments[instrument] = pushMarket;
            }
            m_subScribeInstruments[instrument]->subscribePolicy.emplace_back(subScribeQuote.policyID);
             Types::OnSubScribeQuote onSubScribeQuote;
            onSubScribeQuote.policyID = subScribeQuote.policyID;
            strcpy(onSubScribeQuote.instrumentID.data(), instrument.data());
//            auto itrSeries = m_kDataManager.m_allKLineSeries.find(onSubScribeQuote.instrumentID);
//            if (itrSeries == m_kDataManager.m_allKLineSeries.end()){
//                assert(false);
//            }
//            onSubScribeQuote.m_kSeriesMap = itrSeries->second;
            m_driver->send(onSubScribeQuote);
        }

        void TAP_CDECL
        EESMarket::OnRspSubscribeQuote(TAPIUINT32
                                       sessionID,
                                       TAPIINT32 errorCode, TAPIYNFLAG
                                       isLast,
                                       const TapAPIQuoteWhole *info
        ) {
            if (errorCode == 0) {
                spdlog::info("OnRspSubscribeQuote success {}. {}, {}", info->Contract.Commodity.CommodityNo, info->Contract.ContractNo1, info->Contract.ContractNo2);
            } else {
                spdlog::error("OnRspSubscribeQuote error={}, contract={}, {}, {}", errorCode, info->Contract.Commodity.CommodityNo, info->Contract.ContractNo1, info->Contract.ContractNo2);
            }

        }

        void TAP_CDECL
        EESMarket::OnRtnQuote(const TapAPIQuoteWhole *info) {
           // fprintf(stderr,"OnRtnQuote : productid=%s, updateTime=%s, instrument=%s, instrument2=%s, commodityType=%c\n",info->Contract.Commodity.CommodityNo,
           //			    info->DateTimeStamp, info->Contract.ContractNo1, info->Contract.ContractNo2, info->Contract.Commodity.CommodityType);
             Types::Instrument_t instemp{""};
            if (info->Contract.Commodity.CommodityType == TAPI_COMMODITY_TYPE_FUTURES) {
                sprintf(instemp.data(), "%s%s", info->Contract.Commodity.CommodityNo, info->Contract.ContractNo1);
            } else if (info->Contract.Commodity.CommodityType == TAPI_COMMODITY_TYPE_SPREAD_MONTH) {
                sprintf(instemp.data(), "SPD %s%s&%s%s", info->Contract.Commodity.CommodityNo,
                        info->Contract.ContractNo1, info->Contract.Commodity.CommodityNo, info->Contract.ContractNo2);
            } else if (info->Contract.Commodity.CommodityType == TAPI_COMMODITY_TYPE_SPREAD_COMMODITY) {
                instemp[0] = 'I';
                instemp[1] = 'P';
                instemp[2] = 'S';
                instemp[3] = ' ';
                instemp[4] = info->Contract.Commodity.CommodityNo[0];
                instemp[5] = info->Contract.Commodity.CommodityNo[1];
                instemp[6] = info->Contract.ContractNo1[0];
                instemp[7] = info->Contract.ContractNo1[1];
                instemp[8] = info->Contract.ContractNo1[2];
                instemp[9] = '&';
                instemp[10] = info->Contract.Commodity.CommodityNo[3];
                instemp[11] = info->Contract.Commodity.CommodityNo[4];
                instemp[12] = info->Contract.ContractNo2[0];
                instemp[13] = info->Contract.ContractNo2[1];
                instemp[14] = info->Contract.ContractNo2[2];
            }

            auto itr = m_subScribeInstruments.find(instemp);
            auto event = itr->second->eventDataList.getNewMemory();
            auto marketData = itr->second->marketDataList.getNewMemory();
            marketData->epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();

            marketData->lastPrice = info->QLastPrice;
            marketData->upperLimitPrice = info->QLimitUpPrice;
            marketData->lowerLimitPrice = info->QLimitDownPrice;
            marketData->highestPrice = info->QHighPrice;
            marketData->lowestPrice = info->QLowPrice;
            marketData->openPrice = info->QOpeningPrice;
            if (marketData->openPrice > 999999999.9) {
                marketData->highestPrice = marketData->lastPrice;
                marketData->lowestPrice = marketData->lastPrice;
                marketData->openPrice = marketData->lastPrice;
            }

            marketData->bidVolume[0] = info->QBidQty[0];
            marketData->askVolume[0] = info->QAskQty[0];
            marketData->bidPrice[0] = info->QBidPrice[0];
            marketData->askPrice[0] = info->QAskPrice[0];
            if (marketData->bidVolume[0] == 0) {
                marketData->bidPrice[0] = marketData->lastPrice;
            }
            if (marketData->askVolume[0] == 0) {
                marketData->askPrice[0] = marketData->lastPrice;
            }

            strcpy(marketData->instrumentID.data(), instemp.data());


            //            std::copy(std::begin(pDepthMarketData->TradingDay), std::begin(pDepthMarketData->TradingDay) + marketData->tradingDay.size(), std::begin(marketData->tradingDay));




            for (auto i = 0; i < 8; i++) {
                marketData->updateTime[i] = info->DateTimeStamp[11 + i];
            }
            marketData->milliSeconds = (info->DateTimeStamp[20] - '0') * 100;


            marketData->psSecond =  Utils::ToPsSeconds(marketData->updateTime);

            marketData->midPrice = (marketData->bidPrice[0] + marketData->askPrice[0]) * 0.5;

            marketData->amount = info->QTotalTurnover;
            marketData->volume = info->QTotalQty;
            marketData->oi = info->QPositionQty;
          /*  if ( Utils::FTTrait::FT_NOT_TRADING ==
                 Utils::TradingHours::getProductTrait(marketData->instrumentID, marketData->psSecond)) {
		 fprintf(stderr,"OnRtnQuote  noTradeingHours : productid=%s, instrument=%s, instrument2=%s, commodityType=%c\n",info->Contract.Commodity.CommodityNo,
                         info->Contract.ContractNo1, info->Contract.ContractNo2, info->Contract.Commodity.CommodityType);

                return;
            }*/

//            for (auto period :  Types::m_kperoidVec) {
//                auto series = m_kDataManager.getSeries(marketData->instrumentID, period);
//                series->addTick(marketData);
//            }
            //  fprintf(stderr, "OnRtnDepthMarketData instrumentid=%s, updateTime=%s.%d, volume=%d\n",marketData->instrumentID.data(), marketData->updateTime.data(), marketData->milliSeconds, marketData->volume);
            event->point = marketData;
            event->type = 0;
            for (auto i: itr->second->subscribePolicy) {
                if (marketData == nullptr) {
                    assert(false && "market is nullptr");
                }

            //    fprintf(stderr, "instrumentID=%s, threadID=%d, size=%d\n", marketData->instrumentID.data(), i,itr->second->subscribePolicy.size());
                m_driver->callback_asyncEventData(event, i);
            }
        }



    }
}
