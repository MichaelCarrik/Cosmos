//
// Created by zhangyingwei on 2022/1/18.
//

#ifndef Cosmos_MMSIGNAL_H
#define Cosmos_MMSIGNAL_H

#include "Signal.h"

namespace Cosmos{
    namespace Types {
        struct MMSignal {
            int abtrgId{-1};
            Types::Signal buySignal;
            Types::Signal sellSignal;

        };
    }
}

#endif //Cosmos_MMSIGNAL_H
