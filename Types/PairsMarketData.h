//
// Created by Zhangyingwei on 2022/7/25.
//

#ifndef TRENDFOLLOW_ARBITRAGEMARKET_H
#define TRENDFOLLOW_ARBITRAGEMARKET_H
#include "../Utils//EWMAPriceIndicator.h"

namespace TrendFollow{
    namespace Types {
        struct PairsMarketData {
            double mixBidPrice{0.0};
            double mixAskPrice{0.0};
            int mixBidVolume{0};
            int mixAskVolume{0};
            bool isBidValid{false};
            bool isAskValid{false};
            double solidBidPrice{0.0};
            double solidAskPrice{0.0};
            int solidBidVolume{0};
            int solidAskVolume{0};
      //      Utils::EWMAPriceIndicator ewmaAbtrgPrice;

            void _calMarketPrice(const Types::MarketData* hedgeMD, const Types::MarketData* tradeMD,const Types::MarketData* abtrgMD, int tickSize) {


                if (hedgeMD->bidVolume[0] > 0 && tradeMD->askVolume[0] > 0) {
                    solidBidPrice = hedgeMD->bidPrice[0] - tradeMD->askPrice[0];
                    solidBidVolume = std::min(hedgeMD->bidVolume[0], tradeMD->askVolume[0]);
                    isBidValid = true;
                }

                if (hedgeMD->askVolume[0] > 0 && tradeMD->bidVolume[0] > 0) {
                    solidAskPrice = hedgeMD->askPrice[0] - tradeMD->bidPrice[0];
                    solidAskVolume = std::min(hedgeMD->askVolume[0], tradeMD->bidVolume[0]);
                    isAskValid = true;
                }

                mixBidPrice= solidBidPrice;
                mixAskPrice = solidAskPrice;
                mixBidVolume = solidBidVolume;
                mixAskVolume = solidAskVolume;

	//	fprintf(stderr, "%.3f, %.3f, %.3f, %.3f\n", mixBidPrice, mixAskPrice, abtrgMD->bidPrice[0] ,abtrgMD->askPrice[0]);

                if (abtrgMD->bidVolume[0] > 0 && mixBidPrice + 0.00001 < abtrgMD->bidPrice[0] +tickSize && abtrgMD->bidPrice[0] + 0.00001 < mixAskPrice) {
                    //   bflag =1;
                    if(abs(mixBidPrice - abtrgMD->bidPrice[0]) < 0.00001){
                        //      bflag =3;
                        mixBidVolume += abtrgMD->bidVolume[0];
                    }else{
                        mixBidVolume = abtrgMD->bidVolume[0];
                    }
                    mixBidPrice = abtrgMD->bidPrice[0];
                }

                if (abtrgMD->askVolume[0] > 0 && mixAskPrice - 0.00001 > abtrgMD->askPrice[0] - tickSize && abtrgMD->askPrice[0] - 0.00001 > mixBidPrice) {
                    //    sflag=1;
                    if(abs(mixAskPrice - abtrgMD->askPrice[0]) < 0.00001){
                        //        sflag=3;
                        mixAskVolume += abtrgMD->askVolume[0];
                    }else{
                        mixAskVolume = abtrgMD->askVolume[0];
                    }
                    mixAskPrice = abtrgMD->askPrice[0];
                }

            //    double theoryPrice = (mixBidPrice * mixAskVolume + mixAskPrice * mixBidVolume)/ (mixBidVolume + mixAskVolume);

           //     this->ewmaAbtrgPrice.setEWMAValue(theoryPrice);
            }

        };
    }
}
#endif //TRENDFOLLOW_ARBITRAGEMARKET_H
