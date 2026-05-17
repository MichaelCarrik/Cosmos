//
// This source code resides at www.jaeckel.org/LetsBeRational.7z .
//
// ======================================================================================
// Copyright © 2013-2023 Peter Jäckel.
//
// Permission to use, copy, modify, and distribute this software is freely granted,
// provided that this notice is preserved.
//
// WARRANTY DISCLAIMER
// The Software is provided "as is" without warranty of any kind, either express or implied,
// including without limitation any implied warranties of condition, uninterrupted use,
// merchantability, fitness for a particular purpose, or non-infringement.
// ======================================================================================
//
#ifndef   LETS_BE_RATIONAL_H
#define   LETS_BE_RATIONAL_H

#include "dllmain.h"
namespace LetsBeRational {
    // As defined in equation (1.1) in "Let's Be Rational".
    extern "C" DLL_EXPORT double Black(double F, double K, double sigma, double T, double q /* q=±1 */);
    // As defined in equation (2.3) in "Let's Be Rational": 𝛽(x,s) := B(F,K,σ,T,θ=±1)/√(F·K) with x=ln(F/K) and s=σ√T
    extern "C" DLL_EXPORT double NormalisedBlack(double x, double s, double q /* q=±1 */);

    extern "C" DLL_EXPORT double ImpliedBlackVolatility(double price, double F, double K, double T, double q /* q=±1 */);

    // The input 'beta' is taken as a "normalised Black" price as defined in equation (2.3) in "Let's Be Rational".
    extern "C" DLL_EXPORT double NormalisedImpliedBlackVolatility(double beta, double x, double q /* q=±1 */);

    //    b̄(x,s,θ)          :=   bₘₐₓ(x,θ)   -  b(x,s,θ)
    //                       =   eˣ𝄍²·Φ(-x/s-s/2) + e⁻ˣ𝄍²·Φ(x/s-s/2)                                            |     for both θ = ±1
    // Same for calls and puts, i.e., no dependency on θ = ±1.
    extern "C" DLL_EXPORT double ComplementaryNormalisedBlack(double x, double s);

#define EXPOSE_VEGA_AND_MORE

#if defined(EXPOSE_VEGA_AND_MORE) || !defined(SWIG)
    // ∂Black(F,K,σ,T)/∂σ  [no dependency on the call/put flag θ=±1]
    extern "C" DLL_EXPORT double Vega(double F, double K, double sigma, double T);
    // ∂𝛽(x,s)/∂s          [no dependency on the call/put flag θ=±1]   with x=ln(F/K) and s=σ√T
    extern "C" DLL_EXPORT double NormalisedVega(double x, double s);
    // ∂²Black(F,K,σ,T)/∂σ²  [no dependency on the call/put flag θ=±1]
    extern "C" DLL_EXPORT double Volga(double F, double K, double sigma, double T);
    // ∂²𝛽(x,s)/∂s²          [no dependency on the call/put flag θ=±1]   with x=ln(F/K) and s=σ√T
    extern "C" DLL_EXPORT double NormalisedVolga(double x, double s);
    // The attainable *relative* accuracy of 𝛽 = b(s) when s has *relative* accuracy ε is (to lowest order) (|s·b'(s)/b(x)|+1)·ε --- see the source code for a detailed derivation.
    // The attainable *relative* accuracy of x = b⁻¹(𝛽) when 𝛽 has *relative* accuracy ε is (to lowest order) (|b(s)/(s·b'(s))|+1)·ε .
    // This function returns (s·∂b(x,s)/∂s)/b(x,s,θ=±1). In order to get the accuracy limit of implied volatility calculations, take (1+1/BlackAccuracyFactor(x,s,θ))·DBL_EPSILON.
    extern "C" DLL_EXPORT double BlackAccuracyFactor(double x /* = ln(F/K) */, double s /* = σ√T */, double q /* q=±1 */);
    // The attainable *relative* accuracy of x = b⁻¹(𝛽) when 𝛽 has *relative* accuracy ε is (to lowest order) (|b(s)/(s·b'(s))|+1)·ε .
    extern "C" DLL_EXPORT double ImpliedVolatilityAttainableAccuracy(double x, double s, double q /* q=±1 */);

    // DBL_EPSILON
    extern "C" DLL_EXPORT double DblEpsilon();
    // DBL_MIN
    extern "C" DLL_EXPORT double DblMin();
    // DBL_MAX
    extern "C" DLL_EXPORT double DblMax();
#endif

    //#define ENABLE_CHANGING_THE_MAXIMUM_ITERATION_COUNT
#ifdef ENABLE_CHANGING_THE_MAXIMUM_ITERATION_COUNT
    extern "C" DLL_EXPORT int set_implied_volatility_maximum_iterations(int n);
#endif

    //#define ENABLE_CHANGING_THE_HOUSEHOLDER_METHOD_ORDER
#ifdef ENABLE_CHANGING_THE_HOUSEHOLDER_METHOD_ORDER
    extern "C" DLL_EXPORT int set_implied_volatility_householder_method_order(int m);
#endif

    //#define ENABLE_SWITCHING_THE_OUTPUT_TO_ITERATION_COUNT
#ifdef ENABLE_SWITCHING_THE_OUTPUT_TO_ITERATION_COUNT
    extern "C" DLL_EXPORT int set_implied_volatility_output_type(int type);
#endif
}
#endif // LETS_BE_RATIONAL_H
