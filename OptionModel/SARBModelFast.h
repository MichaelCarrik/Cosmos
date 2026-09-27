//
// Created by zhangyingwei on 2026/9/26.
//

#ifndef COSMOS_SARBMODELFAST_H
#define COSMOS_SARBMODELFAST_H


#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include "KData.h"
#include <unsupported/Eigen/NonLinearOptimization>


namespace Cosmos {
    namespace OptionModel {
        inline double sabr_vol(double F, double K, double T, double alpha, double beta, double nu, double rho) {
            // 保护：极端情况回退
            if (alpha <= 0.0 || nu < 0.0 || std::abs(rho) >= 1.0) return 0.2;

            // ATM 情况快速特判，避免 0/0 奇异点
            if (std::abs(F - K) < 1e-7) {
                double f_b = std::pow(F, 1.0 - beta);
                double term1 = ((1.0 - beta) * (1.0 - beta) / 24.0) * (alpha * alpha) / (f_b * f_b);
                double term2 = 0.25 * (rho * beta * nu * alpha) / f_b;
                double term3 = ((2.0 - 3.0 * rho * rho) / 24.0) * (nu * nu);
                return (alpha / f_b) * (1.0 + (term1 + term2 + term3) * T);
            }

            double fK = F * K;
            double sqrt_fK = std::sqrt(fK);
            double fK_b = std::pow(sqrt_fK, 1.0 - beta);
            double logFK = std::log(F / K);
            double z = (nu / alpha) * fK_b * logFK;

            // 数值稳定性保护
            double x_z;
            if (std::abs(z) < 1e-5) {
                x_z = 1.0; // z / x_z -> 1
            } else {
                double s = std::sqrt(1.0 - 2.0 * rho * z + z * z);
                double denominator = (1.0 - rho);
                x_z = std::log((s + z - rho) / denominator);
                if (std::abs(x_z) < 1e-12) return 0.2;
            }

            double z_ratio = (std::abs(z) < 1e-5) ? 1.0 : (z / x_z);

            double term1 = ((1.0 - beta) * (1.0 - beta) / 24.0) * (alpha * alpha) / (fK_b * fK_b);
            double term2 = 0.25 * (rho * beta * nu * alpha) / fK_b;
            double term3 = ((2.0 - 3.0 * rho * rho) / 24.0) * (nu * nu);
            double numerator = alpha * (1.0 + (term1 + term2 + term3) * T);

            double logFK_2 = logFK * logFK;
            double term_d1 = ((1.0 - beta) * (1.0 - beta) / 24.0) * logFK_2;
            double term_d2 = (std::pow(1.0 - beta, 4) / 1920.0) * logFK_2 * logFK_2;
            double denominator = fK_b * (1.0 + term_d1 + term_d2);

            return z_ratio * (numerator / denominator);
        }

        // 2. 根据 ATM 波动率 3 步牛顿迭代求解 Alpha
        inline double solve_alpha_atm(double F, double T, double beta, double nu, double rho, double atm_vol) {
            double alpha = atm_vol * std::pow(F, 1.0 - beta); // 优质初值
            for (int i = 0; i < 3; ++i) {
                double v = sabr_vol(F, F, T, alpha, beta, nu, rho);
                double diff = v - atm_vol;
                if (std::abs(diff) < 1e-6) break;
                // 单边有限差分导数
                double h = 1e-5;
                double v_h = sabr_vol(F, F, T, alpha + h, beta, nu, rho);
                double d_alpha = (v_h - v) / h;
                alpha -= diff / d_alpha;
            }
            return std::max(alpha, 1e-4);
        }

        // ==========================================
        // 3. Eigen 优化器适配 Functor
        // ==========================================
        struct SABR2DFunctor {
            const double F, T, beta, atm_vol;
            const std::vector<double> &strikes;
            const std::vector<double> &market_vols;
            const std::vector<double>& sqrt_weights;

            using Scalar = double;
            using InputType = Eigen::VectorXd;
            using ValueType = Eigen::VectorXd;
            using JacobianType = Eigen::MatrixXd;

            enum {
                InputsAtCompileTime = 2, // 优化变量数量: x[0]->rho, x[1]->nu
                ValuesAtCompileTime = Eigen::Dynamic
            };

            SABR2DFunctor(double F_, double T_, double beta_, double atm_vol_,
                          const std::vector<double> &k, const std::vector<double> &v, const std::vector<double>& w)
                : F(F_), T(T_), beta(beta_), atm_vol(atm_vol_), strikes(k), market_vols(v),sqrt_weights(w) {
            }

            int inputs() const { return 2; }
            int values() const { return static_cast<int>(strikes.size()); }

            // 计算残差向量
            int operator()(const Eigen::VectorXd &x, Eigen::VectorXd &fvec) const {
                // 无约束空间映射回物理边界
                double rho = std::tanh(x[0]);
                double nu = std::exp(x[1]);
                double alpha = solve_alpha_atm(F, T, beta, nu, rho, atm_vol);

                for (size_t i = 0; i < strikes.size(); ++i) {
                    double model_vol = sabr_vol(F, strikes[i], T, alpha, beta, nu, rho);
                    fvec[i] = (model_vol - market_vols[i]) * sqrt_weights[i];
                }
                return 0;
            }
        };

        class SABRModelFast {
        public:
            SABRModelFast(int tradingDay, int expireDay) : m_expireDay(expireDay), m_tradingDay(tradingDay) {
                auto diff = (Utils::intToSysDays(m_expireDay) - Utils::intToSysDays(m_tradingDay)).count();
                m_T = std::max(diff / 365.0, 1e-5);
            };



            void sarbFit(double forwardPrice, std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                         int optionSeriesIndex) ;

            void getParameters(KData::SabrPRMT &sabrPrmt) {
                sabrPrmt.alpha = m_alpha;
                sabrPrmt.beta = m_beta;
                sabrPrmt.rho = m_rho;
                sabrPrmt.nu = m_nu;
                if (std::isnan(m_rmse) == true || std::isinf(m_rmse) == true) {
                    m_rmse = 0.0;
                }

                sabrPrmt.rmse = m_rmse;
            }

        private:
            int m_recordUnderlySeriesIndex{0};
            int underlyTodayBeginIndex{0};
            int m_tradingDay{0};
            int m_expireDay{0};
            double m_T{0.0};
            double m_alpha{0.0};
            double m_beta{0.99};
            double m_rho{0.0};
            double m_nu{0.4};
            double m_rmse{0.0};

            double m_fowardPrice{0.0};

            int m_useOptionNumb{4};
            //   bool m_isInitialized{true};


            void _prepareSliceData(std::vector<double> &strikes, std::vector<double> &volatilities,std::vector<double> &vegas,
                                   double forwardPrice, const std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                   int optionSeriesIndex) ;
        };
    }
}

#endif //COSMOS_SARBMODELFAST_H
