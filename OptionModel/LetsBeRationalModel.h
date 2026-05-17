//
// Created by zhangyingwei on 2026/5/13.
//

#ifndef LETSBERATIONALDEMO_LETSBERATIONAL_H
#define LETSBERATIONALDEMO_LETSBERATIONAL_H
#include "lets_be_rational.h"
#include "normaldistribution.h"
#include <chrono>
#include <cassert>

#include <stdexcept>

#include "../KData/KData.h"

namespace Cosmos {
    namespace OptionModel {
            class LetsBeRationalModel {
            public:
                LetsBeRationalModel(char input_optionType, double strike, int tradingday, int expireday, double riskFreeRate): m_optionType{input_optionType}, m_strikePrice{strike},
                m_tradingDay(tradingday), m_expireDay(expireday), m_riskFreeRate(riskFreeRate) {
                    auto diff = (Utils::intToSysDays(m_expireDay) - Utils::intToSysDays(m_tradingDay)).count();
                    m_T = diff / 365.0;
                    if (m_optionType == 'C') {
                        m_optionTypeDouble = 1.0;
                    }else if (m_optionType == 'P') {
                        m_optionTypeDouble = -1.0;
                    }else {
                        assert(false);
                    }
                    m_discountFactor = std::exp(-m_riskFreeRate * m_T);
                }  ;

                double calImpliedVol(double underlyPrice, double optionPrice) {
                    auto iv = LetsBeRational::ImpliedBlackVolatility(optionPrice, underlyPrice, m_strikePrice, m_T, m_optionTypeDouble);
                    if (std::isinf(iv) == true ) {
                        throw std::runtime_error("ImpliedVolatility is infinity ");
                    }else if (std::isnan(iv) == true) {
                        throw std::runtime_error("ImpliedVolatility is nan");
                    }
                    return iv;
                };

                void calGreeks(double underlyPrice, KData::Greeks & greeks) {
                    double sigma_sqrt_T = greeks.IV * std::sqrt(m_T);
                    double d1 = (std::log(underlyPrice / m_strikePrice) + 0.5 * greeks.IV * greeks.IV * m_T) / sigma_sqrt_T;
                    double d2 = d1 - sigma_sqrt_T;
                    double pdf_d1 = LetsBeRational::norm_pdf(d1);
                    double cdf_d1 = LetsBeRational::norm_cdf(d1);

                    greeks.delta = (m_optionTypeDouble > 0) ? (m_discountFactor * cdf_d1)
                                      : (m_discountFactor * (cdf_d1 - 1.0));

                    greeks.gamma = (pdf_d1 * m_discountFactor) / (underlyPrice * greeks.IV * std::sqrt(m_T));

                    greeks.vega = LetsBeRational::Vega(underlyPrice, m_strikePrice,greeks.IV,m_T);
                    greeks.vega = greeks.vega/100;

                    double term1 = -(underlyPrice * pdf_d1 * greeks.IV * m_discountFactor) / (2.0 * std::sqrt(m_T));

                    if (m_optionTypeDouble > 0) {
                        // Call Theta
                        greeks.theta = term1 - m_riskFreeRate * m_strikePrice * std::exp(-m_riskFreeRate * m_T) * LetsBeRational::norm_cdf(d2);
                    } else {
                        // Put Theta
                        greeks.theta = term1 + m_riskFreeRate * m_strikePrice * std::exp(-m_riskFreeRate * m_T) * LetsBeRational::norm_cdf(-d2);
                    }
                    greeks.theta = greeks.theta /365;

                    greeks.vanna = -m_discountFactor * pdf_d1 * d2 / greeks.IV;
                    greeks.vanna =  greeks.vanna/100.0;

                    greeks.volga = greeks.vega * d1 * d2 / greeks.IV;
                    greeks.volga = greeks.volga / 10000.0;
                };

            private:

                double m_T{0.0};
                double m_strikePrice{0.0};
                char m_optionType{'N'};
                double m_optionTypeDouble{0.0};
                int m_tradingDay{0};
                int m_expireDay{0};
                double m_riskFreeRate{0.0};
                double m_discountFactor{0.0};


            };

    }
}


#endif //LETSBERATIONALDEMO_LETSBERATIONAL_H
