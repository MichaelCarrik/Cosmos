//
// Created by zhangyingwei on 2026/9/26.
//

#include "SARBModelFast.h"
#include "KSeries.h"

namespace Cosmos {
    namespace OptionModel {
        void SABRModelFast::_prepareSliceData(std::vector<double> &strikes, std::vector<double> &volatilities,
                                              std::vector<double> &vegas,
                                              double forwardPrice,
                                              const std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                              int optionSeriesIndex) {
            int idx = 0;
            for (auto itr = callPutSeriesMap->rbegin(); itr != callPutSeriesMap->rend(); ++itr) {
                auto strikePrice = itr->first / 1000.0;
                if (strikePrice <= forwardPrice) {
                    strikes.push_back(strikePrice);
                    auto putSeries = itr->second->putSeries;
                    volatilities.push_back(putSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.IV);
                    vegas.push_back(putSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.vega);
                    idx++;
                }
                if (idx >= m_useOptionNumb) {
                    break;
                }
            }

            std::reverse(strikes.begin(), strikes.end());
            std::reverse(volatilities.begin(), volatilities.end());
            idx = 0;

            for (auto itr = callPutSeriesMap->begin(); itr != callPutSeriesMap->end(); ++itr) {
                auto strikePrice = itr->first / 1000.0;
                if (strikePrice > forwardPrice) {
                    strikes.push_back(strikePrice);
                    auto callSeries = itr->second->callSeries;
                    volatilities.push_back(callSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.IV);
                    vegas.push_back(callSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.vega);
                    idx++;
                }
                if (idx >= m_useOptionNumb) {
                    break;
                }
            }
        };

        void SABRModelFast::sarbFit(double forwardPrice, std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                    int optionSeriesIndex) {
            double fixed_beta = 1.0;
            m_fowardPrice = forwardPrice;

            std::vector<double> strikes;
            std::vector<double> volatilities;
            std::vector<double> vegas;

            _prepareSliceData(strikes, volatilities, vegas, forwardPrice,
                              callPutSeriesMap, optionSeriesIndex);

            // 提取 ATM 波动率（寻找离 F 最近的点）
            auto it = std::min_element(strikes.begin(), strikes.end(),
                                       [forwardPrice](double a, double b) {
                                           return std::abs(a - forwardPrice) < std::abs(b - forwardPrice);
                                       });
            double atm_vol = volatilities[std::distance(strikes.begin(), it)];

            SABR2DFunctor functor(m_fowardPrice, m_T, fixed_beta, atm_vol, strikes, volatilities, vegas);
            Eigen::NumericalDiff<SABR2DFunctor> numDiff(functor);
            Eigen::LevenbergMarquardt<Eigen::NumericalDiff<SABR2DFunctor> > lm(numDiff);

            // 优化控制阈值（兼顾精度与速度）
            lm.parameters.maxfev = 50; // 最多 50 次评估
            lm.parameters.ftol = 1e-5;
            lm.parameters.xtol = 1e-5;

            // 参数初值 (x[0] -> rho=-0.2, x[1] -> nu=0.4)
            Eigen::VectorXd x(2);
            x[0] = std::atanh(-0.2); // rho 对应的未约束空间初值
            x[1] = std::log(0.4); // nu 对应的未约束空间初值

            // 启动优化
            lm.minimize(x);

            // 映射回模型物理参数
            double final_rho = std::tanh(x[0]);
            double final_nu = std::exp(x[1]);
            double final_alpha = solve_alpha_atm(m_fowardPrice, m_T, fixed_beta, final_nu, final_rho, atm_vol);

            Eigen::VectorXd final_fvec(strikes.size());
            functor(x, final_fvec);

            m_alpha = final_alpha;
            m_beta = fixed_beta;
            m_rho = final_rho;
            m_nu = final_nu;
            double rmse = final_fvec.norm() / std::sqrt(strikes.size());

            //      return {final_alpha, fixed_beta, final_nu, final_rho, rmse};
        };
    }
}
