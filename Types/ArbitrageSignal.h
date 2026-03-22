//
// Created by zhangyingwei on 2021/11/25.
//

#ifndef Cosmos_ARBITRAGESIGNAL_H
#define Cosmos_ARBITRAGESIGNAL_H

#include "Signal.h"

namespace Cosmos{
    namespace Types {
        struct ArbitrageSignal {
            int abtrgId{-1};
            Types::Signal buySignal;
            Types::Signal sellSignal;

        };
    }
}


#endif //Cosmos_ARBITRAGESIGNAL_H
