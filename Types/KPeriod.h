//
// Created by zhangyw on 1/19/21.
//

#ifndef TRENDFOLLOW_KPERIOD_H
#define TRENDFOLLOW_KPERIOD_H
namespace TrendFollow{
    namespace Types {
        enum class KPeriod {
            Min1,
            Min5,
            Min15,
            Min30,
            H1,
            D1,
        };



        static std::unordered_map<KPeriod, int> KPeroidToSecondsMap{
                {KPeriod::Min1,  60},
                {KPeriod::Min5,  300},
                {KPeriod::Min15, 900},
                {KPeriod::Min30, 1800},
                {KPeriod::H1,    3600},
                {KPeriod::D1,    3600 * 24}
        };


        static std::unordered_map<KPeriod, int> KPeroidToIntervalMap{

                {KPeriod::Min1,  1},
                {KPeriod::Min5,  5},
                {KPeriod::Min15, 15},
                {KPeriod::Min30, 30},
                {KPeriod::H1,    60},
                {KPeriod::D1,    24}
        };

        static std::unordered_map<KPeriod, std::array<char, 26>> KPeroidToFutureTableMap{

                {KPeriod::Min1,   std::array<char, 26>{"oneMinute_bars"}},
                {KPeriod::Min5,   std::array<char, 26>{"minutes_bars"}},
                {KPeriod::Min15,  std::array<char, 26>{"minutes_bars"}},
                {KPeriod::Min30,  std::array<char, 26>{"minutes_bars"}},
                {KPeriod::H1,     std::array<char, 26>{"minutes_bars"}},
                {KPeriod::D1,   std::array<char, 26>{"day_bars"}}
        };

        static std::unordered_map<KPeriod, std::array<char, 26>> KPeroidToOptionTableMap{

                {KPeriod::Min1,   std::array<char, 26>{"optionOneMinute_bars"}},
                {KPeriod::Min5,   std::array<char, 26>{"optionMinutes_bars"}},
                {KPeriod::Min15,  std::array<char, 26>{"optionMinutes_bars"}},
                {KPeriod::Min30,  std::array<char, 26>{"optionMinutes_bars"}},
                {KPeriod::H1,     std::array<char, 26>{"optionMinutes_bars"}},
                {KPeriod::D1,   std::array<char, 26>{"optionDay_bars"}}
        };


        static std::vector< Types::KPeriod> m_kperoidVec{
              Types::KPeriod::Min1 , Types::KPeriod::Min5,
              Types::KPeriod::Min15 , Types::KPeriod::Min30,
              Types::KPeriod::H1, Types::KPeriod::D1 ,

        };

        static const std::unordered_map<std::string, KPeriod> configParamToKPeriodMap{
                {"Min1", KPeriod::Min1},
                {"Min5", KPeriod::Min5},
                {"Min15", KPeriod::Min15},
                {"Min30", KPeriod::Min30},
                {"H1", KPeriod::H1},
                {"D1", KPeriod::D1}
        };

 //       static std::vector< Types::KPeriod> m_kperoidVec{ Types::KPeriod::D1 };
    }
}

#endif //TRENDFOLLOW_KPERIOD_H
