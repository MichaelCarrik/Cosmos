//
// Created by zhangyw on 2/3/21.
//

#ifndef Cosmos_SIGNAL_H
#define Cosmos_SIGNAL_H
#include "../Types/Type.h"


namespace Cosmos{
    namespace Types {
        struct Signal {
            double price;
            int volume;
            SignalIntension intension;
             Types::OrderSide signalSide;
             Types::OrderTimeType orderTimeType;
            int64_t epoch_time{0L};


            Signal() {
                this->price = 0;
                this->volume = 0;
                this->intension = SignalIntension::put;
                this->orderTimeType = OrderTimeType::COMMON;
            }
        };
    }
}
#endif //Cosmos_SIGNAL_H
