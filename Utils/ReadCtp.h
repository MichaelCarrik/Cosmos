//
// Created by zhangyw on 1/11/21.
//

#ifndef TRENDFOLLOW_READCTP_H
#define TRENDFOLLOW_READCTP_H

#include "../Types/Type.h"
#include "../Types/MarketData.h"
#include <experimental/filesystem>
#include "Utils.h"

namespace TrendFollow{
    namespace ReadCtp{
        int read_tick(char* path, std::vector< Types::MarketData>& tickVector ) {
            //   TradeBots::Types::MarketData marketdata;
            std::ifstream file;
            std::string strLine;


            file.open(path);
            std::string separator = ",";
            int i = 0;
            while (getline(file, strLine)) {
                if (strLine.empty()) {
                    continue;
                }
                if (i > 0) {
                    unsigned int start = 0;
                    auto index = strLine.find_first_of(separator, start);
                    std::vector <std::string> line_vector;
                    int countsRows = 0;
                    do {
                        countsRows++;
                        if (index != std::string::npos) {
                            auto substring = strLine.substr(start, index - start);
                            line_vector.emplace_back(substring);
                            start = index + separator.size();
                            index = strLine.find(separator, start);

                            if (start == std::string::npos) {
                                break;
                            }

                        }else{
                            line_vector.emplace_back(strLine.substr(start, strLine.length()- start));
                            break;
                        }
                    } while (true);
                     Types::MarketData marketData;

                    //      std::copy(line_vector[0].begin(), line_vector[0].end(), marketData.tradingDay.begin());
                    std::copy(line_vector[1].begin(), line_vector[1].end(),  marketData.instrumentID.begin());
                    //  marketData.instrumentID = m_subscribes[0];

                    marketData.lastPrice = atof(line_vector[4].c_str());

                    marketData.askPrice[0] = atof(line_vector[24].c_str());
                    marketData.bidPrice[0] = atof(line_vector[22].c_str());
                    marketData.askVolume[0] = atof(line_vector[25].c_str());
                    marketData.bidVolume[0] = atof(line_vector[23].c_str());

                    marketData.oi = atof(line_vector[13].c_str());
                    marketData.volume = atof(line_vector[11].c_str());
                    marketData.amount = atof(line_vector[12].c_str());
                    std::copy(line_vector[20].begin(), line_vector[20].end(), marketData.updateTime.begin());
                    marketData.milliSeconds = atof(line_vector[21].c_str());

                    marketData.psSecond =  Utils::ToPsSeconds(marketData.updateTime);
                    //   printf("this line end: %s.%d", marketData.updateTime.data(), sortMarket.marketData.milliSeconds);

                    tickVector.emplace_back(marketData);
                    //       this->onRtnQuote(&marketdata);


                }
                i++;
            }
            return 0;

        }
    }
}



#endif //TRENDFOLLOW_READCTP_H
