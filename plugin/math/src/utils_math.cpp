#include <cmath>
#include "math/include/utils_math.h"
#include "utils/include/utils_io.h"
#include "utils/include/debug.h"
#include "math/include/constants.h"

namespace feasst {

int round(double x) { return floor(x + 0.5); }

std::vector<double> cumulative_probability(
    const std::vector<double>& weights) {
  std::vector<double> prob(weights.size());
  double total_weight = 0;
  for (const double weight : weights) {
    total_weight += weight;
  }
  double accumulator = 0.;
  for (int index = 0; index < static_cast<int>(weights.size()); ++index) {
    accumulator += weights[index]/total_weight;
    prob[index] = accumulator;
  }
  TRACE("prob " << feasst_str(prob));
  ASSERT(std::abs(prob.back() - 1.) < NEAR_ZERO,
    "cumulative probability must end in 1. " <<
    MAX_PRECISION << prob.back());
  return prob;
}

double radians_to_degrees(const double radians) {
  return radians/PI*180.;
}

double degrees_to_radians(const double degrees) {
  return degrees/180.*PI;
}

/// Swap the values.
}  // namespace feasst
