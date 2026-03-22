//
// Created by zhangyw on 5/11/19.
//

#ifndef TRADEBOTS_PARAM_H
#define TRADEBOTS_PARAM_H

#include "Type.h"

namespace Cosmos{
    namespace Types{
        class Param{
        public:
            std::string name;
            std::string value;
            std::map<std::string, std::string> paramMap;
            static std::string getValue(std::map<std::string, std::string> const& paramsMap, std::string&& key){
                auto itr = paramsMap.find(key);
                if (itr == paramsMap.end()){
                    assert(false);
                }
                return itr->second;
            };
        };
    }
}

#endif //TRADEBOTS_PARAM_H
