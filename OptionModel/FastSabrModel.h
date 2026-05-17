//
// Created by zhangyingwei on 2026/5/17.
//

#ifndef COSMOS_FASTSABRMODEL_H
#define COSMOS_FASTSABRMODEL_H
#include "KData.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include "../Utils/Utils.h"

namespace Cosmos {
    namespace OptionModel {
        struct FastSabrModel {
        public:

            FastSabrModel(int tradingDay, int expireDay) : m_expireDay(expireDay),  m_tradingDay(tradingDay){

                auto diff = (Utils::intToSysDays(m_expireDay) - Utils::intToSysDays(m_tradingDay)).count();
                m_T = std::max(diff / 365.0, 1e-5);
            };
            int m_tradingDay{0};
            int m_expireDay{0};
            double m_T{0.0};
            double m_alpha{0.0};
            double m_beta{0.99};
            double m_rho{0.0};
            double m_nu{0.4};
            int m_useOptionNumb{4};
            void sarbFit(double forwardPrice,  std::map<int, KData::CallPutSeries *> *  callPutSeriesMap, int optionSeriesIndex);
            void getParameters(KData::SabrPRMT & sabrPrmt);
        private:
            // 1. 【极致内联】Hagan (2002) 经典隐含波动率显式代数公式
            double _impVolHagan(double F, double K, double T, double alpha, double beta, double rho,
                                             double nu) ;

            // 2. 【核心降维加速】一维黄金分割法校准（无需通用优化器，彻底消除收敛死锁）
            // 假设 beta 已由盘口历史统计固定 (例如股票/商品期权通常设 beta = 0.5)
             void _calibrateSmile(double F, double T, double atmVol,
                                       const std::vector<double> &strikes,
                                       const std::vector<double> &marketVols,
                                       double beta,
                                       double &out_alpha, double &out_rho, double &out_nu) ;


            void _prepareSliceData(std::vector<double>& strikes,  std::vector<double>& volatilities,
            double forwardPrice, const std::map<int, KData::CallPutSeries *> *  callPutSeriesMap, int optionSeriesIndex) ;
        };
    }
}


#endif //COSMOS_FASTSABRMODEL_H
