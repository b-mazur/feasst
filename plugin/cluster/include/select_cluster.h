
#ifndef FEASST_CLUSTER_SELECT_CLUSTER_H_
#define FEASST_CLUSTER_SELECT_CLUSTER_H_

#include <vector>
#include <memory>
#include "monte_carlo/include/trial_select_particle.h"

namespace feasst {

typedef std::map<std::string, std::string> argtype;

/**
  Select a random particle.
  Find all particles which interact with any of the sites on the selection
  via full potential (default) or a given reference potential.
 */
class SelectCluster : public TrialSelect {
 public:
  /**
    args:
    - neighbor_index: NeighborCriteria index contained in Configuration (default: 0).
   */
  explicit SelectCluster(argtype args = argtype());
  explicit SelectCluster(argtype * args);

  /// Return a cluster as selection given one particle in the system.
  void select_cluster(const int first_particle, const System& system,
                      Select * select);

  // Same as above, except put selection in mobile.
  void select_cluster(const int first_particle, const System& system);

  // Return true if the cluster has changed (e.g., increased in size)
  // for detailed balance constraint of rigid cluster moves.
  bool are_constraints_satisfied(const int old,
    const System& system) const override;

  /// Return a list of selections representing individual cluster.
  std::vector<Select> select_clusters(const System& system);

  void precompute(System * system) override;
  bool select(const Select& perturbed,
              System* system,
              Random * random) override;

  std::shared_ptr<TrialSelect> create(std::istream& istr) const override;
  void serialize(std::ostream& ostr) const override;
  explicit SelectCluster(std::istream& istr);
  virtual ~SelectCluster() {}

 protected:
  void serialize_select_cluster_(std::ostream& ostr) const;

 private:
  int neighbor_;
  std::shared_ptr<TrialSelectParticle> select_particle_;
};

inline std::shared_ptr<SelectCluster> MakeSelectCluster(
    const argtype &args = argtype()) {
  return std::make_shared<SelectCluster>(args);
}

}  // namespace feasst

#endif  // FEASST_CLUSTER_SELECT_CLUSTER_H_
