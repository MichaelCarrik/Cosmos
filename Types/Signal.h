//
// Created by zhangyw on 2/3/21.
//

#ifndef Cosmos_SIGNAL_H
#define Cosmos_SIGNAL_H
#include "../Types/Type.h"


namespace Cosmos{
    namespace Types {
        struct Signal {
            double price{0.0};
            int volume{0};
            OrderIntension OI{OrderIntension::OIPut};
            Types::OrderSide signalSide;
            Types::OrderTimeType orderTimeType;
            int64_t epoch_time{0L};

        };
    }
}
#endif //Cosmos_SIGNAL_H
