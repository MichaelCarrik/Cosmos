//
// Created by zhangyw on 3/5/21.
//

#ifndef TRENDFOLLOW_CTPTRADECONNECTION_H
#define TRENDFOLLOW_CTPTRADECONNECTION_H

namespace TrendFollow{
    namespace Types {
        struct CtpTradeConnection{
            std::string username;
            std::string password;
            std::string authCode;
            std::string brokerId;
            std::string appId;
            std::array<char,3> OrderPrefix;
            std::string tradeFront;
            std::string interpreterConfig;
        };
    }
}

#endif //TRENDFOLLOW_CTPTRADECONNECTION_H
