//
// Created by zhangyw on 1/11/21.
//

#ifndef TRENDFOLLOW_KSERIESTIME_H
#define TRENDFOLLOW_KSERIESTIME_H

#include "../Types/Type.h"
#include "../Types/KPeriod.h"
#include "../Utils/TradingHours.h"

namespace TrendFollow{
    namespace KData{

        struct KTime{
            int beginPstime{0};
            int endPstime{0};
             Types::UpdateTime_t updateTime_begin{""};
             Types::UpdateTime_t updateTime_end{""};
        };
        class KSeriesTime{

        private:
            int m_tradingday;
             Types::KPeriod m_period;
            std::vector<KTime> m_ktime;
            int index{0};
            int m_secondsBias{0};
        public:
            KSeriesTime(int tradingday,  Types::KPeriod m_period, int secondsBias):m_tradingday(tradingday),m_period(m_period),m_secondsBias{secondsBias}{

            }

            bool isLast(){
                if(index == m_ktime.size()-1){
                    return true;
                }
                return false;
            }

            KTime *getNext(){
                return &m_ktime[++index];
            };

            KTime *getCurrent(int& remain){
                remain = m_ktime.size()- index;
                return &m_ktime[index];

            };

            void align(int tradingday ,int beginTime){
               if(tradingday < m_tradingday){

               }else if(tradingday ==m_tradingday){
                   while(m_ktime[index].beginPstime <= beginTime and this->isLast()== false){
                       index++;
                   }
               }else {
                   assert(false && "align");
               }
            }

            void initKTime( Utils::TradingSession& tradingSession, bool isDay){
                m_ktime.clear();
                index = 0;
                auto seconds =  Types::KPeroidToSecondsMap[m_period];
                int beginTime =0;
                int lastEndTime=0;

                for(auto tradingDr : tradingSession.tradingVec){

                    if(m_period ==  Types::KPeriod::D1){
                        KTime ktime;
                        ktime.beginPstime = 0-5*60;
                        ktime.endPstime =  tradingSession.closeTodaySeconds;
                        m_ktime.emplace_back(ktime);
                        break;
                    }

                    if(isDay == true && tradingDr.endTime <  Utils::DayBegin){
                        continue;
                    } else if(isDay == false && tradingDr.beginTime >  Utils::DayBegin){
                        continue;
                    }


                    if(lastEndTime + seconds > tradingDr.beginTime && beginTime < tradingDr.beginTime){
                        KTime ktime;
                        ktime.beginPstime = beginTime ;
                        ktime.endPstime =  ktime.beginPstime+seconds -1 ;
                         Utils::ToUpdateTime(ktime.beginPstime, ktime.updateTime_begin);
                         Utils::ToUpdateTime(ktime.endPstime, ktime.updateTime_end);
                        m_ktime.emplace_back(ktime);
                        beginTime =  ktime.beginPstime+seconds;
                    }else if(lastEndTime >beginTime){
                        KTime ktime;
                        ktime.beginPstime = beginTime ;
                        ktime.endPstime =  lastEndTime ;
                         Utils::ToUpdateTime(ktime.beginPstime, ktime.updateTime_begin);
                         Utils::ToUpdateTime(ktime.endPstime, ktime.updateTime_end);
                        m_ktime.emplace_back(ktime);
                        beginTime =  tradingDr.beginTime;
                    }else{
                        beginTime =  tradingDr.beginTime ;
                    }


                    while(beginTime +seconds <= tradingDr.endTime and beginTime >= tradingDr.beginTime){
                        KTime ktime;
                        ktime.beginPstime = beginTime ;
                        ktime.endPstime =  ktime.beginPstime+seconds  < tradingDr.endTime?  ktime.beginPstime+seconds -1 : ktime.beginPstime+seconds;
                         Utils::ToUpdateTime(ktime.beginPstime, ktime.updateTime_begin);
                         Utils::ToUpdateTime(ktime.endPstime, ktime.updateTime_end);
                        m_ktime.emplace_back(ktime);
                        beginTime =  ktime.beginPstime+seconds;
                    }

                    lastEndTime = tradingDr.endTime;

//                    if(beginTime <tradingSession.closeTodaySeconds and beginTime +seconds >= tradingSession.closeTodaySeconds ){
//                        KTime ktime;
//                        ktime.beginPstime = beginTime;
//                        ktime.endPstime =   tradingSession.closeTodaySeconds;
//                         Utils::ToUpdateTime(ktime.beginPstime, ktime.updateTime);
//                        m_ktime.emplace_back(ktime);
//                    }
                }
                setSecondsBias();
            }


            void setSecondsBias(){
                for(auto& ktime :  m_ktime){
                        ktime.beginPstime +=  m_secondsBias ;
                        ktime.endPstime +=  m_secondsBias;
                         Utils::ToUpdateTime(ktime.beginPstime, ktime.updateTime_begin);
                         Utils::ToUpdateTime(ktime.endPstime, ktime.updateTime_end);
                }
            }

            void setIndex(int ind){
                index =ind;
            }
        };
    }
}

#endif //TRENDFOLLOW_KSERIESTIME_H
