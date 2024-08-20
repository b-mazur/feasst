#include <cmath>
#include "utils/include/serialize.h"
#include "configuration/include/configuration.h"
#include "configuration/include/select.h"
#include "configuration/include/particle_factory.h"
#include "system/include/thermo_params.h"
#include "monte_carlo/include/acceptance.h"
#include "monte_carlo/include/trial_select.h"
#include "cluster/include/compute_remove_avb.h"

namespace feasst {

ComputeRemoveAVB::ComputeRemoveAVB() {
  class_name_ = "ComputeRemoveAVB";
}

class MapComputeRemoveAVB {
 public:
  MapComputeRemoveAVB() {
    auto obj = MakeComputeRemoveAVB();
    obj->deserialize_map()["ComputeRemoveAVB"] = obj;
  }
};

static MapComputeRemoveAVB mapper_ = MapComputeRemoveAVB();

void ComputeRemoveAVB::perturb_and_acceptance(
    Criteria * criteria,
    System * system,
    Acceptance * acceptance,
    std::vector<TrialStage*> * stages,
    Random * random) {
  DEBUG("ComputeRemoveAVB");
  compute_rosenbluth(1, criteria, system, acceptance, stages, random);
  acceptance->set_energy_new(criteria->current_energy() - acceptance->energy_old());
  acceptance->set_energy_profile_new(criteria->current_energy_profile());
  acceptance->subtract_from_energy_profile_new(acceptance->energy_profile_old());
  acceptance->add_to_macrostate_shift(-1);
  DEBUG("old en " << criteria->current_energy());
  DEBUG("new en " << MAX_PRECISION << acceptance->energy_new());
  { // Metropolis
    const Configuration& config = system->configuration();
    const TrialSelect& select = (*stages)[0]->trial_select();
    const int particle_index = select.mobile().particle_index(0);
    const int particle_type = config.select_particle(particle_index).type();
    acceptance->set_macrostate_shift_type(particle_type);
    const ThermoParams& params = system->thermo_params();
    DEBUG("selprob " << select.probability() << " betamu " << params.beta_mu(particle_type));
    acceptance->add_to_ln_metropolis_prob(
      + std::log(select.probability())
      - params.beta_mu(particle_type));
    DEBUG("lnmet " << acceptance->ln_metropolis_prob());
  }
}

std::shared_ptr<TrialCompute> ComputeRemoveAVB::create(std::istream& istr) const {
  return std::make_shared<ComputeRemoveAVB>(istr);
}

ComputeRemoveAVB::ComputeRemoveAVB(std::istream& istr)
  : TrialCompute(istr) {
  // ASSERT(class_name_ == "ComputeRemoveAVB", "name: " << class_name_);
  const int version = feasst_deserialize_version(istr);
  ASSERT(6256 == version, "mismatch version: " << version);
}

void ComputeRemoveAVB::serialize_compute_remove_avb_(std::ostream& ostr) const {
  serialize_trial_compute_(ostr);
  feasst_serialize_version(6256, ostr);
}

void ComputeRemoveAVB::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_compute_remove_avb_(ostr);
}

}  // namespace feasst
