#pragma once

#include <vector>  // std::vector

#include "eos/common/thermodynamic_constants.hpp"  // eos::gas_constant
#include "eos/cubic_eos/isobaric_isothermal_state.hpp"
#include "eos/cubic_eos/isothermal_line.hpp"

namespace eos {

template <typename Eos>
struct cubic_eos_traits {};

template <typename Derived>
class cubic_eos_crtp_base {
 public:
  static constexpr auto omega_a = cubic_eos_traits<Derived>::omega_a;
  static constexpr auto omega_b = cubic_eos_traits<Derived>::omega_b;

  cubic_eos_crtp_base() = default;
  cubic_eos_crtp_base(const cubic_eos_crtp_base &) = default;
  cubic_eos_crtp_base(cubic_eos_crtp_base &&) = default;

  /// @brief Constructs cubic EoS
  /// @param[in] pc Critical pressure
  /// @param[in] tc Critical temperature
  cubic_eos_crtp_base(double pc, double tc) noexcept
      : pc_{pc},
        tc_{tc},
        ac_{this->critical_attraction_param(pc, tc)},
        bc_{this->critical_repulsion_param(pc, tc)} {}

  cubic_eos_crtp_base &operator=(const cubic_eos_crtp_base &) = default;
  cubic_eos_crtp_base &operator=(cubic_eos_crtp_base &&) = default;

  // Static functions

  /// @param[in] pc Critical pressure
  /// @param[in] tc Critical temperature
  static double critical_attraction_param(double pc, double tc) noexcept {
    constexpr auto R = gas_constant<double>();
    return (omega_a * R * R) * tc * tc / pc;
  }

  /// @param[in] pc Critical pressure
  /// @param[in] tc Critical temperature
  static double critical_repulsion_param(double pc, double tc) noexcept {
    constexpr auto R = gas_constant<double>();
    return (omega_b * R) * tc / pc;
  }

  /// @brief Returns reduced attraction parameter at a given pressure and
  /// temperature without temperature correction.
  /// @param[in] pr Reduced pressure
  /// @param[in] tr Reduced temperature
  static double reduced_attraction_param(double pr, double tr) noexcept {
    return omega_a * pr / (tr * tr);
  }

  /// @brief Returns reduced repulsion parameter at a given pressure and
  /// temperature.
  /// @param[in] pr Reduced pressure
  /// @param[in] tr Reduced temperature
  static double reduced_repulsion_param(double pr, double tr) noexcept {
    return omega_b * pr / tr;
  }

  // Member functions

  /// @param[in] pc Critical pressure
  /// @param[in] tc Critical temperature
  void set_params(double pc, double tc) noexcept {
    pc_ = pc;
    tc_ = tc;
    ac_ = critical_attraction_param(pc, tc);
    bc_ = critical_repulsion_param(pc, tc);
  }

  /// @brief Computes reduced pressure
  /// @param[in] p Pressure
  double reduced_pressure(double p) const noexcept { return p / pc_; }

  /// @brief Computes reduced temperature
  /// @param[in] t Temperature
  double reduced_temperature(double t) const noexcept { return t / tc_; }

 protected:
  /// @brief Get reference to derived class object
  Derived &derived() noexcept { return static_cast<Derived &>(*this); }

  /// @brief Get const reference to derived class object
  const Derived &derived() const noexcept {
    return static_cast<const Derived &>(*this);
  }

  double pc_;  /// Critical pressure
  double tc_;  /// Critical temperature
  double ac_;  /// Critical attraction parameter
  double bc_;  /// Critical repulsion parameter
};

/// @brief Two-parameter cubic equation of state (EoS)
/// @tparam Derived Concrete EoS class
///
/// Derived EoS classes must have the following static functions:
///    - pressure(t, v, a, b)
///    - zfactor_cubic_eq(ar, br)
///    - fugacity_coeff(z, ar, br)
///    - residual_enthalpy(z, t, ar, br, beta)
///    - residual_entropy(z, ar, br, beta)
///    - alpha(tr)
///    - beta()
/// where t is temperature, v is volume, a is attraction parameter, b is
/// repulsion parameter, ar is reduced attraction parameter, br is reduced
/// repulsion parameter, z is z-factor, and beta is a temperature correction
/// factor.
///
/// cubic_eos_traits class specialized for each concrete EoS class must be
/// defined in the detail namespace. cubic_eos_traits class must define the
/// following types and constants:
///    - double: double type
///    - omega_a: Constant for attraction parameter
///    - omega_b: Constant for repulsion parameter
///
template <typename Derived, bool UseTemperatureCorrectionFactor>
class cubic_eos_base : public cubic_eos_crtp_base<Derived> {
 public:
  using base_type = cubic_eos_crtp_base<Derived>;
  static constexpr auto omega_a = base_type::omega_a;
  static constexpr auto omega_b = base_type::omega_b;

  // Constructors

  cubic_eos_base() = default;

  /// @brief Constructs cubic EoS
  /// @param[in] pc Critical pressure
  /// @param[in] tc Critical temperature
  cubic_eos_base(double pc, double tc) noexcept : base_type{pc, tc} {}

  cubic_eos_base(const cubic_eos_base &) = default;
  cubic_eos_base(cubic_eos_base &&) = default;

  cubic_eos_base &operator=(const cubic_eos_base &) = default;
  cubic_eos_base &operator=(cubic_eos_base &&) = default;

  // Member functions

  /// @brief Creates isothermal state
  /// @param[in] t Temperature
  isothermal_line<Derived> create_isothermal_line(double t) const noexcept {
    const auto tr = this->reduced_temperature(t);
    const auto alpha = this->derived().alpha(tr);
    return {t, alpha * ac_, bc_};
  }

  /// @brief Creates isobaric-isothermal state
  /// @param[in] p Pressure
  /// @param[in] t Temperature
  isobaric_isothermal_state<Derived, UseTemperatureCorrectionFactor>
  create_isobaric_isothermal_state(double p, double t) const noexcept {
    const auto pr = this->reduced_pressure(p);
    const auto tr = this->reduced_temperature(t);
    const auto ar =
        this->derived().alpha(tr) * this->reduced_attraction_param(pr, tr);
    const auto br = this->reduced_repulsion_param(pr, tr);
    const auto beta = this->derived().beta(tr);
    return {t, ar, br, beta};
  }

  /// @brief Computes pressure at given temperature and volume
  /// @param[in] t Temperature
  /// @param[in] v Volume
  double pressure(double t, double v) const noexcept {
    const auto tr = this->reduced_temperature(t);
    const auto a = this->derived().alpha(tr) * this->attraction_param();
    const auto b = this->repulsion_param();
    return Derived::pressure(t, v, a, b);
  }

  /// @brief Computes Z-factor at given pressure and temperature
  /// @param[in] p Pressure
  /// @param[in] t Temperature
  /// @return A list of Z-factors
  std::vector<double> zfactor(double p, double t) const noexcept {
    return this->create_isobaric_isothermal_state(p, t).zfactor();
  }
};

template <typename Derived>
class cubic_eos_base<Derived, false> : public cubic_eos_crtp_base<Derived> {
 public:
  using base_type = cubic_eos_crtp_base<Derived>;
  static constexpr auto omega_a = base_type::omega_a;
  static constexpr auto omega_b = base_type::omega_b;

  // Constructors

  cubic_eos_base() = default;

  /// @brief Constructs cubic EoS
  /// @param[in] pc Critical pressure
  /// @param[in] tc Critical temperature
  cubic_eos_base(double pc, double tc) noexcept : base_type{pc, tc} {}

  cubic_eos_base(const cubic_eos_base &) = default;
  cubic_eos_base(cubic_eos_base &&) = default;

  cubic_eos_base &operator=(const cubic_eos_base &) = default;
  cubic_eos_base &operator=(cubic_eos_base &&) = default;

  // Member functions

  /// @brief Creates isothermal state
  /// @param[in] t Temperature
  isothermal_line<Derived> create_isothermal_line(double t) const noexcept {
    return {t, ac_, bc_};
  }

  /// @brief Creates isobaric-isothermal state
  /// @param[in] p Pressure
  /// @param[in] t Temperature
  isobaric_isothermal_state<Derived, false> create_isobaric_isothermal_state(
      double p, double t) const noexcept {
    const auto pr = this->reduced_pressure(p);
    const auto tr = this->reduced_temperature(t);
    const auto ar = this->reduced_attraction_param(pr, tr);
    const auto br = this->reduced_repulsion_param(pr, tr);
    return {t, ar, br};
  }

  /// @brief Computes pressure at given temperature and volume
  /// @param[in] t Temperature
  /// @param[in] v Volume
  double pressure(double t, double v) const noexcept {
    return Derived::pressure(t, v, ac_, bc_);
  }

  /// @brief Computes Z-factor at given pressure and temperature
  /// @param[in] p Pressure
  /// @param[in] t Temperature
  /// @return A list of Z-factors
  std::vector<double> zfactor(double p, double t) const noexcept {
    return this->create_isobaric_isothermal_state(p, t).zfactor();
  }
};

}  // namespace eos