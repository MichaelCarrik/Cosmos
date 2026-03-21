//
// Created by zhangyingwei on 2024/12/20.
//

#ifndef OPTIONTRADING_UNDERLYINSMAP_H
#define OPTIONTRADING_UNDERLYINSMAP_H

#include "Type.h"
#include <map>


namespace TrendFollow {
    namespace Types {
        struct RiskOrderControl{
            bool callBuy{false};
            bool putBuy{false};
            bool callSell{false};
            bool putSell{false};
        };
        class UnderlyInsMap{
        public:
            std::map<Types::Instrument_t, RiskOrderControl> underlyInsMap;
            Types::Instrument_t nearInstrument{""};
            Types::Instrument_t farInstrument{""};
            bool isIn(Types::Instrument_t const& instrument){
               auto itr=  underlyInsMap.find(instrument);
               if(itr == underlyInsMap.end()){
                   return false;
               }
               return true;
            }

        };
    }
}

#endif //OPTIONTRADING_UNDERLYINSMAP_H
