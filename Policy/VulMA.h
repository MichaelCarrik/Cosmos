//
// Created by zhangyw on 2/14/21.
//

#ifndef Cosmos_VULMA_H
#define Cosmos_VULMA_H
namespace Cosmos {
    namespace Policy {
        class VulMA : public LPolicy {
        private:
             Types::KPeriod m_kperiod;
            int m_length;
            double m_alpha;
            double m_mark;


            int m_lastBarIndex{0};
            int m_marketPosition;
            int m_offset;

             KData::KSeries *m_mainKSeries{nullptr};
             KData::KSeries *m_dayKSeries{nullptr};

	//    int m_isHalf{0};

            int m_lots{0};
            int m_onoff{0};
            int m_tradeNum{0};
            std::vector<double> m_dayHighestVec;
            std::vector<double> m_dayLowestVec;
            std::vector<double> m_dayCloseVec;
            double m_band;
            double m_upBand;
            double m_downBand;
            double m_minsMA;
            double m_winExistLong{0.0};
            double m_winExistShort{99999.9};

        public:
            VulMA( Types::KPeriod kperiod, int length, double alpha, double mark, int lots,
                  std::string &policyName,
                  std::string &engineName, int tradingday) :
                    m_kperiod(kperiod), m_lots(lots), m_length(length), m_alpha(alpha), m_mark(mark) {
                m_policyName = policyName;
                m_engineName = engineName;
                m_tradingday = tradingday;
                spdlog::info(
                        "createPolicy m_engineName={}, m_policyName={}, kperiod={}  m_length={}, m_alpha={}, m_mark={}, "
                        "lots={}, tradingday={}", engineName, policyName, (int) kperiod, m_length, m_alpha, m_mark,
                        lots, m_tradingday);
            }

            ~VulMA() {

            }

            virtual void start() override {


                char configPath[256]{""};
                sprintf(configPath, "./logs/policy/%s_%s.txt", m_engineName.c_str(), m_policyName.c_str());
                m_signalPrice = atof(this->GetLastValueFromFile(configPath, "signalPrice").c_str());
                m_winExistLong = atof(this->GetLastValueFromFile(configPath, "winExistLong").c_str());
                m_winExistShort = atof(this->GetLastValueFromFile(configPath, "winExistShort").c_str());
                m_winExistShort = m_winExistShort > 0.0001 ? m_winExistShort : 999999.9;
                m_marketPosition = atoi(this->GetLastValueFromFile(configPath, "marketPosition").c_str());
		//m_isHalf = atoi(this->GetLastValueFromFile(configPath, "isHalf").c_str());
                m_targetPosition = atoi(this->GetLastValueFromFile(configPath, "targetPosition").c_str());
                m_preTargetPosition = m_targetPosition;
                m_changeType = (Types::ChangeContract) atoi(
                        this->GetLastValueFromFile(configPath, "changeType").c_str());
                m_changeStatus = (Types::ChangeStatus) atoi(
                        this->GetLastValueFromFile(configPath, "changeStatus").c_str());
                fprintf(stderr,
                        "[%s_%s] start, m_kperiod=%d,  m_lots=%d, m_length=%d, m_offset=%d, m_winExistLong=%f, m_winExistShort=%f, m_signalPrice=%f,"// m_isHalf=%d, 
                        "m_marketPosition=%d, m_targetPosition=%d, m_preTargetPosition=%d, m_changeStatus=%d, m_changeType=%d\n",
                        m_engineName.c_str(), m_policyName.c_str(), m_kperiod, m_lots, m_length, m_offset,
                        m_winExistLong, m_winExistShort, m_signalPrice, //m_isHalf,
                        m_marketPosition, m_targetPosition, m_preTargetPosition, (int) m_changeStatus,
                        (int) m_changeType);

            };


            virtual void runTick(const  Types::MarketData *pMD) override {

                if (m_lastBarIndex == 0 and m_mainKSeries->m_seriesIndex > m_lastBarIndex) {

                    for (auto i = 0; i < m_dayKSeries->m_seriesIndex; i++) {
                        auto dayBar = m_dayKSeries->m_kseries[i];
                        if (dayBar->m_tradingday < m_tradingday) {
                            m_dayHighestVec.emplace_back(dayBar->m_high);
                            m_dayLowestVec.emplace_back(dayBar->m_low);
                            m_dayCloseVec.emplace_back(dayBar->m_close);
                        }
                    }

                    double highest = 9999.0, lowest = 0.0;
                    if (m_dayHighestVec.size() > 0) {
                        if (m_dayHighestVec.size() < m_mark) {
                            highest = *std::max_element(std::begin(m_dayHighestVec), std::end(m_dayHighestVec));
                            lowest = *std::min_element(std::begin(m_dayLowestVec), std::end(m_dayLowestVec));
                        } else {
                            highest = *std::max_element(std::end(m_dayHighestVec) - m_mark, std::end(m_dayHighestVec));
                            lowest = *std::min_element(std::end(m_dayLowestVec) - m_mark, std::end(m_dayLowestVec));
                        }
                        m_band = (highest - lowest) * m_alpha / std::sqrt(m_mark);
                        double dayOpen = m_dayKSeries->m_kseries[m_dayKSeries->m_seriesIndex]->m_open;
	                double dayClose = m_dayCloseVec[m_dayCloseVec.size()-1];
                        m_upBand = (dayClose + dayOpen)*0.5 + m_band;
                        m_downBand = (dayClose + dayOpen)*0.5 - m_band;
                    } else {
                        m_band = (highest + lowest) * 0.5;
                        m_upBand = highest;
                        m_downBand = lowest;
                    }

                    if (m_downBand - m_band > m_winExistLong) {
                        m_winExistLong = m_downBand - m_band;
                    }

                    if (m_upBand + m_band < m_winExistShort) {
                        m_winExistShort = m_upBand + m_band;
                    }


                    int beginI = 0, endI = 0;
                    if (m_dayCloseVec.size() < m_length) {
                        beginI = 0;
                        endI = m_dayCloseVec.size() - 1;

                    } else {
                        beginI = m_dayCloseVec.size() - m_length;
                        endI = m_dayCloseVec.size() - 1;
                    }

                    m_minsMA = Indicator::MA(m_dayCloseVec, beginI, endI);


                    if (m_mainKSeries->m_seriesIndex > 0 and m_dayCloseVec.size() > 0) {
                        auto lastBar = m_mainKSeries->m_kseries[m_mainKSeries->m_seriesIndex - 1];
                        auto barIndex = m_mainKSeries->m_seriesIndex - 1;
                        m_configLog->info(
                                "{}, {}, {}, {}, close={:.3f}, open={:.3f}, m_band={:.3f}, "
                                "m_upBand={:.3f}, m_downBand={:.3f}, "
                                "winExistLong={:.3f}, winExistShort={:.3f}, m_ma={:.3f}, "
                                "onoff={}, marketPosition={}, signalPrice={:.3f}, " // m_isHalf={}, 
                                "targetPosition={}, changeStatus={}, changeType={}",
                                m_symbol->instrumentID.data(), lastBar->m_tradingday,
                                lastBar->m_updateTime.data(), pMD->updateTime.data(),
                                lastBar->m_close, lastBar->m_open, m_band, m_upBand, m_downBand,
                                m_winExistLong, m_winExistShort, m_minsMA, m_onoff,
                                m_marketPosition, m_signalPrice, //m_isHalf, 
				m_targetPosition, (int) m_changeStatus, (int) m_changeType);
                    }

                } else if (m_lastBarIndex < m_mainKSeries->m_seriesIndex and m_dayCloseVec.size() > 0) {

                    auto lastBar = m_mainKSeries->m_kseries[m_lastBarIndex];


                    if (lastBar->m_close > m_minsMA) {
                        m_onoff = 1;
                    } else if (lastBar->m_close < m_minsMA) {
                        m_onoff = -1;
                    }


                    if (m_marketPosition > 0 &&
                               (lastBar->m_close <= m_downBand or lastBar->m_close <= m_winExistLong)) {  //long close
                        ++m_tradeNum;
                        m_marketPosition = 0;
                        m_targetPosition = 0;
                        m_signalPrice = lastBar->m_close;
                  //      m_onoff = 0;
                        m_winExistLong = 0.0;
		  //	m_isHalf=false;
                    } else if (m_marketPosition < 0 &&
                               (lastBar->m_close >= m_upBand or lastBar->m_close >= m_winExistShort)) {  //short close
                        ++m_tradeNum;
                        m_marketPosition = 0;
                        m_targetPosition = 0;
                        m_signalPrice = lastBar->m_close;
                  //      m_onoff = 0;
                        m_winExistShort = 0.0;
		 //	m_isHalf=false;
                    }
		 /*else if(m_marketPosition > 0 && lastBar->m_close > m_signalPrice*1.02 && m_isHalf == false){
		           m_isHalf=true;
		    }else if(m_marketPosition < 0 && lastBar->m_close < m_signalPrice*0.98 && m_isHalf == false){
                           m_isHalf=true;
                    } */


                    if (m_tradeNum < 10 && m_marketPosition == 0) {
                        if (lastBar->m_close >= m_upBand && m_onoff == 1) //&& lastBar->m_close > lastBar->m_open && m_onoff == 1) //long open
                        {
                            ++m_tradeNum;
                            m_marketPosition = 1;
                            m_targetPosition = m_lots;
                            m_signalPrice = lastBar->m_close;
                  //          m_onoff = 0;
                            m_winExistLong = m_downBand - m_band;
		  //	    m_isHalf=false;

                        } else if (lastBar->m_close <= m_downBand && m_onoff == -1)//&& lastBar->m_close < lastBar->m_open && m_onoff == -1)//short open
                        {
                            ++m_tradeNum;
                            m_marketPosition = -1;
                            m_targetPosition = -m_lots;
                            m_signalPrice = lastBar->m_close;
                   //         m_onoff = 0;
                            m_winExistShort = m_upBand + m_band;
		   //	    m_isHalf=false;

                        }
                    }


                    if (this->m_targetPosition == this->m_preTargetPosition && this->m_targetPosition == 0) {
                        m_signalPrice = 0.0;
                    }

                    if (this->m_targetPosition > 0 && this->m_targetPosition != m_lots){// && m_isHalf ==false) {   // for risk adjust
                        this->m_targetPosition = m_lots;
                    }/*else if (this->m_targetPosition > 0 && this->m_targetPosition != 0.5*m_lots && m_isHalf ==true) {   // for risk adjust
                        this->m_targetPosition = round(0.5*m_lots);
                    }*/ 
		    else if (this->m_targetPosition < 0 && this->m_targetPosition != -m_lots){// && m_isHalf ==false) {
                        this->m_targetPosition = -m_lots;
                    }/*else if (this->m_targetPosition < 0 && this->m_targetPosition != -0.5*m_lots && m_isHalf ==true) {
                        this->m_targetPosition = - round(0.5*m_lots);
                    }*/


                    m_configLog->info(
                            "{}, {}, {}, {}, close={:.3f}, open={:.3f}, m_band={:.3f}, m_upBand={:.3f}, m_downBand={:.3f}, "
                            "winExistLong={:.3f}, winExistShort={:.3f}, m_ma={:.3f}, "
                            "onoff={}, marketPosition={}, signalPrice={:.3f}, "// m_isHalf={}, 
                            "targetPosition={}, changeStatus={}, changeType={}",
                            m_symbol->instrumentID.data(), lastBar->m_tradingday,
                            lastBar->m_updateTime.data(), pMD->updateTime.data(),
                            lastBar->m_close, lastBar->m_open, m_band, m_upBand, m_downBand,
                            m_winExistLong, m_winExistShort, m_minsMA, m_onoff,
                            m_marketPosition, m_signalPrice, //m_isHalf, 
			    m_targetPosition,    (int) m_changeStatus, (int) m_changeType);
                    m_configLog->flush();
                }
                m_lastBarIndex = m_mainKSeries->m_seriesIndex;
            };

            virtual void switchSymbol() override {

                m_lastBarIndex = 0;
                //        m_winExistLong=0.0;
                //        m_winExistShort =99999.9;
                m_dayHighestVec.clear();
                m_dayLowestVec.clear();
                m_dayCloseVec.clear();
                auto itr = m_symbol->m_kSeriesMap->find(m_kperiod);
                m_mainKSeries = itr->second;

                itr = m_symbol->m_kSeriesMap->find(Types::KPeriod::D1);
                m_dayKSeries = itr->second;
            }


        };
    }
}
#endif //Cosmos_VULMA_H
