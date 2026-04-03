//
// Created by zhangyw on 5/11/19.
//

#ifndef TRADEBOTS_PARAM_H
#define TRADEBOTS_PARAM_H

#include "Type.h"

namespace Cosmos{
    namespace Types{
        class InitParam{
        public:
            std::string engineName{""};
            std::map<std::string, std::string> paramMap;
            std::vector <std::map<std::string, std::string>> subPolicyParamsVec;

        };


        struct NetModifyParam {
            std::string engineName{""};
            std::string subPolicyName{""};
            std::string paramName{""};
            Types::Instrument_t symbolName{""};
            std::string paramValue{""};
        };




        struct EngineParam {
            int affiThreadId{-1};
            Product_t productID{""};
            int putResendTimeout{600};
            int hitResendTimeout{60};
            bool futurePreCloseToday{false};
            int futureMinVolume{1};
            int optionMinVolume{1};
            int futureMaxPosition{1};
            int optionMaxPosition{1};
            int riskOTR{1000};
            int riskOpenVolume{480};
            int riskMaxOrderNumb{200};
            ExecuteIntension futureEI{ExecuteIntension::EIPut};
            ExecuteIntension optionEI{ExecuteIntension::EIPut};
            HedgeType hedgeType{HedgeType::spec};
            bool isUseUnderlyPrice{true};
            int m_kbarBiasSeconds{0};

        };
    }
}

#endif //TRADEBOTS_PARAM_H
