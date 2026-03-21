//
// Created by zhangyw on 6/10/20.
//
//
// Created by zhangyw on 6/13/19.
//

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "MockMarket.h"
#include <algorithm>

#include "../Utils/Utils.h"

namespace TrendFollow {
    namespace Market {
        MockMarket::MockMarket(decltype(m_driver) driver,
                               std::string &rawTickPath) :
                m_driver(driver),   m_rawTickPath(rawTickPath) {

        //    m_kDataManager.m_mysql = mySql;
        }


        void MockMarket::SubScribeQuote( Types::SubScribeQuote const &subScribeQuote) {
//            fprintf(stderr, "MockMarket::SubScribeQuote %d %s\n", subScribeQuote.policyID,
//                    subScribeQuote.instrumentID.data());
             Types::Instrument_t instrument{""};
            strcpy(instrument.data(), subScribeQuote.instrumentID.data());

            //    strcpy(instrument.data() , "ag2102");

             Utils::TradingHours::initInstrumentTradingHours(instrument);
            auto itr = m_subScribeInstruments.find(instrument);
            if (itr == m_subScribeInstruments.end()) {
                Types::PushMarket *pushMarket = new Types::PushMarket(0);

                m_subScribeInstruments[instrument] = pushMarket;
            }

            m_subScribeInstruments[instrument]->subscribePolicy.emplace_back(subScribeQuote.policyID);

            Types::OnSubScribeQuote onSubScribeQuote;
            strcpy(onSubScribeQuote.instrumentID.data(), subScribeQuote.instrumentID.data());
            onSubScribeQuote.policyID = subScribeQuote.policyID;
//            auto itrSeries = m_kDataManager.m_allKLineSeries.find(onSubScribeQuote.instrumentID);
//            if (itrSeries == m_kDataManager.m_allKLineSeries.end()) {
//                assert(false);
//            }
//            onSubScribeQuote.m_kSeriesMap = itrSeries->second;
            m_driver->send(onSubScribeQuote);
        };

        void MockMarket::onRtnQuote(const Types::MarketData *marketData) {
            if( Utils::FTTrait::FT_NOT_TRADING ==  Utils::TradingHours::getProductTrait(marketData->instrumentID, marketData->psSecond)){
                return;
            }
        //    fprintf(stderr,"MockMarket instrument=%s, updateTime=%s\n", marketData->instrumentID.data(), marketData->updateTime.data());
            auto itr = m_subScribeInstruments.find(marketData->instrumentID);
            if (itr != m_subScribeInstruments.end()) {

                auto event = itr->second->eventDataList.getNewMemory();
                event->point = marketData;
                event->type = 0;
                for (auto i : itr->second->subscribePolicy) {
                    if (marketData == nullptr) {
                        assert(false && "market is nullptr");
                    }
                    m_driver->callback_asyncEventData(event, i);
        //            m_driver->send(*marketData);
                }
            }
        }





        int MockMarket::start(int tradingday, bool isDay) {

            std::string dayOrNigh = isDay == true ? "day" : "ngt";


            std::vector<Types::MarketData> allSymbolMarket;
            for (auto &itr : m_subScribeInstruments) {
//                if (strcmp(instrument.data(), "ag2102") !=0){
//                    continue;
//                }
                read_tick(itr.first, tradingday, dayOrNigh, allSymbolMarket);
                //    read_tick(instrument, tradingday, "ngt", allSymbolMarket );
            }
            std::sort(std::begin(allSymbolMarket), std::end(allSymbolMarket),
                      [](auto &a, auto &b) {
                          if (a.psSecond != b. psSecond){
                              return  a.psSecond < b. psSecond;
                          }

                          else {
                              return a.epoch_time < b.epoch_time;
                          }

                      });

            for (auto &marketData : allSymbolMarket) {
//                fprintf(stderr, "mockMarket sendMarket : %s %s %d\n", marketData.instrumentID.data(),
//                        marketData.updateTime.data(), marketData.milliSeconds);
                this->onRtnQuote(&marketData);
            }
            allSymbolMarket.clear();

            return 0;
        }

        int MockMarket::read_tick(Types::Instrument_t const &instrument, int tradingday, std::string &dayOrNight,
                                  std::vector<Types::MarketData> &allSymbolMarket) {

            std::ifstream file;
            std::string strLine;

            if (strcmp(instrument.data(),"MO2606P9600") ==0) {
                int a = 1;
            }

            char read_path[256]{""};
            sprintf(read_path, "%s/%d_%s/instruments/%s.txt", m_rawTickPath.c_str(), tradingday, dayOrNight.c_str(),
                    instrument.data());
            std::filesystem::path filePath(read_path);
            if(!std::filesystem::exists(filePath)) {
                memset(read_path, 256, sizeof(char));
                Types::Instrument_t underly{""};
                char optionType{'N'};
                double strikePrice{0.0};
                 Utils::parseInstruemnt(instrument, underly, optionType, strikePrice);
                if(underly[0] == 'I' && underly[1] == 'F'){
                    underly[1] = 'O';
                }else  if(underly[0] == 'I' && underly[1] == 'H'){
                    underly[0] = 'H';
                    underly[1] = 'O';
                }else  if(underly[0] == 'I' && underly[1] == 'M'){
                    underly[0] = 'M';
                    underly[1] = 'O';
                }
                sprintf(read_path, "%s/%d_%s/instruments/%s-%c-%d.txt", m_rawTickPath.c_str(), tradingday, dayOrNight.c_str(),
                        underly.data(), optionType, int(strikePrice));
            }
            file.open(read_path);
            std::string separator = ",";
            int i = 0;
            while (getline(file, strLine)) {
                if (strLine.empty()) {
                    continue;
                }
                if (i >= 1) {
                    unsigned int start = 0;
                    auto index = strLine.find_first_of(separator, start);
                    std::vector<std::string> line_vector;
                    std::string substring = "";
                    do {

                        if (index != std::string::npos) {
                            substring = strLine.substr(start, index - start);

                            line_vector.emplace_back(substring);
                            start = index + separator.size();
                            index = strLine.find(separator, start);


                            if (start == std::string::npos) {
                                break;
                            }

                        }
                    } while (index != std::string::npos);
                    line_vector.emplace_back(strLine.substr(start, index - start));
                    if (line_vector.size() < 45 ) {
                        continue;
                    }
                    Types::MarketData marketData;
                    line_vector[1].erase(std::remove(line_vector[1].begin(), line_vector[1].end(), '-'), line_vector[1].end());
                    strcpy(marketData.instrumentID.data(), line_vector[1].c_str());
                    marketData.openPrice = atof(line_vector[8].c_str());
                    marketData.highestPrice = atof(line_vector[9].c_str());
                    marketData.lowestPrice = atof(line_vector[10].c_str());

                    marketData.lastPrice = atof(line_vector[4].c_str());
                    marketData.upperLimitPrice = atof(line_vector[16].c_str());
                    marketData.lowerLimitPrice = atof(line_vector[17].c_str());
                    marketData.askPrice[0] = atof(line_vector[24].c_str());
                    marketData.bidPrice[0] = atof(line_vector[22].c_str());
                    marketData.askVolume[0] = atof(line_vector[25].c_str());
                    marketData.bidVolume[0] = atof(line_vector[23].c_str());

                    marketData.volume = atoi(line_vector[11].c_str());
                    marketData.amount = atof(line_vector[12].c_str());
                    marketData.oi = atof(line_vector[13].c_str());
                    marketData.settlementPrice = atof(line_vector[15].c_str());

                    std::copy(line_vector[20].begin(), line_vector[20].end(), marketData.updateTime.begin());
                    marketData.milliSeconds = atof(line_vector[21].c_str());
      //             fprintf(stderr, "updateTime=%s\n", marketData.updateTime.data());

                    marketData.psSecond = Utils::ToPsSeconds(marketData.updateTime);
                    marketData.midPrice = (marketData.bidPrice[0] + marketData.askPrice[0]) * 0.5;
//                    if((marketData.settlementPrice > 0  &&    marketData.settlementPrice< 9999) ||   (marketData.psSecond > 54000 && marketData.psSecond < 58000 ) ){
//                        int a =1;
//                    }
                    if (marketData.openPrice > 999999999.9) {
                        marketData.openPrice = marketData.lastPrice;
                        marketData.highestPrice = marketData.lastPrice;
                        marketData.lowestPrice = marketData.lastPrice;
                    }

                    marketData.epoch_time = atol(line_vector[45].c_str());

                    allSymbolMarket.emplace_back(marketData);
                }
                i++;
            }
            return 0;
        }

    }
}

#include "MockMarket.h"
