#include "utils/include/serialize.h"
#include "utils/include/arguments.h"
#include "math/include/constants.h"
#include "configuration/include/physical_constants.h"
#include "configuration/include/site.h"
#include "configuration/include/model_params.h"
#include "configuration/include/configuration.h"
#include "charge/include/electric_field.h"

namespace feasst {

FEASST_MAPPER(ElectricField, argtype({{"field_strength", "1"}}));

ElectricField::ElectricField(argtype args) {
  class_name_ = "ElectricField";
  dimension_ = integer("dimension", &args, 0);
  field_strength_ = dble("field_strength", &args);
  feasst_check_all_used(args);
}

void ElectricField::precompute(const Configuration& config) {
  Model::precompute(config);
  const ModelParams& existing = config.model_params();
  // convert from V/A to kJ/mol/A/e
  conversion_factor_ = existing.constants().elementary_charge()*
    existing.constants().avogadro_constant()/1e3;
}

void ElectricField::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_model_(ostr);
  feasst_serialize_version(3905, ostr);
  feasst_serialize(dimension_, ostr);
  feasst_serialize(field_strength_, ostr);
  feasst_serialize(conversion_factor_, ostr);
}

ElectricField::ElectricField(std::istream& istr) : ModelOneBody(istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(version == 3905, "unrecognized verison: " << version);
  feasst_deserialize(&dimension_, istr);
  feasst_deserialize(&field_strength_, istr);
  feasst_deserialize(&conversion_factor_, istr);
}

double ElectricField::energy(
    const Position& wrapped_site,
    const Site& site,
    const Configuration& config,
    const ModelParams& model_params) {
  const int type = site.type();
  TRACE("type: " << type);
  const double charge = model_params.select(charge_index()).value(type);
  TRACE("charge: " << charge);
  const double distance = wrapped_site.coord(dimension_);
  TRACE("distance: " << distance);
  return -conversion_factor_*charge*field_strength_*distance;
}

}  // namespace feasst
