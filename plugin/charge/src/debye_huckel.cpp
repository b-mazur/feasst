#include <cmath>
#include "utils/include/arguments.h"
#include "utils/include/serialize.h"
#include "math/include/constants.h"
#include "configuration/include/physical_constants.h"
#include "configuration/include/configuration.h"
#include "configuration/include/model_params.h"
#include "charge/include/debye_huckel.h"

namespace feasst {

FEASST_MAPPER(DebyeHuckel, argtype({{"kappa", "1"}, {"dielectric", "1"}}));

DebyeHuckel::DebyeHuckel(argtype * args) {
  class_name_ = "DebyeHuckel";
  kappa_ = dble("kappa", args);
  dielectric_ = dble("dielectric", args);
  smoothing_distance_ = dble("smoothing_distance", args, -1.);
}
DebyeHuckel::DebyeHuckel(argtype args) : DebyeHuckel(&args) {
  feasst_check_all_used(args);
}

void DebyeHuckel::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_model_(ostr);
  feasst_serialize_version(3682, ostr);
  feasst_serialize(conversion_factor_, ostr);
  feasst_serialize(kappa_, ostr);
  feasst_serialize(dielectric_, ostr);
  feasst_serialize(smoothing_distance_, ostr);
}

DebyeHuckel::DebyeHuckel(std::istream& istr) : ModelTwoBody(istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(version == 3682, "unrecognized verison: " << version);
  feasst_deserialize(&conversion_factor_, istr);
  feasst_deserialize(&kappa_, istr);
  feasst_deserialize(&dielectric_, istr);
  feasst_deserialize(&smoothing_distance_, istr);
}

double DebyeHuckel::energy(
    const double squared_distance,
    const int type1,
    const int type2,
    const ModelParams& model_params) {
  double distance = std::sqrt(squared_distance);
  TRACE("distance " << distance);
  if (std::abs(distance) < NEAR_ZERO) {
    TRACE("near inf");
    return NEAR_INFINITY;
  }
  const double mixed_charge = model_params.select(charge_index()).mixed_values()[type1][type2];
  //TRACE("mixed_charge " << mixed_charge);
  //TRACE("conversion_factor_ " << conversion_factor_);
  const double prefactor = mixed_charge*conversion_factor_/dielectric_;
  double en;
  if (smoothing_distance_ < 0) {
    en = prefactor*std::exp(-kappa_*distance)/distance;
  } else {
    const double mixed_cutoff = model_params.select(cutoff_index()).mixed_values()[type1][type2];
    const double dx = mixed_cutoff - distance;
    if (dx < smoothing_distance_) {
      distance = mixed_cutoff;
    }
    en = prefactor*std::exp(-kappa_*distance)/distance;
    if (dx < smoothing_distance_) {
      en *= dx / smoothing_distance_;
    }
  }
  return en;
}

void DebyeHuckel::precompute(const Configuration& config) {
  Model::precompute(config);
  conversion_factor_ = config.model_params().constants().charge_conversion();
}

}  // namespace feasst
