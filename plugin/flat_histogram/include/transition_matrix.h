
#ifndef FEASST_FLAT_HISTOGRAM_TRANSITION_MATRIX_H_
#define FEASST_FLAT_HISTOGRAM_TRANSITION_MATRIX_H_

#include <vector>
#include <memory>
#include "utils/include/arguments.h"
#include "flat_histogram/include/bias.h"
#include "flat_histogram/include/collection_matrix.h"

namespace feasst {

/**
  Transition matrix flat histogram bias.

  Direct calculation of liquid–vapor phase equilibria from transition matrix
  Monte Carlo simulation
  https://doi.org/10.1063/1.1572463

  Elucidating the effects of adsorbent flexibility on fluid adsorption using
  simple models and flat-histogram sampling methods.
  https://doi.org/10.1063/1.4884124
 */
class TransitionMatrix : public Bias {
 public:
  TransitionMatrix(
    /**
      min_visits : A sweep is performed when all macrostates are visited by
        another macrostate this number of times (default: 100).

      min_sweeps : Number of sweeps required for completion.
     */
    const argtype &args = argtype());
  void update_or_revert(
    const int macrostate_old,
    const int macrostate_new,
    const double ln_metropolis_prob,
    const bool is_accepted,
    const bool revert) override;
  const LnProbability& ln_prob() const override {
    return ln_prob_; }
  void resize(const Histogram& histogram) override;
  std::string write() const override;
  std::string write_per_bin(const int bin) const override;
  std::string write_per_bin_header() const override;
  void set_ln_prob(const LnProbability& ln_prob) override;
  void infrequent_update() override;

  std::shared_ptr<Bias> create(std::istream& istr) const override;
  void serialize(std::ostream& ostr) const override;
  explicit TransitionMatrix(std::istream& istr);
  virtual ~TransitionMatrix() {}

 private:
  LnProbability ln_prob_;
  TripleBandedCollectionMatrix collection_;
  std::vector<int> visits_;
  int min_visits_ = 0;
  int num_sweeps_ = 0;
  int min_sweeps_ = 0;

  std::vector<TransitionMatrix> blocks_;
  bool is_block_ = false;
  int iter_block_= -1;

  void update_blocks_(
      const int macrostate_old,
      const int macrostate_new,
      const double ln_metropolis_prob,
      const bool is_accepted,
      const bool revert);
};

inline std::shared_ptr<TransitionMatrix> MakeTransitionMatrix(
    const argtype &args = argtype()) {
  return std::make_shared<TransitionMatrix>(args);
}

}  // namespace feasst

#endif  // FEASST_FLAT_HISTOGRAM_TRANSITION_MATRIX_H_
