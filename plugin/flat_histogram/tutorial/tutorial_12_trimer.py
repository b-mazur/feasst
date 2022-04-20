import argparse
import feasst as fst

print(fst.version())
parser = argparse.ArgumentParser()
parser.add_argument("--task", type=int, help="SLURM job array index", default=0)
parser.add_argument("--num_procs", type=int, help="number of processors", default=12)
parser.add_argument("--num_hours", type=float, help="number of hours before restart", default=1.)
parser.add_argument("--max_particles", type=int, help="maximum number of particles", default=270)
parser.add_argument("--cubic_box_length", type=float, help="cubic_box_length", default=8)
parser.add_argument("--temperature", type=float, help="temperature", default=0.3)
parser.add_argument("--particle", type=str, help="fstprt file that describes trimer", default=fst.install_dir() + "/forcefield/trimer_0.4L.fstprt")
parser.add_argument("--mu", type=float, help="chemical potential", default=-1)
parser.add_argument("--min_sweeps", type=int, help="minimum number of TM sweeps before termination", default=100)
args = parser.parse_args()
print("args:", args)

def config():
    config = fst.MakeConfiguration(fst.args({"cubic_box_length": str(args.cubic_box_length),
                                             "particle_type": args.particle}))
    wca_cutoff = 2**(1./6.)
    config.set_model_param("cutoff", 0, 1, wca_cutoff)
    config.set_model_param("cutoff", 1, 1, wca_cutoff)
    return config

def mc(thread, mn, mx, trials_per=str(int(1e6))):
    mc = fst.MakeMonteCarlo()
    mc.add(config())
    mc.add(fst.MakePotential(fst.MakeLennardJonesForceShift()))
    mc.set(fst.MakeThermoParams(fst.args({"beta": str(1./args.temperature),
                                          "chemical_potential": str(args.mu)})))
    mc.set(fst.MakeFlatHistogram(
        fst.MakeMacrostateNumParticles(
            fst.Histogram(fst.args({"width": "1", "max": str(mx), "min": str(mn)}))),
        # fst.MakeTransitionMatrix(fst.args({"min_sweeps": str(args.min_sweeps)})),
        fst.MakeWLTM(fst.args({"collect_flatness": "18",
                               "min_flatness": "22",
                               "min_sweeps": str(args.min_sweeps)}))))
    mc.add(fst.MakeTrialTranslate(fst.args({"weight": "1.", "tunable_param": "1."})))
    mc.add(fst.MakeTrialRotate(fst.args({"weight": "1.", "tunable_param": "20."})))
    mc.add(fst.MakeTrialTransfer(fst.args({"weight": "4", "particle_type": "0"})))
    mc.add(fst.MakeCheckEnergy(fst.args({"trials_per": trials_per, "tolerance": "0.0001"})))
    mc.add(fst.MakeTune(fst.args({"trials_per": trials_per, "stop_after_phase": "0"})))
    mc.add(fst.MakeLogAndMovie(fst.args({"trials_per": trials_per,
                                         "file_name": "clones" + str(thread),
                                         "file_name_append_phase": "True"})))
    mc.add(fst.MakeEnergy(fst.args({"trials_per_write": trials_per,
                                    "file_name": "en" + str(thread) + ".txt",
                                    "file_name_append_phase": "True",
                                    "start_after_phase": "0",
                                    "multistate": "True"})))
    mc.add(fst.MakeCriteriaUpdater(fst.args({"trials_per": trials_per})))
    mc.add(fst.MakeCriteriaWriter(fst.args({"trials_per": trials_per,
                                            "file_name": "clones" + str(thread) + "_crit.txt",
                                            "file_name_append_phase": "True"})))
    mc.set(fst.MakeCheckpoint(fst.args({"file_name": "checkpoint" + str(thread) + ".fst",
                                        "num_hours": str(0.1*args.num_procs*args.num_hours),
                                        "num_hours_terminate": str(0.9*args.num_procs*args.num_hours)})))
    return mc

windows=fst.WindowExponential(fst.args({
    "alpha": "2.5",
    "num": str(args.num_procs),
    "maximum": str(args.max_particles)})).boundaries()
print(windows)

if args.task == 0:
    clones = fst.MakeClones()
    for proc, win in enumerate(windows):
        clones.add(mc(proc, win[0], win[1]))
    clones.set(fst.MakeCheckpoint(fst.args({"file_name": "checkpoint.fst"})))
else:
    clones = fst.MakeClones("checkpoint", args.num_procs)
#clones.initialize_and_run_until_complete()
clones.initialize_and_run_until_complete(fst.args({"ln_prob_file": "ln_prob.txt",
                                                   "omp_batch": str(int(1e6))}))
print(clones.ln_prob().values())
open('clones.fst', 'w').write(clones.serialize())
