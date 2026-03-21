//
// Created by zhangyingwei on 2024/6/14.
//

#ifndef OPTIONTRADING_VARIANCETYPES_H
#define OPTIONTRADING_VARIANCETYPES_H
#include "Type.h"

namespace TrendFollow {
    namespace Types {
        struct VarianceTypes {
             Types::Instrument_t m_instrument{""};
             Types::UpdateTime_t m_updateTime{""};
            int m_tradingday{0};
            int m_psTime{0};
             Types::Product_t m_productID{""};
            int m_expireDay{0};
            int m_remainDays{0};
            double m_bidVariance {0.0};
            double m_bidVolitality{0.0};
            double m_midVariance {0.0};
            double m_midVolitality{0.0};
            double m_askVariance {0.0};
            double m_askVolitality{0.0};
        };
    }
}


#endif //OPTIONTRADING_VARIANCETYPES_H
