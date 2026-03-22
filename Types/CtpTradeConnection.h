//
// Created by zhangyw on 3/5/21.
//

#ifndef Cosmos_CTPTRADECONNECTION_H
#define Cosmos_CTPTRADECONNECTION_H

namespace Cosmos{
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

#endif //Cosmos_CTPTRADECONNECTION_H
