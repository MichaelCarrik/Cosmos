//
// Created by zhangyingwei on 2026/5/14.
//

#include "SARBModelQuantLib.h"
#include "KSeries.h"
#include <ql/math/interpolations/sabrinterpolation.hpp>
#include <ql/termstructures/volatility/sabr.hpp>


namespace Cosmos {
    namespace OptionModel {
        void SARBModelQuantLib::_prepareSliceData(std::vector<QuantLib::Real> &strikes,
                                                  std::vector<QuantLib::Real> &volatilities,
                                                  std::vector<QuantLib::Real> &vegas,
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
        //
        // void SARBModelQuantLib::sarbFit(double forwardPrice, std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
        //                                 int optionSeriesIndex) {
        //     std::vector<QuantLib::Real> strikesTemp;
        //     std::vector<QuantLib::Real> volatilitiesTemp;
        //
        //     _prepareSliceData(strikesTemp, volatilitiesTemp, forwardPrice,
        //                            callPutSeriesMap, optionSeriesIndex) ;
        //
        //     m_fowardPrice = forwardPrice;
        //
        //     // bool isStrikesSame = true;
        //     // for (int i = 0; i < strikesTemp.size(); i++) {
        //     //     if (std::abs(m_strikes[i] - strikesTemp[i]) > Types::g_epsilon) {
        //     //         isStrikesSame = false;
        //     //     }
        //     // }
        //
        //     if (m_sabrInterp == nullptr || m_strikes.size() != strikesTemp.size() ) {
        //         if (m_sabrInterp != nullptr) {
        //             delete m_sabrInterp;
        //             m_sabrInterp = nullptr;
        //
        //         }
        //         try {
        //             auto minIV = std::min_element(volatilitiesTemp.begin(), volatilitiesTemp.end());
        //             m_alpha = std::max(*minIV, 0.15);
        //
        //             m_strikes.clear();
        //             for (int i = 0; i < strikesTemp.size(); i++) {
        //                 m_strikes.push_back(strikesTemp[i]);
        //             }
        //
        //             m_volatilities.clear();
        //             for (int i = 0; i < volatilitiesTemp.size(); i++) {
        //                 m_volatilities.push_back(volatilitiesTemp[i]);
        //             }
        //
        //             m_sabrInterp = new QuantLib::SABRInterpolation(
        //                m_strikes.begin(), m_strikes.end(),
        //                m_volatilities.begin(),
        //                m_T, m_fowardPrice,
        //                m_alpha, m_beta, m_nu, m_rho,
        //                false, true,
        //                false, false, true, m_endCriteria, m_optimizationMethod
        //                );
        //         }
        //             catch (std::exception &e) {
        //                 m_sabrInterp = nullptr;
        //              //   fprintf(stderr, " new SABRInterpolation  Error: %s\n", e.what());
        //             }
        //     }else {
        //         for (int i = 0; i < strikesTemp.size(); i++) {
        //             m_strikes[i] = strikesTemp[i];
        //         }
        //         for (int i = 0; i < volatilitiesTemp.size(); i++) {
        //             m_volatilities[i] = volatilitiesTemp[i];
        //         }
        //     }
        //
        //     if (m_sabrInterp != nullptr) {
        //         try {
        //             m_sabrInterp->update();
        //             m_alpha = m_sabrInterp->alpha();
        //             if (std::isnan(m_alpha)) {
        //                 int a = 1;
        //             }
        //             m_beta = m_sabrInterp->beta();
        //             m_nu = m_sabrInterp->nu();
        //             m_rho = m_sabrInterp->rho();
        //             m_rmse= m_sabrInterp->rmsError();
        //
        //          //   m_isInitialized = false;
        //         }catch (std::exception &e) {
        //           //  fprintf(stderr, " m_sabrInterp->update(); Error: %s\n", e.what());
        //         }
        //     }
        // };
        //
        void SARBModelQuantLib::sarbFit(double forwardPrice,
                                        std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                        int optionSeriesIndex) {
            std::vector<QuantLib::Real> strikesTemp;
            std::vector<QuantLib::Real> volatilitiesTemp;
            std::vector<QuantLib::Real> vegasTemp;

            _prepareSliceData(strikesTemp, volatilitiesTemp, vegasTemp, forwardPrice,
                              callPutSeriesMap, optionSeriesIndex);

            if (strikesTemp.size() < 3) return;

            m_fowardPrice = forwardPrice;

            // 1. 归一化 Vega 平方根权重
            std::vector<QuantLib::Real> weights(strikesTemp.size(), 1.0);
            if (!vegasTemp.empty() && vegasTemp.size() == strikesTemp.size()) {
                QuantLib::Real maxVega = *std::max_element(vegasTemp.begin(), vegasTemp.end());
                if (maxVega < 1e-7) maxVega = 1.0;
                for (size_t i = 0; i < vegasTemp.size(); ++i) {
                    weights[i] = std::sqrt(std::max(vegasTemp[i] / maxVega, 0.01));
                }
            }

            // 2. 构造共享的栈上 CostFunction
            FastSABRCostFunction costFunction(
                m_fowardPrice, m_T, m_beta,
                strikesTemp, volatilitiesTemp, weights
            );

            QuantLib::NoConstraint noConstraint;
            // 调紧容差以换取更高精度
            QuantLib::EndCriteria endCriteria(150, 20, 1e-7, 1e-7, 1e-7);
            QuantLib::LevenbergMarquardt lm;

            // 3. 构建 3 组互补起点候选集 (Alpha 均使用 ATM IV 附近估计)
            double baseAlpha = (m_alpha > 0.0 && !std::isnan(m_alpha))
                                   ? m_alpha
                                   : std::max(volatilitiesTemp[strikesTemp.size() / 2], 0.05);

            struct GuessPoint {
                double alpha, nu, rho;
            };

            // 起点1：热启动；起点2：强偏斜；起点3：平缓微笑
            std::vector<GuessPoint> candidates = {
                {baseAlpha, std::clamp(m_nu, 0.05, 2.0), std::clamp(m_rho, -0.85, 0.85)},
                {baseAlpha, 0.60, -0.65},
                {baseAlpha, 0.25, -0.15}
            };

            QuantLib::Array bestSolution;
            QuantLib::Real bestChiSq = std::numeric_limits<QuantLib::Real>::max();
            bool fitSuccess = false;

            // 4. 多起点轮询优化 (Multi-Start 核心循环)
            for (const auto &guess: candidates) {
                QuantLib::Array x(3);
                x[0] = guess.alpha;
                x[1] = guess.nu;
                x[2] = std::atanh(guess.rho); // 映射到无约束轴

                try {
                    QuantLib::Problem problem(costFunction, noConstraint, x);
                    lm.minimize(problem, endCriteria);

                    QuantLib::Array curSolution = problem.currentValue();

                    // 物理有效性初验
                    double curAlpha = std::max(curSolution[0], 1e-4);
                    double curNu = std::max(curSolution[1], 1e-4);
                    double curRho = std::tanh(curSolution[2]);

                    if (std::isnan(curAlpha) || std::isnan(curNu) || std::isnan(curRho)) {
                        continue;
                    }

                    // 计算当前解的残差平方和 (Chi-Square)
                    QuantLib::Array curResiduals = costFunction.values(curSolution);
                    QuantLib::Real curChiSq = QuantLib::DotProduct(curResiduals, curResiduals);

                    // 保留最优解
                    if (curChiSq < bestChiSq) {
                        bestChiSq = curChiSq;
                        bestSolution = curSolution;
                        fitSuccess = true;
                    }
                } catch (...) {
                    // 单一起点发散跳过，尝试下一个起点
                    continue;
                }
            }

            // 5. 最终结果落盘
            if (fitSuccess) {
                m_alpha = std::max(bestSolution[0], 1e-4);
                m_nu = std::max(bestSolution[1], 1e-4);
                m_rho = std::tanh(bestSolution[2]);
                m_rmse = std::sqrt(bestChiSq / strikesTemp.size());
            }
        }
    }
}
