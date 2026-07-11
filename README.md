# CRN Dynamic CCC

A C++ simulation study of resilient spectrum access in a **cognitive radio
network (CRN)**. The project is organised in two stages:

1. **A verified spectrum-sensing baseline** (`legacy/`) that we built and validated
   first, modelling primary-user (PU) activity, secondary-user (SU) sensing errors,
   and cooperative vs. local decision making.
2. **The dynamic common-control-channel (CCC) idea** (`src/`) built on top of that
   baseline, adding cluster heads, an underlay fallback band, and a backoff-based
   recovery scheme when a PU displaces an SU cluster from its band.

## Repository layout

```
crn-dynamic-ccc/
├── src/
│   └── dynamic_ccc.cpp          # Main contribution: dynamic CCC + underlay + backoff
├── legacy/
│   └── spectrum_sensing_v1.cpp  # Verified baseline: sensing, Pmd/Pfa, majority voting
├── matlab/
│   └── plot_results.m           # Figures from the dynamic-CCC output
├── .gitignore
├── LICENSE
└── README.md
```

## The dynamic CCC model (`src/dynamic_ccc.cpp`)

Five cluster heads (CHs), each serving four secondary users, operate over 19
regular bands plus one dedicated **underlay** band. Each band's PU activity
follows a two-state Markov chain. Over 30,000 time slots the simulator tracks,
per cluster head:

- a **NORMAL → DISPLACED → ON_UNDERLAY → NORMAL** lifecycle when a PU appears,
- an exponential **backoff** with jitter before attempting underlay access,
- packet delivery, packet loss, outage slots/events, and underlay access success.

It runs for PU activity levels of 0.3, 0.5 and 0.7 and writes two CSV files per
run: a per-slot CH state log and a summary of metrics.

**Reported metrics** (written to `*_summary.csv`): per-CH packet-delivery ratio,
packets lost, packet-loss percentage, total outage slots, average outage
duration, and underlay access success rate.

## The baseline model (`legacy/spectrum_sensing_v1.cpp`)

The earlier framework models 100 bands and 10 SUs, each SU demanding 5 bands.
It supports both **Markov** and **deterministic** PU activity and compares
**local sensing** against **cooperative majority voting**, with configurable
miss-detection (Pmd) and false-alarm (Pfa) probabilities. It logs throughput,
utilisation, interference, collisions, and false-alarm / miss-detection counts
both before and after fusion.

## Build & run

Both files are single-translation-unit C++ programs (C++11 or later):

```bash
# Dynamic CCC
g++ -O2 -std=c++17 src/dynamic_ccc.cpp -o dynamic_ccc
./dynamic_ccc

# Baseline sensing framework
g++ -O2 -std=c++17 legacy/spectrum_sensing_v1.cpp -o spectrum_sensing_v1
./spectrum_sensing_v1
```

Each run writes its CSV output into the current directory (these are
git-ignored). The random seed is fixed (`123`) for reproducibility.

## Plotting the results

After running the dynamic-CCC simulation, open `matlab/plot_results.m` in MATLAB
from the folder containing the generated `packet_based_sim_pu_*_summary.csv` and
`*_ch_states.csv` files. It produces four figures: PDR per CH, packet-loss
percentage per CH, average outage duration per CH, and the CH0 state-transition
timeline at PU activity 0.7.

## Background

This work was carried out as part of a research project on cognitive radio
resource management. A short "Publication" section with the paper title,
authors, and link can be added here.

## License

Released under the MIT License. See [LICENSE](LICENSE).
