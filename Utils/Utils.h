//
// Created by zhangyw on 1/10/21.
//

#ifndef Cosmos_UTILS_H
#define Cosmos_UTILS_H

#include <fstream>
#include <stdlib.h>
#include "../dependencies/sdk/ctp/v6.6.9/ThostFtdcTraderApi.h"
#include "../Types/Type.h"
#include "../Types/MarketData.h"
#include "../Types/InstrumentInfo.h"
#include "../Types/OrderField.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <variant>
#include <boost/range/iterator_range_core.hpp>
#include "cppmysql.h"


namespace Cosmos {
    namespace Utils {
        static Types::ExchangeType getExchangeType(const char *inputExchange) {
            if (strcmp(inputExchange, "CFFEX") == 0) {
                return Types::ExchangeType::CFFEX;
            } else if (strcmp(inputExchange, "SHFE") == 0) {
                return Types::ExchangeType::SHFE;
            } else if (strcmp(inputExchange, "INE") == 0) {
                return Types::ExchangeType::INE;
            } else if (strcmp(inputExchange, "CZCE") == 0) {
                return Types::ExchangeType::ZCE;
            } else if (strcmp(inputExchange, "DCE") == 0) {
                return Types::ExchangeType::DCE;
            } else if (strcmp(inputExchange, "GFEX") == 0) {
                return Types::ExchangeType::GFE;
            }
            assert(false);
        };

        template<class T>
        static int ToPsSeconds(T &updateTime, bool isFormatNight = true) {
            int h, m, s;
            h = (updateTime[0] - '0') * 10 + (updateTime[1] - '0');
            m = (updateTime[3] - '0') * 10 + (updateTime[4] - '0');
            s = (updateTime[6] - '0') * 10 + (updateTime[7] - '0');
            if (isFormatNight == true) {
                if (h >= 20) {
                    h -= 21;
                } else if (h <= 3) {
                    h += 3;
                }
            }


            return (h * 3600 + m * 60 + s);
        }

        static void ToUpdateTime(int psTime, Types::UpdateTime_t &updateTime) {
            int hour = psTime / 3600;
            int minutes = (psTime % 3600) / 60;
            int seconds = (psTime % 3600) % 60;
            sprintf(updateTime.data(), "%02d:%02d:%02d", hour, minutes, seconds);
        }

        static bool isSTGInstrument(const char *instrument) {
            if ((instrument[0] == 'S' && instrument[1] == 'T' & instrument[2] == 'G') ||
                (instrument[0] == 'P' && instrument[1] == 'R' & instrument[2] == 'T')) {
                return true;
            }
            return false;
        }

        static void InstrumentToProduct(Types::Instrument_t const &instrument, Types::Product_t &productId) {

                for (auto i = 0; i < 5; i++) {
                    if (instrument[i] >= '0' && instrument[i] <= '9') {
                        std::copy(std::begin(instrument), std::begin(instrument) + i, std::begin(productId));
                        break;
                    }
                    if (i == 5) {
                        assert(false && "InstrumentToProduct");
                    }
                }
        }

        static void parseInstruemnt(Types::Instrument_t const &instrument, Types::Instrument_t &underly,
                                    char &optionType, double &strickPrice) {
            if (instrument[5] == 'C' or instrument[5] == 'P') {
                std::copy(std::begin(instrument), std::begin(instrument) + 5, std::begin(underly));
                optionType = instrument[5];
                Types::Instrument_t strickStr{""};
                std::copy(std::begin(instrument) + 6, std::end(instrument), std::begin(strickStr));
                strickPrice = atof(strickStr.data());
            } else {
                std::copy(std::begin(instrument), std::begin(instrument) + 6, std::begin(underly));
                optionType = instrument[6];
                Types::Instrument_t strickStr{""};
                std::copy(std::begin(instrument) + 7, std::end(instrument), std::begin(strickStr));
                strickPrice = atof(strickStr.data());
                Types::Product_t productId{""};
                InstrumentToProduct(instrument, productId);
                if (strcmp(productId.data(), "IO") == 0) {
                    underly[0] = 'I';
                    underly[1] = 'F';
                } else if (strcmp(productId.data(), "HO") == 0) {
                    underly[0] = 'I';
                    underly[1] = 'H';
                } else if (strcmp(productId.data(), "MO") == 0) {
                    underly[0] = 'I';
                    underly[1] = 'M';
                }
            }
        }

        static bool checkTerminal(Types::OrderField *orderField) {
            if (orderField->orderStatus == Types::OrderStatus::allTraded ||
                orderField->orderStatus == Types::OrderStatus::canceled ||
                orderField->orderStatus == Types::OrderStatus::failed) {
                return true;
            }
            return false;
        }

        static bool updateOrder(Types::OrderField *orderField, const Types::OrderField *inputOrder) {
            strcpy(orderField->orderSysID.data(), inputOrder->orderSysID.data());
            orderField->lastFilledPrice = inputOrder->lastFilledPrice;
            orderField->lastFilledVolume = inputOrder->lastFilledVolume;
            orderField->filledVolume = inputOrder->filledVolume;
            // cancel order cause failed, may by not cancel succcess
            if (inputOrder->orderStatus == Types::OrderStatus::failed && orderField->orderStatus !=
                Types::OrderStatus::signal) {
            } else {
                orderField->orderStatus = inputOrder->orderStatus;
            }

            orderField->tOrderID = inputOrder->tOrderID;
            orderField->epoch_time = inputOrder->epoch_time;
            orderField->isTerminal = checkTerminal(orderField);

            return true;
        }

        static double checkPriceValid(double price) {
            if (price < 9999999999999.0 && price > -9999999999999.0) {
                return price;
            }
            return 0.0;
        }

        static bool checkTerminal(Types::OrderStatus const &orderStatus) {
            if (orderStatus == Types::OrderStatus::allTraded ||
                orderStatus == Types::OrderStatus::canceled ||
                orderStatus == Types::OrderStatus::failed) {
                return true;
            }
            return false;
        }

        static void updateOrderStatus(Types::OrderStatus &orderStatus,
                                      Types::OrderStatus const &inputorderStatus) {
            if (inputorderStatus == Types::OrderStatus::failed &&
                orderStatus != Types::OrderStatus::signal) {
            } else {
                orderStatus = inputorderStatus;
            }
        }

        static bool is_day(void) {
            auto current_time = std::time(nullptr);
            auto ptm = std::localtime(&current_time);

            return ptm->tm_hour >= 6 && ptm->tm_hour < 18;
        }

        static std::string GetLastValueFromFile(char *filename, const char *valuename, const char *instrument) {
            char buf[BUFSIZ], *field;
            std::string value{""};
            std::string lastLine{""};
            FILE *fp = NULL;
            fp = fopen(filename, "r");
            if (fp == NULL) {
                printf("OnStarted, Warning: Cannot open file: %s !!!\n", filename);
                return "0";
            } else {
                while (fgets(buf, BUFSIZ, fp) != NULL) {
                    //  printf("buf : %s",buf);
                    field = strstr(buf, instrument);
                    if (field) {
                        lastLine = field;
                    }
                }

                char lastLine_char[512]{""};
                strcpy(lastLine_char, lastLine.c_str());
                field = strstr(lastLine_char, valuename);
                if (field == nullptr) {
                    return "0.0";
                }
                std::string substr = field + strlen(valuename) + 1;
                auto index = substr.find_first_of(',', 0);
                if (index != std::string::npos) {
                    value = substr.substr(0, index);
                } else {
                    value = substr;
                }
            }
            fclose(fp);
            return value;
        }

        static void readInstrumentsFromFiles(int tradingday, Types::Instrument_t const &filterUnderly,
                                             std::string &rawPath, std::vector<Types::InstrumentInfo> &testSymbols,
                                             int isDay) {
            char buff[256]{""};
            sprintf(buff, "%s/%d_%s/instruments", rawPath.c_str(), tradingday, isDay == true ? "day" : "ngt");
            std::filesystem::path filePath(buff);
            if (!std::filesystem::exists(filePath)) {
                fprintf(stderr, "path is not exists %s\n", filePath.c_str());
                return;
            }
            char insInfoBuff[256]{""};
            sprintf(insInfoBuff, "%s/%d_%s/instrument_info.csv", rawPath.c_str(), tradingday,
                    isDay == true ? "day" : "ngt");
            std::ifstream file;
            std::string strLine;
            file.open(insInfoBuff);
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
                    if (line_vector.size() < 20 ) {
                        continue;
                            }
                    Types::InstrumentInfo instrumentInfo;
                    //   fprintf(stderr, "%s\n", line_vector[1].c_str());
                    line_vector[1].erase(std::remove(line_vector[1].begin(), line_vector[1].end(), '-'),
                                         line_vector[1].end());
                    strcpy(instrumentInfo.instrumentID.data(), line_vector[1].c_str());
                    // if (strcmp(instrumentInfo.instrumentID.data(), "i2405") ==0) {
                    //     fprintf(stderr,"instrument=%s\n",instrumentInfo.instrumentID.data());
                    // }
                    instrumentInfo.multi = atof(line_vector[13].c_str());
                    instrumentInfo.tickSize = atof(line_vector[14].c_str());
                    int PIC = atoi(line_vector[6].c_str()) ;
                    if (PIC == 1) {
                        instrumentInfo.productIDClass = Types::ProductClass::future ;
                    }else if (PIC == 2 || PIC == 6) {
                        instrumentInfo.productIDClass = Types::ProductClass::option ;
                    }else {
                        continue;
                    }

                    instrumentInfo.expireDate = atoi(line_vector[17].c_str());
                    Utils::InstrumentToProduct(instrumentInfo.instrumentID, instrumentInfo.productID);
                    Utils::parseInstruemnt(instrumentInfo.instrumentID, instrumentInfo.underly,
                                           instrumentInfo.optionType, instrumentInfo.strikePrice);

                    if (strcmp(instrumentInfo.instrumentID.data(), filterUnderly.data()) == 0){

                        testSymbols.emplace_back(instrumentInfo);
                    }
                    else if (strcmp(instrumentInfo.underly.data(), filterUnderly.data()) == 0) {
                      //  fprintf(stderr,"instrument=%s\n",instrumentInfo.instrumentID.data());
                        testSymbols.emplace_back(instrumentInfo);
                    }
                }
                i++;
            }
        }

        static void convertToMarketDaTa(const CThostFtdcDepthMarketDataField *pDepthMarketData,
                                        Types::MarketData *marketData) {
            marketData->lastPrice = pDepthMarketData->LastPrice;
            marketData->upperLimitPrice = pDepthMarketData->UpperLimitPrice;
            marketData->lowerLimitPrice = pDepthMarketData->LowerLimitPrice;
            marketData->settlementPrice = pDepthMarketData->SettlementPrice;
            marketData->highestPrice = pDepthMarketData->HighestPrice;
            marketData->lowestPrice = pDepthMarketData->LowestPrice;
            marketData->openPrice = pDepthMarketData->OpenPrice;

            marketData->lastPrice = Utils::checkPriceValid(marketData->lastPrice);
            if (marketData->openPrice > 999999999.9) {
                marketData->highestPrice = marketData->lastPrice;
                marketData->lowestPrice = marketData->lastPrice;
                marketData->openPrice = marketData->lastPrice;
            }

            marketData->bidPrice[0] = pDepthMarketData->BidPrice1;
            marketData->askPrice[0] = pDepthMarketData->AskPrice1;
            marketData->midPrice = (marketData->bidPrice[0] + marketData->askPrice[0]) * 0.5;
            if (pDepthMarketData->BidVolume1 == 0) {
                marketData->bidPrice[0] = marketData->lastPrice;
                marketData->midPrice = marketData->lastPrice;
            }
            if (pDepthMarketData->AskVolume1 == 0) {
                marketData->askPrice[0] = marketData->lastPrice;
                marketData->midPrice = marketData->lastPrice;
            }
            std::copy(std::begin(pDepthMarketData->InstrumentID),
                      std::begin(pDepthMarketData->InstrumentID) + marketData->instrumentID.size(),
                      std::begin(marketData->instrumentID));

            marketData->bidVolume[0] = pDepthMarketData->BidVolume1;
            marketData->askVolume[0] = pDepthMarketData->AskVolume1;
            std::copy(std::begin(pDepthMarketData->UpdateTime),
                      std::begin(pDepthMarketData->UpdateTime) + marketData->updateTime.size(),
                      std::begin(marketData->updateTime));

            marketData->milliSeconds = pDepthMarketData->UpdateMillisec;

            marketData->psSecond = Utils::ToPsSeconds(marketData->updateTime);

            marketData->amount = checkPriceValid(pDepthMarketData->Turnover);
            marketData->volume = pDepthMarketData->Volume;
            marketData->oi = checkPriceValid(pDepthMarketData->OpenInterest);
        }

        static std::string getParamMapValue(std::map<std::string, std::string> const& paramsMap, std::string&& key){
            auto itr = paramsMap.find(key);
            if (itr == paramsMap.end()){
                assert(false);
            }
            return itr->second;
        };



        static void InitMySql( Utils::CppMySQL3DB *mySql, std::string mysql_config) {
            boost::property_tree::ptree pt;
            boost::property_tree::read_xml(mysql_config, pt);
            auto host = pt.get_child("mysql.host").get_value<std::string>();
            auto port = pt.get_child("mysql.port").get_value<int>();
            auto dbname = pt.get_child("mysql.database").get_value<std::string>();
            auto username = pt.get_child("mysql.user").get_value<std::string>();
            auto password = pt.get_child("mysql.password").get_value<std::string>();

            int nTried = 0;
            int retCode = -1;
            try {
                do {
                    retCode = mySql->open(host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port);
                    if (retCode < 0) {
                        const char *errorMsg = mysql_error(mySql->getMysql());
                        int errorNo = mysql_errno(mySql->getMysql());
                        spdlog::error("failed to connect mysql db. please check settings,error no:{},error msg:{}",
                                      errorNo, errorMsg);
                        sleep(2);
                    }
                } while (nTried++ < 2 && retCode < 0);
                if (nTried > 2 and retCode != 0) {
                    assert(false && "connect mysql failed");
                }
            }
            catch (...) {
                spdlog::error("got exception when opening the mysql db.");
            }
        }
    }
}


#endif //Cosmos_UTILS_H
