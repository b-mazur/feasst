
#ifndef FEASST_MONTE_CARLO_PERTURB_MOVE_H_
#define FEASST_MONTE_CARLO_PERTURB_MOVE_H_

#include <memory>
#include "monte_carlo/include/perturb.h"

namespace feasst {

/**
  Only perturb the positions of the particles and/or sites.
 */
class PerturbMove : public Perturb {
 public:
  explicit PerturbMove(argtype args = argtype()) : PerturbMove(&args) {}
  explicit PerturbMove(argtype * args) : Perturb(args) {}

  /// Move the selection of the system.
  virtual void move(
    const bool is_position_held,
    System * system,
    TrialSelect * select,
    Random * random,
    Acceptance * acceptance) = 0;

  // The perturbation move is simplified such that the move of the selection of
  // the system is all that remains to be implemented.
  void perturb(
      System * system,
      TrialSelect * select,
      Random * random,
      const bool is_position_held = false,
      Acceptance * acceptance = NULL) override;

  /// For perturbations that only move particles and/or sites, the revert step
  /// is the same for all. Simply put the original positions back.
  void revert(System * system) override;
  void finalize(System * system) override;

  explicit PerturbMove(std::istream& istr) : Perturb(istr) {}
  virtual ~PerturbMove() {}
};

}  // namespace feasst

#endif  // FEASST_MONTE_CARLO_PERTURB_MOVE_H_
