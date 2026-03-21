//
// Created by zhangyingwei on 2022/1/18.
//

#ifndef TRENDFOLLOW_MMSIGNAL_H
#define TRENDFOLLOW_MMSIGNAL_H

#include "Signal.h"

namespace TrendFollow{
    namespace Types {
        struct MMSignal {
            int abtrgId{-1};
            Types::Signal buySignal;
            Types::Signal sellSignal;

        };
    }
}

#endif //TRENDFOLLOW_MMSIGNAL_H
