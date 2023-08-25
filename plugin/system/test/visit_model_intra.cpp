#include <cmath>
#include "utils/test/utils.h"
#include "configuration/include/domain.h"
#include "configuration/test/config_utils.h"
#include "system/include/visit_model_intra.h"
#include "system/include/lennard_jones.h"

namespace feasst {

TEST(VisitModelIntra, energy) {
  auto config = MakeConfiguration({{"cubic_side_length", "10"},
    {"particle_type", "../particle/chain10.fstprt"},
    {"add_particles_of_type0", "1"}});
  LennardJones model;
  model.precompute(config->model_params());
  // don't compute intraparticle interactions between bonded sites.
  auto visit = MakeVisitModelIntra({{"intra_cut", "1"}});
  visit->precompute(config.get());
  // set cut-off to 2.5 so only beads 2 away can interact.
  // due to periodic boundary conditions matching exactly the length,
  // each bead interacts once each at a distance of 2
  config->set_model_param("cutoff", 0, 2.5);

  model.compute(config.get(), visit.get());
  const double pe_lj =  4*(std::pow(2, -12) - std::pow(2, -6));
  EXPECT_NEAR(10*pe_lj, visit->energy(), NEAR_ZERO);

  // test exclusion of first site (no 0-2 or 0-8 interaction)
  Select all = config->selection_of_all();
  Select ignore;
  ignore.add_site(0, 0);
  all.exclude(ignore);

  model.compute(all, config.get(), visit.get());
  EXPECT_NEAR(8*pe_lj, visit->energy(), NEAR_ZERO);

  auto visit2 = test_serialize<VisitModelIntra, VisitModel>(*visit);
  model.compute(all, config.get(), visit2.get());
  EXPECT_NEAR(visit->energy(), visit2->energy(), NEAR_ZERO);
}

}  // namespace feasst
