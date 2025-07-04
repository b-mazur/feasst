
#ifndef FEASST_MONTE_CARLO_TRIAL_STAGE_H_
#define FEASST_MONTE_CARLO_TRIAL_STAGE_H_

#include <string>
#include <map>
#include <memory>

namespace feasst {

class Acceptance;
class Criteria;
class Perturb;
class Random;
class Rosenbluth;
class System;
class TrialSelect;

typedef std::map<std::string, std::string> argtype;

/**
  A stage contains both a selection and perturbation.
  Random realizations of the perturbation are performed in a number of steps.
  The use of reference potentials in stages is a generalization of the dual-cut
  configurational bias (DC-CB) methodology as described in
  http://doi.org/10.1080/002689798167881.
 */
class TrialStage {
 public:
  //@{
  /** @name Arguments
   */

  /**
    args:
    - num_steps: number of Rosenbluth steps (default: 1).
    - ref: name of RefPotential. If empty, use Potential (default: empty).
    - new_only: do not compute the Rosenbluth of the old configuration
      (default: false).
   */
  explicit TrialStage(argtype * args);

  //@}
  /** @name Public Functions
   */
  //@{

  /// Return the index of the reference potential.
  int reference() const { return reference_; }

  /// Return the name of the reference potential.
  const std::string& ref() const { return ref_; }

  /// Return true if the trial computes new configuration only.
  bool is_new_only() const { return is_new_only_; }

  /// Return the Rosenbluth.
  const Rosenbluth& rosenbluth() const;

  /// Return the number of steps in Rosenbluth.
  const int num_steps() const;

  /// Set the selection.
  void set(std::shared_ptr<TrialSelect> select);

  /// Return the above.
  const TrialSelect& select() const;

  // HWH deprecate
  const TrialSelect& trial_select() const { return select(); }

  /// Initialization before any stage attempt.
  void precompute(System * system);

  /// Initializations before each stage attempt.
  void before_select();

  /// Perform the selection and update the acceptance.
  /// Return false if selection fails.
  /// Otherwise, set sites involved in stage as unphysical and return true.
  bool select(System * system, Acceptance * acceptance, Random * random, TrialSelect * previous_select);

  /// Set the perturbation.
  void set(std::shared_ptr<Perturb> perturb);

  /// Return the above.
  const Perturb& perturb() const;

  /// Set mobile selection physical.
  void set_mobile_physical(const bool physical, System * system);

  /// Attempt all steps in a stage.
  /// Consider reference potentials and compute Rosenbluth factors.
  /// Set sites involves in stage as physical.
  void attempt(
    System * system,
    Acceptance * acceptance,
    Criteria * criteria,
    /// Set to 1 for "old" system and "0" for new.
    const int old,
    Random * random);

  /// Call between multiple attempts (e.g., old vs new)
  /// Set mobile sites unphysical.
  void mid_stage(System * system);
  void begin_stage();

  /// Return true if constraints are satisfied.
  bool are_constraints_satisfied(const int old, const System& system) const;

  /// Revert the attempt.
  void revert(System * system);

  /// Finalize the attempt.
  void finalize(System * system);

  /// Tune parameters.
  void tune(const double acceptance);

  /// Print status header.
  std::string status_header() const;

  /// Print status.
  std::string status() const;

  // HWH avoid using this
  TrialSelect * get_trial_select();
  void set_tunable(const double tunable);

  /// Serialize.
  void serialize(std::ostream& ostr) const;

  /// Deserialize.
  explicit TrialStage(std::istream& istr);

  ~TrialStage() {}

  //@}
 private:
  int reference_ = -1;
  std::string ref_;
  std::shared_ptr<Perturb> perturb_;
  std::shared_ptr<TrialSelect> select_;
  std::shared_ptr<Rosenbluth> rosenbluth_;
  bool is_new_only_;
  void set_rosenbluth_energy_(const int step, System * system);
};

/// Return the optional arguments relevant to TrialStage.
/// This helps multiple stages with the same arguments.
argtype get_stage_args(argtype * args);

}  // namespace feasst

#endif  // FEASST_MONTE_CARLO_TRIAL_STAGE_H_
