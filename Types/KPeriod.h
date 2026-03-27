//
// Created by zhangyw on 1/19/21.
//

#ifndef Cosmos_KPERIOD_H
#define Cosmos_KPERIOD_H
namespace Cosmos{
    namespace Types {
        enum class KPeriod : int {
            Min1 =0,
            Min5 =1,
            Min15 =2,
            Min30 =3,
            D1 =4,
        };

        static std::vector<int> KPeroidToSecondsVec{
                60, 300, 900, 1800, 3600 * 24
        };


        static std::vector<int> KPeroidToIntervalVec{
                 1,5,15,30, 24
        };

        static std::vector<std::array<char, 26>> KPeroidToFutureTableVec{
                std::array<char, 26>{"futureOneMinute_bars"},
                std::array<char, 26>{"futureMinutes_bars"},
                std::array<char, 26>{"futureMinutes_bars"},
                std::array<char, 26>{"futureMinutes_bars"},
                std::array<char, 26>{"futureDay_bars"}
        };

        static std::vector<std::array<char, 26>> KPeroidToOptionTableMap{
            std::array<char, 26>{"optionOneMinute_bars"},
            std::array<char, 26>{"optionMinutes_bars"},
            std::array<char, 26>{"optionMinutes_bars"},
            std::array<char, 26>{"optionMinutes_bars"},
            std::array<char, 26>{"optionDay_bars"}
        };


        static std::vector< Types::KPeriod> m_kperoidVec{
              Types::KPeriod::Min1 , Types::KPeriod::Min5,
              Types::KPeriod::Min15 , Types::KPeriod::Min30,
              Types::KPeriod::D1 ,
        };

        static const std::unordered_map<std::string, KPeriod> configParamToKPeriodMap{
                {"Min1", KPeriod::Min1},
                {"Min5", KPeriod::Min5},
                {"Min15", KPeriod::Min15},
                {"Min30", KPeriod::Min30},
                {"D1", KPeriod::D1}
        };

 //       static std::vector< Types::KPeriod> m_kperoidVec{ Types::KPeriod::D1 };
    }
}

#endif //Cosmos_KPERIOD_H
