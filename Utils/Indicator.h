//
// Created by zhangyw on 2/4/21.
//

#ifndef Cosmos_INDICATOR_H
#define Cosmos_INDICATOR_H

#include "../Types/Type.h"

namespace Cosmos {
    namespace Indicator {

        template<typename T>
        static T Sum(std::vector<T> const &inputVec, int begin, int end) {


            T sumValue = 0;
            for (int i = begin; i <= end; i++) {
                sumValue = sumValue + inputVec[i];
		// if (end == 999) {
		  //   fprintf(stderr, "Sum : inputVec[%d]=%.3f, begin=%d, end=%d, sumValue=%.3f\n", i,  (double)inputVec[i], begin, end, (double)sumValue);
		 //}

            }
            return sumValue;
        }

        template<typename T>
        static T MA(std::vector<T> const &inputVec, int begin, int end) {
            auto sumValue = Sum(inputVec, begin, end);
	  /*  if (end == 999) {
	        fprintf(stderr, "MA : sumValue=%.3f, begin=%d, end=%d\n", sumValue, begin, end);
	    }*/
            return sumValue / (end - begin +1);
        }


        template<typename T>
        static double STDEV(std::vector<T> const &inputVec, int begin, int end, int stdType = 2) {
            //stdType, MC default is 1, TB default is 2.
            T sumPrice = 0;
            T sumSqr = 0;
            T stdev = 0.0;

            auto mean = MA(inputVec, begin, end);
            //   fprintf(stderr, "STDEV : begin=%d. end=%d, mean=%.3f , lastValue=%.3f \n", begin, end, mean, inputVec[end-1]);
            for (int i = begin; i <=end; i++) {
              /*  if (end == 999) {
                    fprintf(stderr, "STDEV : inputVec[%d]=%.3f. end=%d, mean=%.3f\n", inputVec[i], i, end, mean);
                }*/

                sumSqr = sumSqr + (inputVec[i] - mean) * (inputVec[i] - mean);
            }
            if (stdType == 1)
                stdev = sumSqr / (end - begin +1);
            else if (stdType != 1 && inputVec.size() != 0)
                stdev = sumSqr / (end - begin);
            else
                stdev = 0.0;
            stdev = std::sqrt(stdev);

            return stdev;
        }


        template<typename T>
        static void array_Sum(std::vector<T> const &inputVec, int len, std::vector<T> &sumVec) {
            for (int i = 0; i < inputVec.size(); i++) {
                T sumValue = 0;
                if (i < len) {
                    sumValue = Sum(inputVec, 0, i );
                } else {
                    // fprintf(stderr, "array_Sum : first=%d. end=%d\n", inputVec.size()-len, inputVec.size());
                    sumValue = Sum(inputVec, i + 1 - len, i);
                }

                sumVec.emplace_back(sumValue);
            }
        }


        template<typename T>
        static void array_MA(std::vector<T> const &inputVec, int len, std::vector<T> &maVec) {

            for (int i = 0; i < inputVec.size(); i++) {
                T ma = 0;
                if (i < len) {
                    ma = MA(inputVec, 0, i );
                } else {
                    ma = MA(inputVec, i+1 - len, i);
                }

                maVec.emplace_back(ma);
            }
        }



        template<typename T>
        static void array_STDEV(std::vector<T> &inputVec, int len, std::vector<T> &stdVec, int stdType = 2) {

            for (int i = 0; i < inputVec.size(); i++) {
                T std = 0;
                if (i < len) {
                    std = STDEV(inputVec, 0, i , stdType);
                } else {
                    std = STDEV(inputVec, i +1 - len, i, stdType);
                }

                stdVec.emplace_back(std);
            }

        }


        template<typename T>
        static double Quartile(std::vector<T> const &inputVec, int begin, int end, double quantile) {
            std::vector<T> tempSortVec;

            for (int i = begin; i <=end; i++) {
                tempSortVec.template emplace_back(inputVec[i]);
            }

            std::sort(std::begin(tempSortVec), std::end(tempSortVec));

            double pos =  (tempSortVec.size() +1)*quantile;
            double posQ = pos - std::floor(pos);
            double returnValue = 0.0;
            if(posQ < 0.000001){
                returnValue = tempSortVec[std::round(pos)-1];
            }else{
                T firstValue = tempSortVec[std::floor(pos)-1];
                T secondValue = tempSortVec[std::ceil(pos)-1];
                returnValue = posQ * firstValue+ (1 -posQ) * secondValue;
            }


            return returnValue;
        }

//        template<typename T>
//        static T array_quartiles(std::vector<T> &inputVec, int len, double quartile){
//            T quartileValue = 0.0;
//
//            for (int i = 0; i < inputVec.size(); i++) {
//                if (i < len) {
//                    quartileValue = Quartile(inputVec, 0, i, quartile);
//
//                } else {
//                    quartileValue = Quartile(inputVec, i +1 - len, i, quartile);
//                }
//
//            }
//
//            return quartileValue ;
//        }


        static int calBarVolume(const  KData::KData *bar, int lastTradingday, int &lastVolume) {
            if (lastTradingday != bar->m_tradingday) {
                lastVolume = 0;
            }
            return bar->m_volume - lastVolume;

        }

        static void array_calBarVolume( KData::KSeries *mainKSeries, std::vector<int> &barVolumeVec) {
            int lastTradingday = -1;
            int lastVolume = 0;
            for (auto i = 0; i < mainKSeries->m_seriesIndex; i++) {
                auto bar = mainKSeries->m_KDataVecs[i];

                barVolumeVec.emplace_back(calBarVolume(bar, lastTradingday, lastVolume));
                lastTradingday = bar->m_tradingday;
                lastVolume = bar->m_volume;

                //  fprintf(stderr, "array_calBarVolume %d: %d %s %d %d\n", i ,bar->m_tradingday, bar->m_updateTime.data(), bar->m_volume, barVolumeVec[i]);

            }
        }


    }
}
#endif //Cosmos_INDICATOR_H
