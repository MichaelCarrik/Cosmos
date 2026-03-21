#ifndef _DEMOACCOUNT_H
#define _DEMOACCOUNT_H

#include "RootNetTrade.h"
#include "RootNetMarketData.h"

class DemoAccountCallBackHandler : public RootNetTradeSpi
{
public:
    void OnQueryAcctInfo(const MsgAccountInfo &accountInfo, const PackHead &msgHead, const RespondInfo &msgRespond){};
    void OnQueryFutAcctInfo(const MsgFutAccountInfo &futAccountInfo, const PackHead &msgHead, const RespondInfo &msgRespond){};
    void OnQueryPosition(const MsgPositionInfo &positionInfo, const PackHead &msgHead, const RespondInfo &msgRespond){};
    void OnTradeEvent(const MsgSubTradeReturn &subTradeReturn, const PackHead &msgHead, const RespondInfo &msgRespond){};
};

#endif
