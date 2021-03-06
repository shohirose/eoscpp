#pragma once

#include <array>  // std::array
#include <cmath>  // std::exp, std::log

#include "eos/cubic_eos/cubic_eos_base.hpp"  // eos::cubic_eos_base
#include "eos/math/cubic_equation.hpp"       // eos::cubic_equation

namespace eos {

class van_der_waals_eos;

template <>
struct cubic_eos_traits<van_der_waals_eos> {
  static constexpr double omega_a = 0.421875;
  static constexpr double omega_b = 0.125;
};

/// @brief Van der Waals Equations of State
class van_der_waals_eos : public cubic_eos_base<van_der_waals_eos, false> {
 public:
  using base_type = cubic_eos_base<van_der_waals_eos, false>;

  // Static Functions

  /// @brief Computes pressure at given temperature and volume.
  /// @param[in] t Temperature
  /// @param[in] v Volume
  /// @param[in] a Attraction parameter
  /// @param[in] b Repulsion parameter
  /// @returns Pressure
  static double pressure(double t, double v, double a, double b) noexcept {
    return gas_constant<double>() * t / (v - b) - a / (v * v);
  }

  /// @brief Computes coefficients of the cubic equation of Z-factor
  /// @param[in] a Reduced attraction parameter
  /// @param[in] b Reduced repulsion parameter
  /// @returns Coefficients of the cubic equation of z-factor
  static cubic_equation zfactor_cubic_eq(double a, double b) noexcept {
    return {-b - 1, a, -a * b};
  }

  /// @brief Computes the natural logarithm of a fugacity coefficient
  /// @param[in] z Z-factor
  /// @param[in] a Reduced attraction parameter
  /// @param[in] b Reduced repulsion parameter
  /// @returns The natural logarithm of a fugacity coefficient
  static double ln_fugacity_coeff(double z, double a, double b) noexcept {
    return -std::log(z - b) - a / z + z - 1;
  }

  /// @brief Computes a fugacity coefficient
  /// @param[in] z Z-factor
  /// @param[in] a Reduced attraction parameter
  /// @param[in] b Reduced repulsion parameter
  /// @returns Fugacity coefficient
  static double fugacity_coeff(double z, double a, double b) noexcept {
    return std::exp(ln_fugacity_coeff(z, a, b));
  }

  /// @brief Computes residual enthalpy
  /// @param[in] z Z-factor
  /// @param[in] t Temperature
  /// @param[in] a Reduced attraction parameter
  /// @param[in] b Reduced repulsion parameter
  static double residual_enthalpy(double z, double t, double a,
                                  double b) noexcept {
    return gas_constant<double>() * t * (z - 1 - a / z);
  }

  /// @brief Computes residual entropy
  /// @param[in] z Z-factor
  /// @param[in] a Reduced attraction parameter
  /// @param[in] b Reduced repulsion parameter
  static double residual_entropy(double z, double a, double b) noexcept {
    return gas_constant<double>() * (std::log(z - b));
  }

  /// @brief Computes residual Helmholtz energy
  /// @param[in] z Z-factor
  /// @param[in] t Temperature
  /// @param[in] a Reduced attraction parameter
  /// @param[in] b Reduced repulsion parameter
  static double residual_helmholtz_energy(double z, double t, double a,
                                          double b) noexcept {
    constexpr auto R = gas_constant<double>();
    return R * t * (std::log(z - b) + a / z);
  }

  van_der_waals_eos() = default;

  van_der_waals_eos(double pc, double tc) noexcept : base_type{pc, tc} {}

  van_der_waals_eos(const van_der_waals_eos &) = default;
  van_der_waals_eos(van_der_waals_eos &&) = default;

  van_der_waals_eos &operator=(const van_der_waals_eos &) = default;
  van_der_waals_eos &operator=(van_der_waals_eos &&) = default;

  void set_params(double pc, double tc) noexcept {
    this->base_type::set_params(pc, tc);
  }
};

/// @brief Makes van der Waals EoS
/// @param[in] pc Critical pressure
/// @param[in] tc Critical temperature
inline van_der_waals_eos make_van_der_waals_eos(double pc, double tc) {
  return {pc, tc};
}

}  // namespace eos