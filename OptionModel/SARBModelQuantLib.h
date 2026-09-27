//
// Created by zhangyingwei on 2026/5/14.
//

#ifndef COSMOS_SARBMODEL_H
#define COSMOS_SARBMODEL_H


#include <any.hpp>

#include "KData.h"
#include <memory>
#include <ql/quantlib.hpp>
#include <ql/math/interpolations/sabrinterpolation.hpp>


namespace Cosmos {
    namespace OptionModel {

        class FastSABRCostFunction : public QuantLib::CostFunction {
            QuantLib::Real forward_, T_, beta_;
            const std::vector<QuantLib::Real>& strikes_;
            const std::vector<QuantLib::Real>& vols_;
            const std::vector<QuantLib::Real>& weights_;

        public:
            FastSABRCostFunction(QuantLib::Real forward, QuantLib::Real T, QuantLib::Real beta,
                                 const std::vector<QuantLib::Real>& strikes,
                                 const std::vector<QuantLib::Real>& vols,
                                 const std::vector<QuantLib::Real>& weights)
                : forward_(forward), T_(T), beta_(beta),
                  strikes_(strikes), vols_(vols), weights_(weights) {}

            // 返回残差向量 (优化器最小化该向量的 2-范数)
            QuantLib::Array values(const QuantLib::Array& x) const override {
                // 参数无约束映射：x[0] -> alpha, x[1] -> nu, x[2] -> 映射前的 rho
                QuantLib::Real alpha = std::max(x[0], 1e-4);
                QuantLib::Real nu    = std::max(x[1], 1e-4);
                // 用 tanh 将 rho 自然约束在 (-1, 1) 之间，无需外挂约束器
                QuantLib::Real rho   = std::tanh(x[2]);

                QuantLib::Array residuals(strikes_.size());
                for (size_t i = 0; i < strikes_.size(); ++i) {
                    QuantLib::Real modelVol = QuantLib::sabrVolatility(
                        strikes_[i], forward_, T_, alpha, beta_, nu, rho
                    );
                    residuals[i] = (modelVol - vols_[i]) * weights_[i];
                }
                return residuals;
            }

            QuantLib::Real value(const QuantLib::Array& x) const override {
                QuantLib::Array res = values(x);
                return QuantLib::DotProduct(res, res);
            }
        };

        class SARBModelQuantLib {
        public:
            SARBModelQuantLib(int tradingDay, int expireDay) : m_expireDay(expireDay),  m_tradingDay(tradingDay){
                m_optimizationMethod = boost::make_shared<QuantLib::LevenbergMarquardt>(1e-8, 1e-8,1e-8);
                m_endCriteria = boost::make_shared< QuantLib::EndCriteria>(1000, 200, 1e-8, 1e-8, 1e-8);
                auto diff = (Utils::intToSysDays(m_expireDay) - Utils::intToSysDays(m_tradingDay)).count();
                m_T = std::max(diff / 365.0, 1e-5);
            };

            void sarbFit(double forwardPrice,  std::map<int, KData::CallPutSeries *> *  callPutSeriesMap, int optionSeriesIndex);

            void getParameters(KData::SabrPRMT & sabrPrmt) {
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
            QuantLib::Real m_alpha{0.0};
            QuantLib::Real m_beta{0.99};
            QuantLib::Real m_rho{0.0};
            QuantLib::Real m_nu{0.4};
            QuantLib::Real m_rmse{0.0};

            QuantLib::Real m_fowardPrice{0.0};

            int m_useOptionNumb{4};
         //   bool m_isInitialized{true};


            std::vector<QuantLib::Real>  m_strikes;
            std::vector<QuantLib::Real>  m_volatilities;

            QuantLib::SABRInterpolation* m_sabrInterp{nullptr};

           boost::shared_ptr<QuantLib::LevenbergMarquardt> m_optimizationMethod;
           boost::shared_ptr<QuantLib::EndCriteria> m_endCriteria{nullptr};

            void _prepareSliceData(std::vector<QuantLib::Real>& strikes,  std::vector<QuantLib::Real>& volatilities,
                std::vector<QuantLib::Real>& vegas, double forwardPrice, const std::map<int, KData::CallPutSeries *> * callPutSeriesMap,
                int optionSeriesIndex) ;
        };
    }
}
#endif //COSMOS_SARBMODEL_H
