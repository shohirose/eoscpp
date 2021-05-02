#pragma once

#include <gsl/gsl>  // gsl::span
#include <vector>   // std::vector

namespace eos {

/// @brief Computes real roots of a cubic equation
/// @param[in] a First coefficient
/// @param[in] b Second coefficient
/// @param[in] c Third coefficient
/// @returns An array of real roots in the ascending order
///
/// The cubic equation takes the form of:
/// \f[
///   x^3 + ax^2 + bx + c = 0.
/// \f]
std::vector<double> real_roots(double a, double b, double c) noexcept;

/// @brief Computes real roots of a polynomial equation
/// @param[in] a The coefficients of the polynomial
/// @returns Real roots in the ascending order
///
/// The polynomial takes the form of:
/// \f[
///  a[0] + a[1] x + a[2] x^2 + ... + a[N-1] x^{N-1} = 0
/// \f]
std::vector<double> real_roots(gsl::span<const double> a);

}  // namespace eos