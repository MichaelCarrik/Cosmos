//
// Created by zhangyingwei on 2026/5/14.
//

#include "SARBModelQuantLib.h"
#include "KSeries.h"

namespace Cosmos {
    namespace OptionModel {
        void SARBModelQuantLib::_prepareSliceData(std::vector<QuantLib::Real> &strikes,
                                                  std::vector<QuantLib::Real> &volatilities,
                                                  double forwardPrice,
                                                  const std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                                  int optionSeriesIndex) {

            int idx = 0;
            for (auto itr = callPutSeriesMap->rbegin(); itr != callPutSeriesMap->rend(); ++itr) {
                if (itr->first <= forwardPrice) {
                    strikes.push_back(itr->first);
                    auto putSeries = itr->second->putSeries;
                    volatilities.push_back(putSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.IV);
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
                if (itr->first > forwardPrice) {
                    strikes.push_back(itr->first);
                    auto callSeries = itr->second->callSeries;
                    volatilities.push_back(callSeries->m_KDataVecs[optionSeriesIndex]->m_greeks.IV);
                    idx++;
                }

                if (idx >= m_useOptionNumb) {
                    break;
                }
            }

        };

        void SARBModelQuantLib::sarbFit(double forwardPrice, std::map<int, KData::CallPutSeries *> *callPutSeriesMap,
                                        int optionSeriesIndex) {
            std::vector<QuantLib::Real> strikesTemp;
            std::vector<QuantLib::Real> volatilitiesTemp;

            _prepareSliceData(strikesTemp, volatilitiesTemp, forwardPrice,
                                   callPutSeriesMap, optionSeriesIndex) ;

            m_fowardPrice = forwardPrice;

            if (m_sabrInterp == nullptr || m_strikes.size() != strikesTemp.size() ) {
                if (m_sabrInterp != nullptr) {
                    delete m_sabrInterp;
                    m_sabrInterp = nullptr;

                }

                auto minIV = std::min_element(volatilitiesTemp.begin(), volatilitiesTemp.end());
                m_alpha = std::max(*minIV, 0.15);

                m_strikes.clear();
                for (int i = 0; i < strikesTemp.size(); i++) {
                    m_strikes.push_back(strikesTemp[i]);
                }

                m_volatilities.clear();
                for (int i = 0; i < volatilitiesTemp.size(); i++) {
                    m_volatilities.push_back(volatilitiesTemp[i]);
                }
                try {
                    m_sabrInterp = new QuantLib::SABRInterpolation(
                       m_strikes.begin(), m_strikes.end(),
                       m_volatilities.begin(),
                       m_T, m_fowardPrice,
                       m_alpha, m_beta, m_nu, m_rho,
                       false, true,
                       false, false, true//, m_endCriteria, m_optimizationMethod
                       );
                }
                    catch (std::exception &e) {
                        m_sabrInterp = nullptr;
                     //   fprintf(stderr, " new SABRInterpolation  Error: %s\n", e.what());
                    }
            }else {
                for (int i = 0; i < strikesTemp.size(); i++) {
                    m_strikes[i] = strikesTemp[i];
                }
                for (int i = 0; i < volatilitiesTemp.size(); i++) {
                    m_volatilities[i] = volatilitiesTemp[i];
                }
            }

            if (m_sabrInterp != nullptr) {
                try {
                    m_sabrInterp->update();
                    m_alpha = m_sabrInterp->alpha();
                    if (std::isnan(m_alpha)) {
                        int a = 1;
                    }
                    m_beta = m_sabrInterp->beta();
                    m_nu = m_sabrInterp->nu();
                    m_rho = m_sabrInterp->rho();
                    m_rmse= m_sabrInterp->rmsError();

                 //   m_isInitialized = false;
                }catch (std::exception &e) {
               //    fprintf(stderr, " m_sabrInterp->update(); Error: %s\n", e.what());
                }
            }
        };
    }
}
