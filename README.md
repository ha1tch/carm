# CARM — Controlled Attention Routing and Masking

**Architectural concept isolation in hybrid AI systems.**

CARM is a mechanism for enforcing concept-level access policy in transformer
attention at the architectural layer, rather than through behavioural training
or prompt engineering. Blocked concepts receive **exactly zero** attention
weight — not approximately zero, but zero by construction — because they are
excluded from the softmax domain before any score computation occurs.

This repository accompanies the paper:

> Horacio López Barrios, *"CARM: Controlled Attention Routing and Masking —
> Architectural Concept Isolation in Hybrid AI Systems"*, 2026.
> [`paper/carm.pdf`](paper/carm.pdf)

---

## The core idea

Standard approaches to concept restriction in LLMs (prompt engineering, RLHF,
output filtering) operate on the model's *output* or *training distribution*.
They produce probabilistic tendencies, not categorical guarantees.

CARM operates on the *computation graph*. A routing policy — supplied by an
external authority — determines which concepts are accessible before any
attention score is computed. Blocked concepts are structurally unreachable,
not behaviourally suppressed.

```
  AXI Component (deterministic, domain-compiled policy)
        │
        │  P : id(v) → {ALLOW, DENY}
        ▼
  CARM Layer (routing phase, query-independent)
        A = { v ∈ V : P(id(v)) = ALLOW }
        │
        │  Accessible set A
        ▼
  ACI Component (attention over A only → output embedding)
```

The paper defines **four compliance criteria** (C1–C4) that characterise a
CARM-compliant implementation. The central guarantee:

> *CARM formally guarantees that within a compliant attention layer, no denied
> concept contributes to ACI output — by construction, not by tendency.*

---

## What this implementation demonstrates

This is a proof-of-concept operating at the **attention-component level**. It
demonstrates:

- Correct routing enforcement across 5,000 independent computations with **zero
  violations** (blocked weight = 0.0000000000 in every trial)
- Routing overhead below **0.13% at worst case** (31–159 ns routing vs
  25–366 µs attention)
- Latency stability: p99 = 54 ns across 20,000 samples
- Monotonically increasing output divergence as blocking fraction increases

**This is not a full transformer integration.** Applying CARM to a production
LLM — with residual connections, feed-forward sublayers, and autoregressive
generation — is ongoing work. See §6 of the paper for the open research
questions and planned next steps.

---

## Repository structure

```
carm/
├── src/
│   ├── routing_table.h          # O(1) routing array: API and inline lookup
│   ├── routing_table.c          # Rule application and table management
│   ├── multihead_attention.h    # MHA with CARM integration: types and API
│   ├── multihead_attention.c    # Three-phase CARM computation
│   ├── multihead_attention_test.c  # Test suite (6 tests)
│   ├── multihead_demo.c         # Interactive demonstration
│   ├── embeddings.h             # Binary embedding loader API (Phase 2)
│   └── embeddings.c             # GloVe/Word2Vec binary loader
│
├── benchmarks/
│   ├── carm_benchmark.c         # Comprehensive benchmark suite (B1–B7)
│   └── routing_bench.c          # Routing microbenchmark (O(1) vs linear)
│
├── tools/
│   ├── build_vocabulary.py      # Build concept vocabulary with TOSID labels
│   └── load_glove.py            # Load GloVe embeddings into binary format
│
├── examples/
│   └── vocabulary_1000.txt      # 1,000-concept vocabulary with TOSID assignments
│
├── paper/
│   ├── carm.pdf                 # The paper
│   ├── carm.tex                 # LaTeX source
│   ├── figures/                 # TikZ figure sources
│   └── data/                    # Benchmark data for paper figures
│
├── Makefile
├── LICENSE
└── README.md
```

---

## Building and running

**Requirements:** GCC (C11), make, libm. No external dependencies.

```bash
# Build everything
make all

# Run the test suite (6 tests, should all pass)
make test

# Run the interactive demo
make demo

# Run the comprehensive benchmark (produces CSV output)
make bench

# Run benchmark and save results
make bench > results.csv
```

Expected test output:
```
Total tests:  6
Passed:       6
Failed:       0
✓ ALL TESTS PASSED!
```

---

## The TOSID identifier scheme

CARM is agnostic to the identifier scheme used, provided identifiers are
stable and prefix-matchable. This implementation uses a 32-bit simplified
TOSID encoding:

```
0xDDCCVVVV
  DD   = Domain    (8 bits)   e.g. 0x10 = medical taxonomy
  CC   = Category  (8 bits)   e.g. 0xC5 = MED-SUP (medical supplies)
  VVVV = Variant   (16 bits)  specific concept instance
```

The routing table operates on the upper 16 bits (DD+CC), so a single rule
covers all variants within a domain-category pair. Example clinical context:

```c
// Allow all medical concepts (0x10C5****)
// Deny financial (0x11B1****) and PHI (0x13D2****)

RoutingRule rules[] = {
    { .prefix = 0x10C50000, .mask = 0xFFFF0000, .action = ALLOW },
    { .prefix = 0x11B10000, .mask = 0xFFFF0000, .action = DENY  },
    { .prefix = 0x13D20000, .mask = 0xFFFF0000, .action = DENY  },
};
```

The full TOSID specification is a separately developed system not published
in this repository. Any stable, prefix-matchable identifier scheme is
compatible with CARM.

---

## Benchmark results (reference machine: Linux x86-64, gcc 13.3, -O2)

| Metric | Value |
|--------|-------|
| Zero-leakage violations (5,000 measurements) | 0 |
| Routing overhead at worst case (5 concepts) | 0.126% |
| Routing overhead at 100 concepts | 0.043% |
| Routing latency p50 | 31 ns |
| Routing latency p99 | 54 ns |
| Routing latency p99.9 | 182 ns |

---

## Current status and ongoing work

| Component | Status |
|-----------|--------|
| Routing layer (O(1) array lookup) | Complete |
| Multi-head attention with CARM enforcement | Complete |
| Synthetic clustered embeddings | Complete |
| GloVe embeddings at 1,000 concepts | Complete |
| Additive masking variant (Phase 2 Track A) | In progress |
| Micro-universe MRE (`make mre`) | Complete |
| Sequential generation with context accumulation | In progress |
| Adversarial robustness testing | Planned |
| Full transformer stack integration | Planned |
| New CARM-native architecture (Phase 3) | Planned |
| TOSIDcoders — feedback loop (Phase 4) | Planned |

---

## Citation

```bibtex
@misc{lopezbarrios2026carm,
  author = {L\'opez Barrios, Horacio},
  title  = {{CARM}: Controlled Attention Routing and Masking ---
             Architectural Concept Isolation in Hybrid {AI} Systems},
  year   = {2026},
  url    = {https://github.com/ha1tch/carm}
}
```

---

## Using real embeddings (Phase 1.2 — optional)

**Nothing in the default build requires this.** `make test`, `make demo`,
and `make mre` all use synthetic embeddings and work with no external files.

The Phase 1.2 embedding loader (`src/embeddings.h`, `src/embeddings.c`)
provides infrastructure for loading GloVe pre-trained vectors into a binary
format for use with the 1,000-concept vocabulary. To use it:

**1. Download GloVe vectors**

```bash
# 100-dimensional GloVe trained on Wikipedia + Gigaword (822 MB)
wget https://nlp.stanford.edu/data/glove.6B.zip
unzip glove.6B.zip   # produces glove.6B.100d.txt among others
```

**2. Build the binary embedding file**

```bash
# From the repo root — outputs embeddings/embeddings_glove_100.bin
python3 tools/build_vocabulary.py \
    --vocab      examples/vocabulary_1000.txt \
    --dimension  100 \
    --output     embeddings/embeddings_glove_100
```

This writes three files: `embeddings_glove_100.bin` (read by the C loader),
`embeddings_glove_100.txt` (human-readable), and
`embeddings_glove_100_metadata.json` (coverage stats).

The `embeddings/` directory is present in the repo for this purpose.
The generated files are excluded from version control (see `.gitignore`).

The binary format is documented in `src/embeddings.h`.
`tools/load_glove.py` is a separate exploration script for testing GloVe
similarity and composition — it does not write any files.

---

## Licence

Copyright (c) 2026 haitch  
Licensed under the Apache License, Version 2.0.  
See [LICENSE](LICENSE) for details.
