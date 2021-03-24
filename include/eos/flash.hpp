#pragma once

#include <algorithm>  // std::max_element
#include <cassert>    // assert
#include <cmath>      // std::pow
#include <iostream>   // std::cerr
#include <utility>    // std::pair, std::forward

namespace eos {

/// @brief Estimates vapor pressure of a pure component by using Wilson
/// equation.
template <typename T>
T estimate_vapor_pressure(const T& t, const T& pc, const T& tc,
                          const T& omega) noexcept {
  assert(t <= tc);
  using std::pow;
  return pc * pow(10, 7.0 / 3.0 * (1 + omega) * (1 - tc / t));
}

/// @brief Flash calculation class
/// @tparam Eos Equation of State
template <typename Eos>
class flash {
 public:
  /// @brief Constructs flash object
  /// @param[in] eos EoS
  ///
  /// The default values of tolerance and maxixum iteration are 1e-6 and 100,
  /// respectively.
  flash(const Eos& eos) : eos_{eos}, tol_{1e-6}, max_iter_{100} {}

  /// @brief Constructs flash object
  /// @param[in] eos EoS
  /// @param[in] tol Tolerance for Flash calculation convergence
  /// @param[in] max_iter Maximum iteration
  flash(const Eos& eos, double tol, int max_iter)
      : eos_{eos}, tol_{tol}, max_iter_{max_iter} {}

  /// @brief Error code for Flash calculation
  enum class error_code {
    success,                   /// Calculation succeeded
    max_iter_reached,          /// Maximum iteration reached
    multiple_roots_not_found,  /// Multiple roots not found in z-factor
  };

  /// @brief Iteration report
  struct iter_report {
    double rsd;        /// Relative residual
    int iter;          /// Iteration count
    error_code error;  /// Error code
  };

  /// @brief Computes vapor pressure
  /// @tparam T Value type
  /// @param[in] p_init Initial pressure
  /// @param[in] t Temperature
  /// @return A pair of vapor pressure and iteration report
  template <typename T>
  std::pair<T, iter_report> vapor_pressure(const T& p_init, const T& t) const
      noexcept {
    auto p = p_init;
    double eps = 1;
    int iter = 0;

    using std::fabs;

    while (eps > tol_ && iter < max_iter_) {
      const auto state = eos_.state(p, t);
      const auto z = state.zfactor();

      if (z.size() < 2) {
        std::cerr << "Multiple roots not found in z-factor." << std::endl;
        return {0, {eps, iter, error_code::multiple_roots_not_found}};
      }

      const auto z_vap = *std::max_element(z.begin(), z.end());
      const auto z_liq = *std::min_element(z.begin(), z.end());
      const auto phi_vap = state.fugacity_coeff(z_vap);
      const auto phi_liq = state.fugacity_coeff(z_liq);

      eps = fabs(1 - phi_liq / phi_vap);

      // Update vapor pressure by successive substitution
      p *= phi_liq / phi_vap;

      ++iter;
    }

    if (iter >= max_iter_) {
      std::cerr << "Max iteration reached." << std::endl;
      return {0, {eps, iter, error_code::max_iter_reached}};
    }

    return {p, {eps, iter, error_code::success}};
  }

  double tolerance() const noexcept { return tol_; }
  double& tolerance() noexcept { return tol_; }

  int max_iter() const noexcept { return max_iter_; }
  int& max_iter() noexcept { return max_iter_; }

 private:
  /// EoS
  Eos eos_;
  /// Tolerance
  double tol_;
  /// Maximum iteration
  int max_iter_;
};

template <typename Eos>
inline flash<Eos> make_flash(Eos&& eos) {
  return flash<Eos>{std::forward<Eos>(eos)};
}

}  // namespace eos