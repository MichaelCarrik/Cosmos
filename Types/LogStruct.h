//
// Created by zhangyw on 2/6/21.
//

#ifndef TRENDFOLLOW_LOGSTRUCT_H
#define TRENDFOLLOW_LOGSTRUCT_H
#include "Type.h"

namespace TrendFollow {
    namespace Types {
        struct LogStruct{
            std::shared_ptr<spdlog::logger> recordLog;
            std::shared_ptr<spdlog::logger> m_positionLog;
            std::map<std::string ,  std::shared_ptr<spdlog::logger>> m_configLogsMap;
        };
    }
}

#endif //TRENDFOLLOW_LOGSTRUCT_H
