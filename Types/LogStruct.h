//
// Created by zhangyw on 2/6/21.
//

#ifndef Cosmos_LOGSTRUCT_H
#define Cosmos_LOGSTRUCT_H
#include "Type.h"

namespace Cosmos {
    namespace Types {
        struct LogStruct{
            std::shared_ptr<spdlog::logger> recordLog;
            std::shared_ptr<spdlog::logger> m_positionLog;
            std::map<std::string ,  std::shared_ptr<spdlog::logger>> m_configLogsMap;
        };
    }
}

#endif //Cosmos_LOGSTRUCT_H
