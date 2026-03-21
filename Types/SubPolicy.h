//
// Created by zhangyw on 4/17/20.
//

#ifndef HFT_MM_SUBPOLICY_H
#define HFT_MM_SUBPOLICY_H

#include <string>

namespace TrendFollow{
    namespace Types{
        struct SubscribeEngine{
            int policyid{-1};
            int m_affiThreadId{-1};
            std::string policyName{""};

        };
    };
};
#endif //HFT_MM_SUBPOLICY_H
