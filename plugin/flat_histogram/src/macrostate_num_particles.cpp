#include "utils/include/serialize.h"
#include "utils/include/arguments.h"
#include "math/include/histogram.h"
#include "system/include/system.h"
#include "monte_carlo/include/criteria.h"
#include "flat_histogram/include/macrostate_num_particles.h"

namespace feasst {

MacrostateNumParticles::MacrostateNumParticles(argtype * args) :
    MacrostateNumParticles(Histogram(args), args) {}
MacrostateNumParticles::MacrostateNumParticles(const Histogram& histogram,
    argtype * args) : Macrostate(histogram, args) {
  class_name_ = "MacrostateNumParticles";
  num_ = ConstrainNumParticles(
    {{"type", str("particle_type", args, "")}});
  ASSERT(num_.type() >= -2, "particle_type: " << num_.type());
}
MacrostateNumParticles::MacrostateNumParticles(const Histogram& histogram,
    argtype args) : MacrostateNumParticles(histogram, &args) {
  feasst_check_all_used(args);
}
MacrostateNumParticles::MacrostateNumParticles(argtype args) :
    MacrostateNumParticles(Histogram(&args), &args) {
  feasst_check_all_used(args);
}
std::shared_ptr<Macrostate> MacrostateNumParticles::create(argtype * args) const {
  return std::make_shared<MacrostateNumParticles>(args);
}

double MacrostateNumParticles::value(const System& system,
    const Criteria& criteria,
    const Acceptance& acceptance) {
  return num_.num_particles(system, acceptance);
}

FEASST_MAPPER(MacrostateNumParticles, argtype({{"width", "1"}, {"max", "1"}}));

std::shared_ptr<Macrostate> MacrostateNumParticles::create(std::istream& istr) const {
  return std::make_shared<MacrostateNumParticles>(istr);
}

MacrostateNumParticles::MacrostateNumParticles(std::istream& istr)
  : Macrostate(istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(version == 204, "version mismatch: " << version);
  feasst_deserialize_fstobj(&num_, istr);
}

void MacrostateNumParticles::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_macrostate_(ostr);
  feasst_serialize_version(204, ostr);
  feasst_serialize_fstobj(num_, ostr);
}

}  // namespace feasst
