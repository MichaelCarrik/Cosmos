//
// Created by zhangyingwei on 2026/1/20.
//


#include "BSModelQuantLib.h"






int main(int argc, char* argv[]) {
    auto BSModelQuantLib = new  KData::BSModelQuantLib('C',166000 , 20260120,
                                                           20260206, 0.023);
    auto iv =  BSModelQuantLib->calImpliedVol(157560.0, 6840.0);

    double delta{0.0};
    double gamma{0.0};
    double theta{0.0};
    double vega{0.0};

    BSModelQuantLib->calGreeks(157560.0, iv,
                                      delta,
                                      gamma,
                                      theta,
                                      vega);
    int a =1;

}