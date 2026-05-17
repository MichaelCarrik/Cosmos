//
// Created by zhangyingwei on 2026/5/14.
//

#ifndef COSMOS_SARBMODEL_H
#define COSMOS_SARBMODEL_H


#include <any.hpp>

#include "KData.h"

#include <ql/quantlib.hpp>
#include <ql/math/interpolations/sabrinterpolation.hpp>


namespace Cosmos {
    namespace OptionModel {
        class SARBModelQuantLib {
        public:
            SARBModelQuantLib(int tradingDay, int expireDay) : m_expireDay(expireDay),  m_tradingDay(tradingDay){
                m_endCriteria = new  QuantLib::EndCriteria(1000, 100, 1e-6, 1e-6, 1e-6);
                auto diff = (Utils::intToSysDays(m_expireDay) - Utils::intToSysDays(m_tradingDay)).count();
                m_T = std::max(diff / 365.0, 1e-5);
            };

            void sarbFit(double forwardPrice,  std::map<int, KData::CallPutSeries *> *  callPutSeriesMap, int optionSeriesIndex);

            void getParameters(KData::SabrPRMT & sabrPrmt) {
                sabrPrmt.alpha = m_alpha;
                sabrPrmt.beta = m_beta;
                sabrPrmt.rho = m_rho;
                sabrPrmt.nu = m_nu;
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

            QuantLib::Real m_fowardPrice{0.0};

            int m_useOptionNumb{4};
         //   bool m_isInitialized{true};


            std::vector<QuantLib::Real>  m_strikes;
            std::vector<QuantLib::Real>  m_volatilities;

            QuantLib::SABRInterpolation* m_sabrInterp{nullptr};

            QuantLib::LevenbergMarquardt m_optimizationMethod;
            QuantLib::EndCriteria *m_endCriteria{nullptr};

            void _prepareSliceData(std::vector<QuantLib::Real>& strikes,  std::vector<QuantLib::Real>& volatilities,
            double forwardPrice, const std::map<int, KData::CallPutSeries *> *  callPutSeriesMap, int optionSeriesIndex) ;
        };
    }
}
#endif //COSMOS_SARBMODEL_H
