//
// Created by zhangyingwei on 2026/3/20.
//

#ifndef OPTIONTRADING_V2_CALLPUTSERIES_H
#define OPTIONTRADING_V2_CALLPUTSERIES_H


namespace Cosmos {
    namespace KData {
        class KSeries;
        struct CallPutSeries{
            KSeries* callSeries{nullptr};
            KSeries* putSeries{nullptr};
        };
    }
}
#endif //OPTIONTRADING_V2_CALLPUTSERIES_H