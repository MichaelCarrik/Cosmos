//
// Created by zhangyw on 5/11/19.
//

#ifndef TRADEBOTS_PARAM_H
#define TRADEBOTS_PARAM_H

#include "Type.h"

namespace Cosmos{
    namespace Types{
        class InitParam{
        public:
            std::string engineName;
            std::map<std::string, std::string> paramMap;
            std::vector <std::map<std::string, std::string>> subPolicyParamsVec;

        };

        struct EngineParam {
            int affiThreadId{-1};
            Types::Product_t productID{""};
            int putResendTimeout{600};
            bool futurePreCloseToday{false};
            int futureMinVolume{1};
            int optionMinVolume{1};
            int futureMaxPosition{1};
            int optionMaxPosition{1};
            Types::HedgeType hedgeType{Types::HedgeType::spec};
            bool isUseUnderlyPrice{true};

        };
    }
}

#endif //TRADEBOTS_PARAM_H
