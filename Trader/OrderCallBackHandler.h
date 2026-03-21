#ifndef _DEMO_ORDER_H
#define _DEMO_ORDER_H

#include "RootNetTrade.h"
#include "RootNetMarketData.h"
#include <string>

namespace TrendFollow {
    namespace Trader {
        class OrderCallBackHandler : public RootNetTradeSpi, public RootNetMarketDataSpi
        {

        public:
             Utils::MemoryList< Types::OrderField,  Types::OrderBuffSize>* m_orderList{nullptr};

            void OnOrderNew(const MsgOrderNew &newOrderMsg, const PackHead &msgHead, const RespondInfo &msgRespond){};
            void OnOrderCancel(const MsgOrderCancel &cancelOrderMsg, const PackHead &msgHead, const RespondInfo &msgRespond){};
            void OnQueryStkInfo(const MsgStkInfo &stkInfo, const PackHead &msgHead, const RespondInfo &msgRespond){};
            void OnQueryOrder(const MsgOrderInfo &orderInfo, const PackHead &msgHead, const RespondInfo &msgRespond){};
            void OnQueryTrade(const MsgKnockInfo &knockInfo, const PackHead &msgHead, const RespondInfo &msgRespond){};
            void OnTradeEvent(const MsgSubTradeReturn &subTradeReturn, const PackHead &msgHead, const RespondInfo &msgRespond){};
        };

    }
}





#endif
