//
// Created by zhangyw on 1/10/21.
//

#ifndef Cosmos_UTILS_H
#define Cosmos_UTILS_H

#include <fstream>
#include <stdlib.h>

#include "../Types/Type.h"
#include "../Types/MarketData.h"
#include "../Types/InstrumentInfo.h"
#include "../Types/OrderField.h"
#include "../Types/ArbitrageOrderField.h"


namespace Cosmos {
    namespace Utils {
        static Types::ExchangeType getExchangeType(const char *inputExchange) {
            if (strcmp(inputExchange, "CFFEX") == 0) {
                return Types::ExchangeType::CFFEX;
            } else if (strcmp(inputExchange, "SHFE") == 0) {
                return Types::ExchangeType::SHFE;
            }
            else if (strcmp(inputExchange, "INE") == 0) {
                return Types::ExchangeType::INE;
            }
            else if (strcmp(inputExchange, "CZCE") == 0) {
                return Types::ExchangeType::ZCE;
            }
            else if (strcmp(inputExchange, "DCE") == 0) {
                return Types::ExchangeType::DCE;
            }
            else if (strcmp(inputExchange, "GFEX") == 0) {
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

        static bool isAbtrgInstrument(const char *instrument) {
            if ((instrument[0] == 'S' && instrument[1] == 'P') || (instrument[0] == 'I' && instrument[1] == 'P')) {
                return true;
            }
            return false;
        }

        static bool isSTGInstrument(const char *instrument) {
            if ((instrument[0] == 'S' && instrument[1] == 'T' & instrument[2] == 'G') ||
                (instrument[0] == 'P' && instrument[1] == 'R' & instrument[2] == 'T')) {
                return true;
            }
            return false;
        }


        static void InstrumentToProduct(Types::Instrument_t const &instrument, Types::Product_t &productId) {
            if (Utils::isAbtrgInstrument(instrument.data()) == true) {
                int p = 0;
                for (auto i = 0; i < instrument.size() || instrument[i] == '\0'; i++) {
                    if (instrument[i] < '0' || instrument[i] > '9') {
                        productId[p] = instrument[i];
                        p++;
                    }
                }
            } else {
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


        template<class T>
        static void logOrder(const Types::OrderField *orderField, std::shared_ptr<spdlog::logger> m_orderLog,
                             const T *symbol, const Types::OrderStatus orderStatus, int tradingday,
                             const int64_t epoch_time) {
            auto log_epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();


            m_orderLog->info(
                "{}, {}, {}, {:03d}, {}, {}, {}, {}, {}, {}, {}, {:.3f}, {}, {:.3f}, {}, {}, delta={}, {:.3f}, {}, {}, {}",
                orderField->instrumentID.data(), tradingday,
                symbol->lastMD->updateTime.data(), symbol->lastMD->milliSeconds,
                Types::orderStatusMap[orderStatus].data(),
                orderField->pOrderID, orderField->tOrderID,
                Types::positionEffectMap[orderField->pet].data(),
                Types::orderSideMap[orderField->orderSide].data(),
                Types::intensionMap[orderField->intension].data(),
                Types::hedgeMap[orderField->hedgeType].data(),
                orderField->orderPrice, orderField->orderVolume, orderField->lastFilledPrice,
                orderField->lastFilledVolume, orderField->filledVolume, symbol->tradePosition.filledPosition,
                symbol->tradePosition.averagePrice,
                orderField->orderSysID.data(), epoch_time, log_epoch_time - epoch_time);
        }

        static std::shared_ptr<spdlog::logger> initOrderLog(std::string &engineName) {
            char recordFileNames[128];
            char recordF[128];
            sprintf(recordFileNames, "logs/record/%s.txt", engineName.c_str());
            std::filesystem::path path(recordFileNames);
            if (!std::filesystem::exists(path.parent_path())) {
                std::filesystem::create_directories(path.parent_path());
            }

            sprintf(recordF, "record_%s", engineName.c_str());
            return spdlog::basic_logger_st<spdlog::synchronous_factory>(recordF, recordFileNames);
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

        static bool updateArbitrageOrder(Types::ArbitrageOrderField *abtrgOrderField,
                                         const Types::ArbitrageOrderField *abtrgInputOrder) {
            strcpy(abtrgOrderField->orderSysID.data(), abtrgInputOrder->orderSysID.data());
            abtrgOrderField->lastFilledPrice[0] = abtrgInputOrder->lastFilledPrice[0];
            abtrgOrderField->lastFilledPrice[1] = abtrgInputOrder->lastFilledPrice[1];
            abtrgOrderField->lastFilledVolume[0] = abtrgInputOrder->lastFilledVolume[0];
            abtrgOrderField->lastFilledVolume[1] = abtrgInputOrder->lastFilledVolume[1];
            abtrgOrderField->filledVolume[0] = abtrgInputOrder->filledVolume[0];
            abtrgOrderField->filledVolume[1] = abtrgInputOrder->filledVolume[1];
            // cancel order cause failed, may by not cancel succcess
            Utils::updateOrderStatus(abtrgOrderField->orderStatus, abtrgInputOrder->orderStatus);

            abtrgOrderField->tOrderID = abtrgInputOrder->tOrderID;
            abtrgOrderField->epoch_time = abtrgInputOrder->epoch_time;
            abtrgOrderField->isTerminal = checkTerminal(abtrgOrderField->orderStatus);
            return true;
        }

        template<class T>
        static void logAbtrgOrder(const Types::ArbitrageOrderField *abtrgorderField,
                                  std::shared_ptr<spdlog::logger> m_orderLog,
                                  T *abtrgPolicyEvent,
                                  const Types::OrderStatus orderStatus, int tradingday,
                                  const int64_t epoch_time) {
            auto log_epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();


            m_orderLog->info(
                "{} {}&{}, {}, {} {:03d} {} {:03d}, {}, {}, {}, {}, {} {}, {} {}, {}, {:.3f}, {}, {:.3f} {:.3f}, {} {}, {} {}, delta={}, {:.3f}, {}, {}, {}",
                abtrgorderField->orderPrex.data(), abtrgorderField->instrumentIDs[0].data(),
                abtrgorderField->instrumentIDs[1].data(),
                tradingday, abtrgPolicyEvent->hedgeMD->updateTime.data(), abtrgPolicyEvent->hedgeMD->milliSeconds,
                abtrgPolicyEvent->tradeMD->updateTime.data(), abtrgPolicyEvent->tradeMD->milliSeconds,
                Types::orderStatusMap[orderStatus].data(),
                abtrgorderField->subPolicyID, abtrgorderField->pOrderID, abtrgorderField->tOrderID,
                Types::positionEffectMap[abtrgorderField->pets[0]].data(),
                Types::positionEffectMap[abtrgorderField->pets[1]].data(),
                Types::orderSideMap[abtrgorderField->orderSides[0]].data(),
                Types::orderSideMap[abtrgorderField->orderSides[1]].data(),
                Types::intensionMap[abtrgorderField->intension].data(),
                abtrgorderField->orderPrice, abtrgorderField->orderVolume, abtrgorderField->lastFilledPrice[0],
                abtrgorderField->lastFilledPrice[1],
                abtrgorderField->lastFilledVolume[0], abtrgorderField->lastFilledVolume[1],
                abtrgorderField->filledVolume[0], abtrgorderField->filledVolume[1],
                abtrgPolicyEvent->hedgeSymbol->tradePosition.filledPosition,
                abtrgPolicyEvent->hedgeSymbol->tradePosition.averagePrice,
                abtrgorderField->orderSysID.data(), epoch_time, log_epoch_time - epoch_time);

            //            m_orderLog->info(
            //                    "{}, {}, {}, {:03d}, {}, {}, {}, {}, {}, {}, {}, {:.3f}, {}, {:.3f}, {}, {}, delta={}, {:.3f}, {}, {}, {}",
            //                    abtrgorderField->instrumentIDs[1].data(), tradingday,
            //                    marketData->updateTime.data(), marketData->milliSeconds,
            //                    Types::orderStatusMap[orderStatus].data(),
            //                    abtrgorderField->subPolicyID, abtrgorderField->pOrderID, abtrgorderField->tOrderID,
            //                    Types::positionEffectMap[abtrgorderField->pets[1]].data(),
            //                    Types::orderSideMap[abtrgorderField->orderSides[1]].data(),
            //                    Types::intensionMap[abtrgorderField->intension].data(),
            //                    abtrgorderField->orderPrice, abtrgorderField->orderVolume, abtrgorderField->lastFilledPrice[1],
            //                    abtrgorderField->lastFilledVolume[1], abtrgorderField->filledVolume[1],
            //                    tradeSymbol->tradePosition.filledPosition,
            //                    tradeSymbol->tradePosition.averagePrice,
            //                    abtrgorderField->orderSysID.data(), epoch_time, log_epoch_time - epoch_time);
        }

        static std::shared_ptr<spdlog::logger> initLogs(std::string &typeName, char *policyName) {
            char logSymbolPath[128];
            memset(logSymbolPath, 0, sizeof(128));
            sprintf(logSymbolPath, "./logs/%s/%s.txt", typeName.c_str(), policyName);

            std::filesystem::path tradeSymbolPath(logSymbolPath);
            if (!std::filesystem::exists(tradeSymbolPath.parent_path())) {
                std::filesystem::create_directories(tradeSymbolPath.parent_path());
            }
            char logKey[128]{""};
            sprintf(logKey, ".%s_%s_%s", typeName.c_str(), typeName.c_str(), policyName);
            fprintf(stderr, "initLogs %s %s\n", logKey, logSymbolPath);

            return spdlog::basic_logger_st(logKey, logSymbolPath);
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
                    if (line_vector.size() < 45 && !(
                            atoi(line_vector[6].c_str()) == 2 || atoi(line_vector[6].c_str()) == 6)) {
                        continue;
                    }
                    Types::InstrumentInfo instrumentInfo;
                    //   fprintf(stderr, "%s\n", line_vector[1].c_str());
                    line_vector[1].erase(std::remove(line_vector[1].begin(), line_vector[1].end(), '-'),
                                         line_vector[1].end());
                    strcpy(instrumentInfo.instrumentID.data(), line_vector[1].c_str());
                    instrumentInfo.productIDClass = Types::ProductClass::option; // atoi(line_vector[6].c_str());
                    instrumentInfo.expireDate = atoi(line_vector[17].c_str());


                    Utils::InstrumentToProduct(instrumentInfo.instrumentID, instrumentInfo.productID);
                    Utils::parseInstruemnt(instrumentInfo.instrumentID, instrumentInfo.underly,
                                           instrumentInfo.optionType, instrumentInfo.strikePrice);
                    if (strcmp(instrumentInfo.underly.data(), filterUnderly.data()) == 0) {
                        testSymbols.emplace_back(instrumentInfo);
                    }
                }
                i++;
            }
        }
    }
}


#endif //Cosmos_UTILS_H
