
#ifndef FEASST_CORE_POTENTIAL_FACTORY_H_
#define FEASST_CORE_POTENTIAL_FACTORY_H_

#include <vector>
#include <memory>
#include <string>
#include "core/include/potential.h"

namespace feasst {

/**
 */
class PotentialFactory {
 public:
  void add_potential(const Potential potential) {
    potentials_.push_back(potential); }

  double energy(Configuration * config) {
    double en = 0;
    int index = 0;
    while ((index < static_cast<int>(potentials_.size())) &&
           (en < NEAR_INFINITY)) {
      en += potentials_[index].energy(config);
      ++index;
    }
    //INFO("en " << en);
    return en;
  }

  double energy(const Select& select, Configuration * config) {
    double en = 0;
    int index = 0;
    while ((index < static_cast<int>(potentials_.size())) &&
           (en < NEAR_INFINITY)) {
      en += potentials_[index].energy(select, config);
      ++index;
    }
    //INFO("en " << en);
    return en;
  }

  void revert() {
    for (Potential& potential : potentials_) {
      potential.revert();
    }
  }

  const std::vector<Potential>& potentials() const { return potentials_; }

  double stored_energy() const {
    double en = 0.;
    for (const Potential& potential : potentials_) {
      en += potential.stored_energy();
    }
    return en;
  }

  void precompute(Configuration * config) {
    for (Potential& potential : potentials_) {
      potential.precompute(config);
    }
  }

  int num() const { return static_cast<int>(potentials_.size()); }
 private:
  std::vector<Potential> potentials_;
};

}  // namespace feasst

#endif  // FEASST_CORE_POTENTIAL_FACTORY_H_
