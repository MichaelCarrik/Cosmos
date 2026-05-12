//
// Created by zhangyingwei on 2026/3/26.
//

#ifndef COSMOS_LOGUTILS_H
#define COSMOS_LOGUTILS_H
#include "../Types/Type.h"
#include "../Types/OrderField.h"
#include <filesystem>
#include <map>
#include <spdlog/spdlog.h>

namespace Cosmos {
    namespace Utils {
        static std::map<std::array<char, 128>, spdlog::logger *> g_spdLogPool;

        static spdlog::logger *initLogs(std::string &typeName, std::string &engineName, std::string &policyName) {
            std::array<char, 128> logKey{""};
            sprintf(logKey.data(), "%s_%s_%s", typeName.c_str(), engineName.c_str(), policyName.c_str());
            auto itr = g_spdLogPool.find(logKey);
            if (itr == g_spdLogPool.end()) {
                char logSymbolPath[128];
                memset(logSymbolPath, 0, sizeof(128));
                sprintf(logSymbolPath, "./logs/%s/%s_%s.txt", typeName.c_str(), engineName.c_str(), policyName.c_str());
                std::filesystem::path tradeSymbolPath(logSymbolPath);
                if (!std::filesystem::exists(tradeSymbolPath.parent_path())) {
                    std::filesystem::create_directories(tradeSymbolPath.parent_path());
                }
                fprintf(stderr, "initLogs %s %s\n", logKey.data(), logSymbolPath);
                auto logPtr = spdlog::basic_logger_st(logKey.data(), logSymbolPath).get();
                logPtr->set_pattern("[%Y%m%d %H:%M:%S.%e] [%l] %v");
                g_spdLogPool[logKey] = logPtr;
            }
            return g_spdLogPool[logKey];
        }

        template<class T>
        static void logOrder(const Types::OrderField *orderField, spdlog::logger *orderLog,
                             const T *symbol, int tradingday,
                             const int64_t epoch_time) {
            auto log_epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            orderLog->info(
                "{}, {}, {} {:03d}, {}, {}, {}, {}, {}, {}, {:.3f}({:.3f} {:.3f} {} {}), {}, {:.3f}, {}, {}, delta={}, {}, {}, {}",
                orderField->instrumentID.data(), tradingday,
                symbol->lastMD->updateTime.data(), symbol->lastMD->milliSeconds,
                Types::orderStatusMap[orderField->orderStatus].data(),
                orderField->pOrderID, orderField->tOrderID,
                Types::positionEffectMap[orderField->pet].data(),
                Types::orderSideMap[orderField->orderSide].data(),
                Types::OIMap[orderField->OI].data(), orderField->orderPrice,
                symbol->lastMD->bidPrice[0], symbol->lastMD->askPrice[0],
                symbol->lastMD->bidVolume[0], symbol->lastMD->askVolume[0],
                orderField->orderVolume, orderField->lastFilledPrice,
                orderField->lastFilledVolume, orderField->filledVolume, symbol->tradePosition.filledPosition,
                orderField->orderSysID.data(), epoch_time, log_epoch_time - epoch_time);
        }

        static void logPos(spdlog::logger *positionLog, bool isTrade, int tradingDay, const Types::Symbol *symbol,
                           Types::RiskIndicator const &riskIndicator,
                           int tradeOrderId) {
            if (isTrade == true) {
                positionLog->info(
                    "symbol={} tickTime={} {}.{} {}, filledPos={}, avgPrice={:.3f}, T_BHold={}, Y_BHold={}, T_SHold={}, "
                    "Y_SHold={}, profit={:.3f}, tradeOrderId={}, openBVlm={}, openSVlm={}, "
                    "SendNumb={}, CancelNumb={}, filledNumb={}, O_SendNumb={}, O_CancelNumb={}, O_filledNumb={},",
                    symbol->instrumentInfo.instrumentID.data(), tradingDay, symbol->lastMD->updateTime.data(),
                    symbol->lastMD->milliSeconds,
                    isTrade, symbol->tradePosition.filledPosition, symbol->tradePosition.averagePrice,
                    symbol->tradePosition.T_buyHold,
                    symbol->tradePosition.Y_buyHold, symbol->tradePosition.T_sellHold, symbol->tradePosition.Y_sellHold,
                    symbol->tradePosition.profit,
                    tradeOrderId, symbol->riskIndicator.openBuyVolume, riskIndicator.openSellVolume,
                    riskIndicator.sendOrderNumb,
                    riskIndicator.cancelOrderNumb, riskIndicator.filledOrderNumb, riskIndicator.optionSendOrderNumb,
                    riskIndicator.optionCancelOrderNumb,
                    riskIndicator.optionFilledOrderNumb);
            } else {
                positionLog->info(
                    "symbol={} tickTime={} {}.{} {}, filledPos={}, avgPrice={:.3f}, T_BHold={}, Y_BHold={}, T_SHold={}, "
                    "Y_SHold={}, profit={:.3f}, tradeOrderId={}, openBVlm={}, openSVlm={}, "
                    "SendNumb={}, CancelNumb={}, filledNumb={}, O_SendNumb={}, O_CancelNumb={}, O_filledNumb={},",
                    symbol->instrumentInfo.instrumentID.data(), tradingDay, "", "", isTrade, symbol->tradePosition.filledPosition,
                    symbol->tradePosition.averagePrice, symbol->tradePosition.T_buyHold, symbol->tradePosition.Y_buyHold, symbol->tradePosition.T_sellHold,
                    symbol->tradePosition.Y_sellHold, symbol->tradePosition.profit, tradeOrderId, symbol->riskIndicator.openBuyVolume,
                    riskIndicator.openSellVolume, riskIndicator.sendOrderNumb, riskIndicator.cancelOrderNumb, riskIndicator.filledOrderNumb,
                    riskIndicator.optionSendOrderNumb, riskIndicator.optionCancelOrderNumb, riskIndicator.optionFilledOrderNumb);
            }

            //   m_positionLog->flush();
        }
    }
}
#endif //COSMOS_LOGUTILS_H
