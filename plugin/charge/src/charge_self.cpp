#include <cmath>
#include "utils/include/serialize.h"
#include "math/include/constants.h"
#include "configuration/include/physical_constants.h"
#include "configuration/include/site.h"
#include "configuration/include/model_params.h"
#include "configuration/include/configuration.h"
#include "charge/include/charge_self.h"

namespace feasst {

FEASST_MAPPER(ChargeSelf,);

void ChargeSelf::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_model_(ostr);
  feasst_serialize_version(8833, ostr);
  feasst_serialize(alpha_, ostr);
  feasst_serialize(conversion_factor_, ostr);
}

ChargeSelf::ChargeSelf(std::istream& istr) : ModelOneBody(istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(version == 8833, "unrecognized verison: " << version);
  feasst_deserialize(&alpha_, istr);
  feasst_deserialize(&conversion_factor_, istr);
}

double ChargeSelf::energy(
    const Position& wrapped_site,
    const Site& site,
    const Configuration& config,
    const ModelParams& model_params) {
  const int type = site.type();
  const double charge = model_params.select(charge_index()).value(type);
  TRACE("charge " << charge);
  TRACE("alpha " << alpha_);
  TRACE("PI " << PI);
  TRACE("conversion_factor_ " << conversion_factor_);
  return -charge*charge*conversion_factor_*alpha_/std::sqrt(PI);
}

void ChargeSelf::precompute(const Configuration& config) {
  Model::precompute(config);
  const ModelParams& existing = config.model_params();
  alpha_ = existing.property("alpha");
  conversion_factor_ = existing.constants().charge_conversion();
  DEBUG("conversion_factor_ " << conversion_factor_);
}

}  // namespace feasst
