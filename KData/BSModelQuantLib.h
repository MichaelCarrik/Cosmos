//
// Created by Zhangyingwei on 2023/2/23.
//

#ifndef EQUITYOPTION_BSMODEL_H
#define EQUITYOPTION_BSMODEL_H

#include <time/calendars/target.hpp>
#include <ql/qldefines.hpp>
#include <ql/instruments/vanillaoption.hpp>
#include <ql/pricingengines/vanilla/analyticeuropeanengine.hpp>
#include <ql/pricingengines/vanilla/analyticeuropeanvasicekengine.hpp>
#include <termstructures/yield/flatforward.hpp>
#include <termstructures/volatility/equityfx/blackconstantvol.hpp>

namespace Cosmos {
    namespace KData {
        class BSModelQuantLib {
        public:
            BSModelQuantLib(char input_optionType, double strike, int tradingday, int expireday, double riskFreeRate){
                QuantLib::Option::Type optionType;
                if (input_optionType == 'C') {
                    optionType = QuantLib::Option::Call;
                } else if (input_optionType == 'P') {
                    optionType = QuantLib::Option::Put;
                }

                m_IV = new QuantLib::SimpleQuote(0.18);
                m_underlyPrice = new QuantLib::SimpleQuote(2702.0);
                QuantLib::Calendar calendar = QuantLib::TARGET();
                QuantLib::Date todaysDate(tradingday % 100, convetMonth((tradingday / 100) % 100), tradingday / 10000);
                QuantLib::Date maturity(expireday % 100, convetMonth((expireday / 100) % 100), expireday / 10000);
                QuantLib::Settings::instance().evaluationDate() = todaysDate;
                QuantLib::DayCounter dayCounter = QuantLib::Actual365Fixed();

                QuantLib::ext::shared_ptr<QuantLib::Exercise> europeanExercise(
                        new QuantLib::EuropeanExercise(maturity));

                QuantLib::ext::shared_ptr<QuantLib::StrikedTypePayoff> payoff(
                        new QuantLib::PlainVanillaPayoff(optionType, strike));

                m_europeanOption = new QuantLib::VanillaOption(payoff, europeanExercise);

                // bootstrap the yield/dividend/vol curves
                m_flatTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>(
                        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>(
                                new QuantLib::FlatForward(todaysDate, riskFreeRate, dayCounter)));
                m_flatDividendTS = QuantLib::Handle<QuantLib::YieldTermStructure>(
                        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>(
                                new QuantLib::FlatForward(todaysDate, 0.0, dayCounter)));

                auto ivpointer = QuantLib::ext::shared_ptr<QuantLib::Quote>(m_IV);
                QuantLib::Handle<QuantLib::Quote> IVH(ivpointer);

//    QuantLib::Handle<QuantLib::Quote> IVH(QuantLib::ext::shared_ptr<QuantLib::Quote>(new QuantLib::SimpleQuote(strike)));
                auto bcv = new QuantLib::BlackConstantVol(todaysDate, calendar, IVH, dayCounter);
                m_flatVolTS = QuantLib::Handle<QuantLib::BlackVolTermStructure>(
                        QuantLib::ext::shared_ptr<QuantLib::BlackVolTermStructure>(bcv));

                auto uppoint = QuantLib::ext::shared_ptr<QuantLib::Quote>(m_underlyPrice);
                QuantLib::Handle<QuantLib::Quote> underlyingH(uppoint);

                auto bsmp = new QuantLib::BlackScholesMertonProcess(underlyingH, m_flatDividendTS, m_flatTermStructure,
                                                                    m_flatVolTS);
                m_bsmProcess = new QuantLib::ext::shared_ptr<QuantLib::BlackScholesMertonProcess>(
                        bsmp);

                m_europeanOption->setPricingEngine(QuantLib::ext::shared_ptr<QuantLib::PricingEngine>(
                        new QuantLib::AnalyticEuropeanEngine(*m_bsmProcess)));
            };

            double calImpliedVol(double underlyPrice, double optionPrice){
                m_underlyPrice->setValue(underlyPrice);
                auto IV = m_europeanOption->impliedVolatility(optionPrice, *m_bsmProcess);
                return IV;
            };

            void calGreeks(double underlyPrice, double IV, double&delta, double& gamma, double& theta, double& vega){
                m_underlyPrice->setValue(underlyPrice);
                m_IV->setValue(IV);
                //auto npv = m_europeanOption->NPV();
                delta = m_europeanOption->delta();
                gamma = m_europeanOption->gamma();
                vega = m_europeanOption->vega() / 100;
                theta = m_europeanOption->theta() / 365;
            };

        private:
            QuantLib::Calendar calendar;

            QuantLib::Month convetMonth(int month){
                switch (month) {
                    case 1:
                        return QuantLib::Month::Jan;
                    case 2:
                        return QuantLib::Month::Feb;
                    case 3:
                        return QuantLib::Month::March;
                    case 4:
                        return QuantLib::Month::April;
                    case 5:
                        return QuantLib::Month::May;
                    case 6:
                        return QuantLib::Month::Jun;
                    case 7:
                        return QuantLib::Month::Jul;
                    case 8:
                        return QuantLib::Month::Aug;
                    case 9:
                        return QuantLib::Month::Sep;
                    case 10:
                        return QuantLib::Month::Oct;
                    case 11:
                        return QuantLib::Month::Nov;
                    case 12:
                        return QuantLib::Month::Dec;

                }
                assert(false);

            };

            QuantLib::VanillaOption *m_europeanOption{nullptr};

            QuantLib::Handle<QuantLib::YieldTermStructure> m_flatTermStructure;

            QuantLib::Handle<QuantLib::YieldTermStructure> m_flatDividendTS;

            QuantLib::Handle<QuantLib::BlackVolTermStructure> m_flatVolTS;
            QuantLib::SimpleQuote *m_underlyPrice{nullptr};
            QuantLib::SimpleQuote *m_IV{nullptr};
            QuantLib::ext::shared_ptr<QuantLib::BlackScholesMertonProcess> *m_bsmProcess{nullptr};
        };
    };

};


#endif //EQUITYOPTION_BSMODEL_H
