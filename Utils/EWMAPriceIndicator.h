//
// Created by zhangyw on 11/10/19.
//

#include "../Types/MarketData.h"

#ifndef HFT_MM_EWMAPRICEINDICATOR_H
#define HFT_MM_EWMAPRICEINDICATOR_H


namespace Cosmos{
    namespace Utils {
        class EWMAPriceIndicator{
        private:
            int m_peroid{20};
        public:

            double m_lastEWMA{0.0};

            void setEWMAValue(double marketPrice ){

                if (m_lastEWMA == 0) {
                    m_lastEWMA= marketPrice;
                }else if(marketPrice !=0){
                    m_lastEWMA =  (marketPrice + m_lastEWMA * (m_peroid - 1)) / m_peroid;
                }

            }

            void set_period(int period){
                m_peroid = period;
            }

            void reset(){
                m_lastEWMA = 0.0;
            }
        };


//        class ArbitrageSpread{
//        public:
//
//            EWMAPriceIndicator m_abtrgEWMA;
//
//            void onNewMarket(double inputAbtrgPrice, bool isTrade){
//                m_abtrgEWMA.setEWMAValue(inputAbtrgPrice);
//            }
//
//            void set_period(int period){
//                m_abtrgEWMA.peroid = period;
//            }
//
//            void reset(){
//                m_abtrgEWMA.reset();
//            }
//
//        private:
//        };
    }
}

#endif //HFT_MM_EWMAPRICEINDICATOR_H
