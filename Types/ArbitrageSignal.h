//
// Created by zhangyingwei on 2021/11/25.
//

#ifndef TRENDFOLLOW_ARBITRAGESIGNAL_H
#define TRENDFOLLOW_ARBITRAGESIGNAL_H

#include "Signal.h"

namespace TrendFollow{
    namespace Types {
        struct ArbitrageSignal {
            int abtrgId{-1};
            Types::Signal buySignal;
            Types::Signal sellSignal;

        };
    }
}


#endif //TRENDFOLLOW_ARBITRAGESIGNAL_H
