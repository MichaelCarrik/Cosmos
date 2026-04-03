//
// Created by zhangyw on 12/18/20.
//

#ifndef HFT_MM_VARIENTTYPES_H
#define HFT_MM_VARIENTTYPES_H

#include "Type.h"
#include "MarketData.h"
#include "OrderField.h"
//#include "../MMEngine/MMEngine.h"
//#include "../TrendEngine/TrendEngine.h"



namespace Cosmos{
    namespace Types {

      enum class EventType {
          marketEvent, orderEvent, paramsEvent
      };


       struct EventData{
           const void* point;
           EventType eventType;
       };


    }

}

#endif //HFT_MM_VARIENTTYPES_H
