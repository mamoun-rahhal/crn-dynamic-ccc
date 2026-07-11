# CRN Dynamic CCC

A C++ simulation study of a **dual-mode cognitive radio network (CRN)** with a
**self-healing common control channel (CC)** and **distributed fusion**. Instead
of relying on a centralized controller or a fixed shared control channel, the
network is fully distributed: secondary users (SUs) form clusters, each led by
an elected **cluster head (CH)** that acts as a local fusion center for
cooperative sensing and for choosing the control channel.

The system operates in two modes:

- **Interweave mode** — SUs communicate over channels currently unoccupied by the
  primary user (PU).
- **Underlay mode** — when a PU appears, the control channel is sustained via
  low-power underlay transmission so coordination survives PU activity.

When a PU displaces a cluster, the CH recovers using a **CSMA-CA-inspired
randomized backoff** to reach the underlay channel without colliding with other
CHs, then migrates back to a free regular band as soon as one is available — a
"self-healing" control channel. The design centres on two ideas from the paper:
adaptive cluster coordination and a context-aware backoff mechanism.

The project is organised in two stages: a **verified spectrum-sensing baseline**
(`legacy/`) built and validated first, and the **dynamic-CCC contribution**
(`src/`) built on top of it.

## Repository layout

```
crn-dynamic-ccc/
├── src/
│   └── dynamic_ccc.cpp          # Dynamic dual-mode CCC: clusters, underlay, backoff recovery
├── legacy/
│   └── spectrum_sensing_v1.cpp  # Verified baseline: sensing, Pmd/Pfa, majority-vote fusion
├── matlab/
│   └── plot_results.m           # Figures from the dynamic-CCC output
├── Dual-Mode Cognitive Radio Network with.pdf   # Project paper
├── .gitignore
├── LICENSE
└── README.md
```

## Dynamic dual-mode CCC (`src/dynamic_ccc.cpp`)

Five cluster heads, each serving four secondary users, operate over 19 regular
bands plus one dedicated underlay band. PU activity on each band follows a
two-state Markov chain. Across 30,000 time slots, each CH moves through a
**NORMAL → DISPLACED → ON_UNDERLAY → NORMAL** lifecycle as PUs come and go:

- When a PU appears on a CH's band, the CH becomes **displaced** and starts a
  randomized backoff (initial window, then doubling with 0–24 jitter, capped at
  512) before attempting the underlay channel.
- Underlay access is modelled probabilistically (a base collision chance that
  grows with PU activity); a successful attempt moves the CH **onto the underlay**,
  and it returns to **normal** once a free regular band is found.

Per-CH metrics written to `*_summary.csv`: packet-delivery ratio (slots where all
four SU packets were received), packets lost, packet-loss percentage, total
outage slots, average outage duration, and underlay access success rate. Runs
cover PU activity levels of 0.3, 0.5, and 0.7.

## Verified baseline (`legacy/spectrum_sensing_v1.cpp`)

The earlier framework models 100 bands and 10 SUs (each demanding 5 bands) and
was used to validate the sensing and fusion logic before the dynamic-CCC work.
It supports both **Markov** and **deterministic** PU activity and compares
**local sensing** against **cooperative majority voting**, with configurable
miss-detection (Pmd) and false-alarm (Pfa) probabilities. It logs throughput,
utilisation, interference, collisions, and false-alarm / miss-detection counts
before and after fusion.

## Build & run

Both files are single-translation-unit C++ programs (C++11 or later):

```bash
# Dynamic dual-mode CCC
g++ -O2 -std=c++17 src/dynamic_ccc.cpp -o dynamic_ccc
./dynamic_ccc

# Baseline sensing framework
g++ -O2 -std=c++17 legacy/spectrum_sensing_v1.cpp -o spectrum_sensing_v1
./spectrum_sensing_v1
```

Each run writes its CSV output into the current directory (git-ignored). The
random seed is fixed (`123`) for reproducibility.

## Plotting the results

After running the dynamic-CCC simulation, open `matlab/plot_results.m` in MATLAB
from the folder containing the generated `packet_based_sim_pu_*_summary.csv` and
`*_ch_states.csv` files. It produces four figures: PDR per CH, packet-loss
percentage per CH, average outage duration per CH, and the CH0 state-transition
timeline at PU activity 0.7.

## Publication

**Dual-Mode Cognitive Radio Network with Distributed Fusion and Self-Healing
Communication Channel.** Habib Lafi, Tareq Aljazzar, Ma'moun Rahall, Mohammad
Hafez. Department of Electrical Engineering, University of Jordan, Amman.
Unpublished manuscript.

Full paper (included in this repo):
[Dual-Mode Cognitive Radio Network with.pdf](Dual-Mode%20Cognitive%20Radio%20Network%20with.pdf).

## License

Released under the MIT License. See [LICENSE](LICENSE).
