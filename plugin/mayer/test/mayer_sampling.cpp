#include <cmath>
#include "utils/test/utils.h"
#include "math/include/utils_math.h"
#include "math/include/random_mt19937.h"
#include "configuration/include/domain.h"
#include "system/test/sys_utils.h"
#include "system/include/hard_sphere.h"
#include "system/include/lennard_jones.h"
#include "system/include/model_two_body_factory.h"
#include "system/include/potential.h"
#include "system/include/thermo_params.h"
#include "models/include/square_well.h"
#include "monte_carlo/test/monte_carlo_utils.h"
#include "steppers/include/log.h"
#include "steppers/include/movie.h"
#include "steppers/include/tune.h"
#include "monte_carlo/include/trial_rotate.h"
#include "monte_carlo/include/trial_translate.h"
#include "monte_carlo/include/monte_carlo.h"
#include "monte_carlo/include/run.h"
#include "mayer/include/mayer_sampling.h"
#include "charge/include/coulomb.h"
#include "charge/include/utils.h"
#include "models/include/lennard_jones_force_shift.h"

namespace feasst {

TEST(MayerSampling, serialize) {
  //auto mayer = MakeMayerSampling({{"num_attempts_per_cycle", "1"}});
  auto mayer = MakeMayerSampling();
  MayerSampling mayer2 = test_serialize(*mayer);
}

TEST(MayerSampling, ljb2) {
  System system = two_particle_system();
  system.add_to_reference(MakePotential(MakeHardSphere()));
  /// HWH notes: does this need a max?
  const int nTrialsEq = 1e4, nTrials = 2e4;
  //const int nTrialsEq = 1e6, nTrials = 1e6;
  Configuration * config = system.get_configuration();
  config->set_model_param("cutoff", 0, NEAR_INFINITY);
  EXPECT_EQ(config->model_params().select("cutoff").value(0), NEAR_INFINITY);
  const double boxl = 2*(config->model_params().select("cutoff").value(0));
  config->set_side_lengths(Position().set_vector({boxl, boxl, boxl}));
  system.set(MakeThermoParams({{"beta", "1."},
    {"chemical_potential", "-2.775"}}));
  MayerSampling criteria;
  auto translate = MakeTrialTranslate({{"new_only", "true"}, {"reference_index", "0"}, {"weight", "0.75"}});
  //auto translate = MakeTrialTranslate({{"tunable_param", "0.5"}});
  translate->precompute(&criteria, &system);
  criteria.set_current_energy(system.energy());
  auto random = MakeRandomMT19937();
  //auto random = MakeRandomMT19937({{"seed", "1678889969"}});
  for (int iTrial = 0; iTrial < nTrialsEq + nTrials; ++iTrial) {
    translate->attempt(&criteria, &system, random.get());
  }
  const double b2 = 2./3.*PI*criteria.second_virial_ratio();
  EXPECT_NEAR(-5.3, b2, 15);
  EXPECT_GT(std::abs(2.0944-b2), 0.0001); // HS value

  std::shared_ptr<Criteria> crit2 = test_serialize<MayerSampling, Criteria>(criteria);
  EXPECT_EQ(system.thermo_params().beta(), 1.);
}

MayerSampling ljb2(const int trials, const int num_beta_taylor = 0) {
  MonteCarlo mc;
  { // initialize system
    auto config = MakeConfiguration({{"cubic_side_length", "1000"},
                                     {"particle_type0", "../particle/lj.txt"},
                                     {"add_particles_of_type0", "2"}});
    config->set_model_param("cutoff", 0, config->domain().side_length(0)/2.);
    mc.add(config);
    mc.add(MakePotential(MakeLennardJones()));
  }
  mc.add_to_reference(MakePotential(MakeHardSphere()));
  mc.set(MakeThermoParams({{"beta", "1."}, {"chemical_potential", "-2.775"}}));
  mc.set(MakeMayerSampling({{"num_beta_taylor",
                          str(num_beta_taylor)}}));
  mc.add(MakeTrialTranslate({{"new_only", "true"}, {"reference_index", "0"}, {"weight", "0.75"}}));
  auto mc2 = test_serialize_unique(mc);
  mc2->attempt(trials);
  std::stringstream ss;
  mc2->criteria().serialize(ss);
  MayerSampling mayer(ss);
  return mayer;
}

TEST(MonteCarlo, ljb2) {
  MayerSampling mayer = ljb2(1e4);
  const double b2 = 2./3.*PI*mayer.second_virial_ratio();
  DEBUG("b2 " << b2);
  EXPECT_NEAR(-5.3, b2, 20);
  EXPECT_GT(std::abs(2.0944 - b2), 0.0001); // HS value
}

TEST(MonteCarlo, ljb2_LONG) {
  MayerSampling mayer = ljb2(1e7);
  const double b2 = 2./3.*PI*mayer.second_virial_ratio();
  DEBUG("b2 " << b2);
  EXPECT_NEAR(-5.3, b2, 0.3);
  EXPECT_GT(std::abs(2.0944 - b2), 0.0001); // HS value
}

TEST(MonteCarlo, ljb2_beta_deriv_LONG) {
  MayerSampling mayer = ljb2(1e7, 3);
  const double b2 = 2./3.*PI*mayer.second_virial_ratio();
  DEBUG("b2 " << b2);
  EXPECT_NEAR(-5.3, b2, 0.3);
  EXPECT_GT(std::abs(2.0944 - b2), 0.0001); // HS value
  const double reffac = 2.*PI/3./mayer.mayer_ref().average();
  int index = 0;
  DEBUG(index << "," << reffac*mayer.mayer().average());
  for (const Accumulator& acc : mayer.beta_taylor()) {
    ++index;
    DEBUG(index << "," << reffac*acc.average());///factorial(index));
  }
  // from https://github.com/usnistgov/mayer-extrapolation
  EXPECT_NEAR(-5.322731267117734, 2.*PI/3.*mayer.beta_taylor(0), 0.2);
  EXPECT_NEAR(-9.281005175555862, 2.*PI/3.*mayer.beta_taylor(1), 0.2);
  EXPECT_NEAR(-2.810611771043566, 2.*PI/3.*mayer.beta_taylor(2), 0.2);
  EXPECT_NEAR(-0.649558510117600, 2.*PI/3.*mayer.beta_taylor(3), 0.2);
}

// Check SPCE

TEST(MayerSampling, square_well_LONG) {
  MonteCarlo mc;
  auto config = MakeConfiguration({{"cubic_side_length", "10"},
    {"particle_type", "../particle/lj.txt,../particle/lj.txt"},
    {"add_particles_of_type0", "1"}, {"add_particles_of_type1", "1"}});
  mc.add(config);
  EXPECT_EQ(2, mc.configuration().num_particles());
  EXPECT_EQ(1, mc.configuration().num_particles_of_type(0));
  mc.add(MakePotential(MakeSquareWell()));
  mc.add_to_reference(MakePotential(MakeHardSphere()));
  const double temperature = 2;
  mc.set(MakeThermoParams({{"beta", str(1./temperature)}}));
  mc.set(MakeMayerSampling());
  mc.add(MakeTrialTranslate({{"new_only", "true"}, {"reference_index", "0"},
    {"tunable_param", "1"}, {"particle_type", "1"}}));
  std::string trials_per = "1e4";
  mc.add(MakeLog({{"trials_per_write", trials_per}, {"output_file", "tmp/sqw.txt"}}));
  mc.add(MakeMovie({{"trials_per_write", trials_per}, {"output_file", "tmp/sqw.xyz"}}));
  mc.add(MakeTune({{"trials_per_write", trials_per}, {"output_file", "tmp/tune.txt"}}));
  auto mc2 = test_serialize_unique(mc);
  mc2->attempt(1e6);
  std::stringstream ss;
  mc2->criteria().serialize(ss);
  MayerSampling mayer(ss);
  const double b2_reduced_analytical = 1-(3*3*3-1)*(std::exp(1/temperature)-1);
  DEBUG(mayer.mayer().str());
  DEBUG("std " << 10*mayer.second_virial_ratio_block_stdev());
  EXPECT_NEAR(b2_reduced_analytical, mayer.second_virial_ratio(), 10*mayer.second_virial_ratio_block_stdev());
}

TEST(MayerSampling, cg4_rigid_LONG) {
  MonteCarlo mc;
  auto config = MakeConfiguration({{"cubic_side_length", "1000"},
    {"particle_type0", "../plugin/chain/particle/cg4_mab.txt"}});
  config->add_particle_type("../plugin/chain/particle/cg4_mab.txt");
  config->add_particle_of_type(0);
  config->add_particle_of_type(1);
  mc.add(config);
  EXPECT_EQ(2, mc.configuration().num_particles());
  EXPECT_EQ(1, mc.configuration().num_particles_of_type(0));
  mc.add(MakePotential(MakeSquareWell()));
  mc.add_to_reference(MakePotential(MakeHardSphere()));
  const double temperature = 0.7092;
  mc.set(MakeThermoParams({{"beta", str(1./temperature)}}));
  mc.set(MakeMayerSampling());
  mc.add(MakeTrialTranslate({{"new_only", "true"}, {"reference_index", "0"},
    {"tunable_param", "1"}, {"particle_type", "1"}}));
  mc.add(MakeTrialRotate({{"new_only", "true"}, {"reference_index", "0"},
    {"tunable_param", "40"}}));
  std::string trials_per = "1e4";
  mc.add(MakeLog({{"trials_per_write", trials_per}, {"output_file", "tmp/cg4.txt"}}));
  mc.add(MakeMovie({{"trials_per_write", trials_per}, {"output_file", "tmp/cg4.xyz"}}));
  mc.add(MakeTune());
  auto mc2 = test_serialize_unique(mc);
  mc2->attempt(1e6);
  std::stringstream ss;
  mc2->criteria().serialize(ss);
  MayerSampling mayer(ss);
  EXPECT_NEAR(0.46, mayer.second_virial_ratio(), 0.15);
}

//// HWH: there is something wrong with mayer sampling + Coulomb
//// Table 2 of https://pubs.acs.org/doi/pdf/10.1021/jp0710685
//// Fig 1 of https://doi.org/10.1063/1.5016165
//TEST(MayerSampling, SPCE_LONG) {
//  MonteCarlo mc;
//  { auto config = MakeConfiguration({{"cubic_side_length", str(NEAR_INFINITY)}});
//    config->add_particle_type("../particle/spce.txt");
//    config->add_particle_type("../particle/spce.txt", "2");
//    for (int stype = 0; stype < config->num_site_types(); ++stype) {
//      config->set_model_param("cutoff", stype, 100.);
//      // HWH Why is there dependence on the cutoff? If large, it drifts too far away
//      //config->set_model_param("cutoff", stype, config->domain().side_length(0)/2.);
//    }
//    config->add_particle_of_type(0);
////    config->add_particle_of_type(0);
//    config->add_particle_of_type(1);
//    mc.add(config);
//  }
//  mc.add(MakePotential(MakeModelTwoBodyFactory({MakeLennardJones(), MakeCoulomb()})));
//  //mc.add_to_reference(MakePotential(MakeLennardJones()));
//  auto ref = MakePotential(MakeHardSphere());
//  ref->set_model_params(mc.configuration());
//  ref->set_model_param("sigma", 0, 30);
//  ref->set_model_param("sigma", 2, 30);
//  mc.add_to_reference(ref);
//  //const double temperature = 373; // kelvin
//  //const double temperature = 300; // kelvin
//  //const double temperature = 400; // kelvin
//  const double temperature = 500; // kelvin
//  //const double temperature = 1e3; // kelvin
//  //const double temperature = 773; // kelvin
//  mc.set(MakeThermoParams({{"beta", str(1./kelvin2kJpermol(temperature))}}));
//  auto mayer = MakeMayerSampling();
//  mc.set(mayer);
//  //mc.add(MakeTrialTranslate({{"new_only", "true"}, {"reference_index", "0"},
//  //  {"tunable_param", "1"}}));
//  mc.add(MakeTrialTranslate({{"new_only", "true"}, {"reference_index", "0"},
//    {"tunable_param", "1"}, {"particle_type", "1"}}));
//  mc.add(MakeTrialRotate({{"new_only", "true"}, {"reference_index", "0"},
//    {"tunable_param", "40"}}));
//  std::string trials_per = "1e5";
//  mc.add(MakeLog({{"trials_per", trials_per}, {"output_file", "tmp/spce.txt"}}));
//  mc.add(MakeMovie({{"trials_per", trials_per}, {"output_file", "tmp/spce.xyz"}}));
//  mc.add(MakeTune({{"trials_per", trials_per}}));
//  mc.attempt(1e6);
//  mc.run(MakeRemove({{"name", "Tune"}}));
//  mc.attempt(1e7);
//  mayer = MakeMayerSampling();
//  mc.set(mayer);
//  mc.attempt(1e7);
//  INFO("mayer ref " << mayer->mayer_ref().str());
//  double b2hs = 2./3.*PI*std::pow(mc.system().reference(0, 0).model_params().sigma().value(0), 3); // A^3
//  INFO(mc.system().reference(0, 0).model_params().sigma().value(0));
//  //double b2hs = 2./3.*PI*std::pow(mc.configuration().model_params().sigma().value(0), 3); // A^3
//  INFO("b2hs(A^3) " << b2hs);
//  b2hs *= 1e-30*1e3*mc.configuration().physical_constants().avogadro_constant(); // L/mol
//  INFO("b2hs(L/mol) " << b2hs);
//  INFO("b2spce/b2hs " << mayer->second_virial_ratio());
//  INFO("b2spce(L/mol) " << b2hs*mayer->second_virial_ratio());
//  EXPECT_NEAR(-0.4082, b2hs*mayer->second_virial_ratio(), 0.006);
//  //EXPECT_NEAR(-0.08596, b2hs*mayer->second_virial_ratio(), 0.006);
//}

// https://dx.doi.org/10.1063/1.4918557
TEST(MayerSampling, trimer_LONG) {
  MonteCarlo mc;
  //mc.set(MakeRandomMT19937({{"seed", "1663862890"}}));
  { auto config = MakeConfiguration({{"cubic_side_length", str(NEAR_INFINITY)}});
    config->add_particle_type("../particle/trimer_0.4L.txt");
    config->add_particle_type("../particle/trimer_0.4L.txt");
    config->add_particle_of_type(0);
    config->add_particle_of_type(1);
    const double rwca = std::pow(2, 1./6.);
    config->set_model_param("cutoff", 0, 1, rwca);
    config->set_model_param("cutoff", 0, 3, rwca);
    config->set_model_param("cutoff", 1, 2, rwca);
    config->set_model_param("cutoff", 2, 3, rwca);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[0][0], 3, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[0][1], rwca, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[0][2], 3, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[0][3], rwca, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[1][1], rwca, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[1][2], rwca, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[1][3], rwca, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[2][2], 3, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[2][3], rwca, 1e-14);
    EXPECT_NEAR(config->model_params().select("cutoff").mixed_values()[3][3], rwca, 1e-14);
    mc.add(config);
  }
  mc.add(MakePotential(MakeLennardJonesForceShift()));
  auto ref = MakePotential(MakeHardSphere());
  //ModelParams params = mc.system().configuration().model_params().deep_copy();
  auto params = ref->model_params(mc.system().configuration()).deep_copy();
  for (int i = 0; i < mc.system().configuration().num_site_types(); ++i) {
    for (int j = 0; j < mc.system().configuration().num_site_types(); ++j) {
      params.set("sigma", i, j, 0.);
    }
  }
  params.set("sigma", 0, 0, 1.);
  params.set("sigma", 0, 2, 1.);
  params.set("sigma", 2, 2, 1.);
  ref->set(params);
  mc.add_to_reference(ref);
  mc.set(MakeThermoParams({{"beta", str(1./0.815)}}));
  auto mayer = MakeMayerSampling();
  mc.set(mayer);
  mc.add(MakeTrialTranslate({{"new_only", "true"}, {"reference_index", "0"},
    {"tunable_param", "1"}, {"particle_type", "1"}}));
  mc.add(MakeTrialRotate({{"new_only", "true"}, {"reference_index", "0"},
    {"tunable_param", "40"}}));
  const std::string trials_per = "1e4";
//  mc.add(MakeLogAndMovie({{"trials_per_write", trials_per}, {"output_file", "tmp/trib"}}));
  mc.attempt(1e6);
  double b2hs = 2./3.*PI*std::pow(mc.configuration().model_params().select("sigma").value(0), 3); // A^3
  DEBUG(mayer->second_virial_ratio());
  DEBUG(b2hs*mayer->second_virial_ratio());
  DEBUG("mayer: " << mayer->mayer().str());
  DEBUG("mayer_ref: " << mayer->mayer_ref().str());
  EXPECT_NEAR(0, mayer->mayer().average(), 4*mayer->mayer().block_stdev());
}

}  // namespace feasst
