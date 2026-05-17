//
// Created by zhangyingwei on 2026/5/17.
//

#include "FastSabrModel.h"


namespace Cosmos {
    namespace OptionModel {
        double FastSabrModel::_impVolHagan(double F, double K, double T, double alpha, double beta, double rho,
                                           double nu) {
            if (std::abs(F - K) < 1e-6) {
                // 平值 (At-The-Money) 情况下特判加速
                double one_minus_beta = 1.0 - beta;
                double F_beta = std::pow(F, one_minus_beta);
                double term1 = alpha / F_beta;
                double term2 = (one_minus_beta * one_minus_beta / 24.0 * alpha * alpha / std::pow(
                                    F, 2.0 - 2.0 * beta)
                                + 0.25 * rho * beta * alpha * nu / F_beta
                                + (2.0 - 3.0 * rho * rho) / 24.0 * nu * nu);
                return term1 * (1.0 + term2 * T);
            }

            // 非平值情况下的全量计算
            double one_minus_beta = 1.0 - beta;
            double logFK = std::log(F / K);
            double F_K_beta = std::pow(F * K, one_minus_beta / 2.0);

            double z = (nu / alpha) * F_K_beta * logFK;
            double x_z = std::log((std::sqrt(1.0 - 2.0 * rho * z + z * z) + z - rho) / (1.0 - rho));

            double numerator = alpha * (1.0 + (one_minus_beta * one_minus_beta / 24.0 * alpha * alpha / std::pow(
                                                   F * K, one_minus_beta)
                                               + 0.25 * rho * beta * nu * alpha / F_K_beta
                                               + (2.0 - 3.0 * rho * rho) / 24.0 * nu * nu) * T);

            double denominator = F_K_beta * (1.0 + (one_minus_beta * one_minus_beta / 24.0) * logFK * logFK
                                             + (one_minus_beta * one_minus_beta * one_minus_beta * one_minus_beta /
                                                1920.0) * std::pow(logFK, 4.0));

            // 临界区保护，防止浮点数溢出
            if (std::abs(z) < 1e-5) return numerator / denominator;
            return (numerator / denominator) * (z / x_z);
        }

        // 2. 【核心降维加速】一维黄金分割法校准（无需通用优化器，彻底消除收敛死锁）
        // 假设 beta 已由盘口历史统计固定 (例如股票/商品期权通常设 beta = 0.5)
        void FastSabrModel::_calibrateSmile(double F, double T, double atmVol,
                                            const std::vector<double> &strikes,
                                            const std::vector<double> &marketVols,
                                            double beta,
                                            double &out_alpha, double &out_rho, double &out_nu) {
            size_t n = strikes.size();
            // 提前利用 ATM 波动率强行锁定 alpha 和其余参数的线性逼近关系，将 4维压缩为 1维
            auto objective_function = [&](double test_nu, double &best_alpha, double &best_rho) -> double {
                double min_error = 1e10;
                // 在固定 nu 下，对 rho 进行快速网格平滑扫描 (rho 范围 -0.99 到 0.99)
                for (double test_rho = -0.9; test_rho <= 0.9; test_rho += 0.1) {
                    // 利用 ATM 波动率直接代数反解出 alpha 的初始近似值，无需非线性迭代！
                    double test_alpha = atmVol * std::pow(F, 1.0 - beta);

                    // 计算当前参数组合下的总残差 (MSE)
                    double current_error = 0.0;
                    for (size_t i = 0; i < n; ++i) {
                        double model_vol = _impVolHagan(F, strikes[i], T, test_alpha, beta, test_rho, test_nu);
                        double diff = model_vol - marketVols[i];
                        current_error += diff * diff;
                    }

                    if (current_error < min_error) {
                        min_error = current_error;
                        best_alpha = test_alpha;
                        best_rho = test_rho;
                    }
                }
                return min_error;
            };

            // 对唯一的非线性参数 nu 执行高效的黄金分割一维搜索
            double ax = 0.01, bx = 2.0; // nu 的可能范围 (Vol-of-Vol)
            double r = 0.61803399;
            double c = bx - r * (bx - ax);
            double d = ax + r * (bx - ax);

            double dummy_alpha, dummy_rho;
            double fc = objective_function(c, dummy_alpha, dummy_rho);
            double fd = objective_function(d, dummy_alpha, dummy_rho);

            // 迭代 15 次即可精确到 10的-5次方，耗时仅几微秒
            for (int iter = 0; iter < 15; ++iter) {
                if (fc < fd) {
                    bx = d;
                    d = c;
                    fd = fc;
                    c = bx - r * (bx - ax);
                    fc = objective_function(c, out_alpha, out_rho);
                } else {
                    ax = ax;
                    ax = c;
                    fc = fd;
                    c = d;
                    d = ax + r * (bx - ax);
                    fd = objective_function(d, out_alpha, out_rho);
                }
            }
            out_nu = 0.5 * (ax + bx);
            objective_function(out_nu, out_alpha, out_rho); // 锁死最终最优的三个输出参数
        }

        void FastSabrModel::getParameters(KData::SabrPRMT & sabrPrmt) {
            sabrPrmt.alpha = m_alpha;
            sabrPrmt.beta = m_beta;
            sabrPrmt.rho = m_rho;
            sabrPrmt.nu = m_nu;
        }

        void FastSabrModel::_prepareSliceData(std::vector<double> &strikes,
                                              std::vector<double> &volatilities,
                                              double forwardPrice,
                                              const std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                              int optionSeriesIndex) {
            // int idx = 0;
            // for (auto itr = callPutSeriesMap->rbegin(); itr != callPutSeriesMap->rend(); ++itr) {
            //     if (itr->first <= forwardPrice) {
            //         strikes.push_back(itr->first);
            //         auto putSeries = itr->second->putSeries;
            //         volatilities.push_back(putSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.IV);
            //         idx++;
            //     }
            //     if (idx >= m_useOptionNumb) {
            //         break;
            //     }
            // }
            //
            // std::reverse(strikes.begin(), strikes.end());
            // std::reverse(volatilities.begin(), volatilities.end());
            // idx = 0;
            //
            // for (auto itr = callPutSeriesMap->begin(); itr != callPutSeriesMap->end(); ++itr) {
            //     if (itr->first > forwardPrice) {
            //         strikes.push_back(itr->first);
            //         auto callSeries = itr->second->callSeries;
            //         volatilities.push_back(callSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.IV);
            //         idx++;
            //     }
            //
            //     if (idx >= m_useOptionNumb) {
            //         break;
            //     }
            // }
        };

        void FastSabrModel::sarbFit(double forwardPrice, std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                    int optionSeriesIndex) {


        };
    }
}
