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
        static void logOrder(const Types::OrderField *orderField, spdlog::logger* m_orderLog,
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
                Types::signalIntensionMap[orderField->intension].data(),
                Types::hedgeMap[orderField->hedgeType].data(),
                orderField->orderPrice, orderField->orderVolume, orderField->lastFilledPrice,
                orderField->lastFilledVolume, orderField->filledVolume, symbol->tradePosition.filledPosition,
                symbol->tradePosition.averagePrice,
                orderField->orderSysID.data(), epoch_time, log_epoch_time - epoch_time);
        }


    }
}
#endif //COSMOS_LOGUTILS_H
