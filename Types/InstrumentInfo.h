//
// Created by zhangyingwei on 2026/3/19.
//

#ifndef OPTIONTRADING_V2_INSTRUMENTINFO_H
#define OPTIONTRADING_V2_INSTRUMENTINFO_H

#include "Type.h"

namespace Cosmos {
    namespace Types {
        struct UnderlyInfo{
            Instrument_t instrumentID{""};
            bool isWithOption{false};
        };

        struct InstrumentInfo{
            Instrument_t instrumentID{""};
            Product_t productID{""};
            ProductClass productIDClass {Types::ProductClass::future};
            ExchangeType exchanges;
            int expireDate{0};
            char optionType{'N'};
            Instrument_t underly{""};
            double strikePrice{0.0};
            double tickSize{0.0};
            double multi{0.0};
        };
    }
}
#endif //OPTIONTRADING_V2_INSTRUMENTINFO_H