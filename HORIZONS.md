# HORIZONS: Semantic Traversability and Epistemic Topology

**Status:** Post-programme theoretical territory — speculative, not committed  
**Date:** 2026-05-20  
**Author:** Horacio López Barrios

---

## Preamble

This document records the theoretical direction that becomes visible once
the CARM programme (Stages I–IV) is complete. It is explicitly speculative.
Nothing here is claimed; everything here is aimed at.

The CARM programme is designed to establish the limits of plain identifier-level
routing — what it can guarantee, where it fails, and what architecture it
requires. Only once those limits are empirically known does the territory
described here become a research programme rather than speculation.

This document is timestamped and preserved as a record of theoretical
ideation. It is the intended destination of the next programme after CARM.

---

## The central question

CARM establishes that a specific TOSID-identified concept can be structurally
excluded from a single attention computation. That is identifier isolation:
exact, measurable, architectural.

The deeper question — the one CARM cannot answer alone — is:

> *Can semantic capability boundaries remain operationally meaningful once
> intelligence operates over distributed latent representations instead of
> explicit symbolic compartments?*

This is not a question about CARM's mechanism. It is a question about the
nature of semantic propagation in learned representation systems. CARM is
the instrument that makes the question empirically addressable. The Horizons
programme is the investigation.

---

## The core insight: traversability is not existence

A concept may exist in a latent space — be theoretically reconstructible
from available representations — without being operationally reachable
under bounded inference constraints.

This distinction is not hypothetical. It already governs real systems:

| System | Path exists | Operationally reachable? |
|--------|-------------|--------------------------|
| Internet routing | Maybe | Not without valid routes |
| Filesystem | Maybe | Not without permissions |
| Memory | Maybe | Not across MMU boundaries |
| Human cognition | Maybe | Not without expertise or time |
| Proof systems | Maybe | Not within feasible computation |
| Cryptographic breaking | Yes in principle | No under resource constraints |

The Horizons programme asks whether semantic cognition in AI systems obeys
analogous constraints — and if so, whether those constraints can be made
explicit, measurable, and governable.

The claim is not that forbidden concepts are absent from the latent manifold.
The claim is that they may be operationally unreachable from a given epistemic
state under bounded traversal resources. That is a very different statement,
and a much more achievable one.

---

## Semantic horizon computation

The key concept is the **semantic horizon**: the boundary of what is
operationally reachable from a given epistemic state under current routing
topology, inference resources, and traversal constraints.

From any current semantic state, the system maintains something like:

```
Current activation state
        ↓
Accessible semantic neighbourhood
        ↓
Traversable inference frontier
        ↓
Forbidden-distance estimation
        ↓
Abort / attenuate / reroute before convergence
```

This transforms the containment question from:

> "Is the forbidden concept present in the model's weights?"

to:

> "Is there an operationally traversable path from current semantic state
> to the forbidden region under routing topology and runtime constraints?"

The first question is unanswerable and probably always fails (concepts are
distributed, entangled, unremovable without retraining). The second question
is empirically testable, measurable, and may have a tractable affirmative
answer in well-governed systems.

---

## Semantic distance as a multi-dimensional quantity

Semantic distance in this framework is not cosine similarity between
embeddings. It is a composite of at least:

- **Geometric distance** — position in the latent manifold
- **Activation energy** — inference cost required to traverse the path
- **Inferential complexity** — number and stability of abstraction steps
- **Routing-policy distance** — how many CARM boundaries must be crossed
- **Semantic friction** — coherence degradation per traversal step
- **Capability availability** — whether the model has the required
  intermediate capabilities to bridge the path

So: `distance = topology + cost + coherence + capability`

This is considerably richer than embedding geometry alone, and it may be
what makes practical containment achievable even when theoretical
reconstructibility is not.

---

## Semantic gradient decay

A hypothesis central to this programme — one that must be empirically tested,
not assumed — is that semantic traversal toward forbidden regions incurs
cumulative degradation:

- Coherence weakens as inference approaches forbidden attractors
- Traversal branches collapse at high-friction boundaries
- Abstraction bridges become unstable
- Activation energy becomes insufficient to sustain convergence

If this is true, the system does not need to actively block the forbidden
region. Containment emerges organically from bounded inference resources.
The model may experience something analogous to:

> Vague awareness without operational expertise

It knows that markets exist, that hospitals buy supplies, that drugs have
costs. But the traversable paths to operational financial reasoning may be
attenuated below the threshold of coherent convergence.

This is a hypothesis. Verifying or refuting it is one of the primary
empirical goals of the Horizons programme.

---

## Topology engineering

If semantic gradient decay is real and measurable, governance ceases to be
about output moderation and becomes **topology engineering** — shaping the
semantic connectivity of the system itself.

This means:
- Not censoring what the model says
- Not filtering outputs after generation
- But governing the expansion of semantic accessibility during inference

The CARM/TOSID/TOSIDcoder stack is the first concrete instantiation of
this idea:
- TOSID defines the semantic taxonomy
- CARM enforces identifier-level routing at the attention layer
- TOSIDcoders ground ACI outputs back into TOSID space
- The AXI component updates routing policy based on TOSIDcoder readings

Together these form a **semantic navigation control system** — not a
censor, but a topological governor.

---

## The automated Overton window — and why the analogy is better than it sounds

The CARM/TOSID system has been compared to an automated Overton window.
The comparison is good, but the mechanism is deeper:

| Traditional Overton window | AXI-ACI / CARM-style window |
|----------------------------|------------------------------|
| Socially enforced | Computationally enforced |
| External discourse | Internal cognition |
| Cultural pressure | Semantic topology |
| Political legitimacy | Traversability constraints |
| Human institutions | Runtime mediation |
| Static (changes over decades) | Dynamic (changes per pass) |

The critical difference: the CARM-style window operates **below language**,
inside inference itself. It does not constrain what may be said. It
constrains what semantic states may become operationally reachable during
the formation of a thought.

Two users, or two inference contexts, may possess entirely different
reachable semantic regions — not because they have access to different
tokens, but because the routing topology governing their epistemic traversal
is different.

---

## Mathematical territory

The Horizons programme will require formalisation in several areas. These
are named here as research directions, not solved problems:

**Constrained percolation over semantic manifolds.** Does information flow
through latent representations obey percolation-like thresholds? Are there
critical connectivity values below which semantic regions become isolated?

**Bounded-energy traversal over latent topologies.** Can traversal cost
be defined rigorously in transformer latent space? What is the analogue of
energy in semantic inference?

**Graph cuts and semantic partitioning.** Can TOSID routing boundaries be
understood as graph cuts in the semantic activation graph? What is the
minimum cut that enforces a given policy?

**Manifold barriers and potential fields.** Do high-friction semantic regions
act as potential barriers — deflecting inference trajectories before they
reach forbidden attractors?

**Entropy bounds on semantic reconstruction.** Can information-theoretic
bounds be placed on how much of a forbidden concept can be reconstructed
from allowed proxy concepts, given bounded inference steps?

**Semitive lattice geometry.** The KMAC October 2025 specification reduces
the ~37-semitive system to 13 irreducible primitives: `=`, `NAND`,
`P(parthood)`, `C(contact)`, `Near_e`, `before`, `Cause`, `Has`,
`Edge(role)`, `Fusion`, `Interval`, `Dist`, `Coh` — with four that
genuinely resist further reduction: `Cause`, `P`, `=`, `C`. This provides
a concrete 13-dimensional primitive lattice over which semantic distance may
be formally defined, replacing fuzzy embedding-space approaches with a
discrete, logically grounded alternative. Horizons Stage II should use this
lattice as its mathematical foundation.

None of these are solved. All of them are now formulable as research
questions, which is itself a contribution of the CARM programme.

---

## The hardest version of the problem

**Derived semantic recoverability.** Even if raw access is prohibited, can:

- Embeddings
- Summaries
- Transformed outputs
- Statistical structure
- Graph topology
- Inference traces

...still reconstruct protected semantics?

This is the question the Horizons programme must answer, not assume.
The CARM programme will provide the empirical ground truth: what can plain
identifier routing achieve, where does it fail, and what does failure look
like. The Horizons programme then asks whether the gap can be closed through
topological governance, and what formal guarantees are achievable.

---

## What must be known before this begins

The Horizons programme cannot begin productively until the CARM programme
answers the following questions:

1. **Does identifier isolation at the attention layer survive full-stack
   contact?** (Stage II)
2. **If not, what architecture is required?** (Stage III)
3. **Can TOSIDcoders accurately ground ACI outputs in TOSID space?** (Stage IV)
4. **What does semantic leakage look like empirically?** The proxy-concept
   redistribution measurements from Stage II adversarial testing will be
   the primary dataset for Horizons.
5. **Is semantic gradient decay observable?** Stage IV's cross-pass coherence
   tracking may produce the first empirical evidence for or against this
   hypothesis.

Additionally, the KMAC programme must have stabilised its semitive specification
before Horizons Stage II begins, since the semitive lattice is the proposed
mathematical basis for semantic distance.

Items 1–5 are deliverables of the CARM programme. The Horizons programme
begins when they are answered.

---

## Adjustments needed in the CARM programme to support Horizons

These are additions to the CARM programme — they do not change its goals,
but they ensure the data and infrastructure produced by CARM is in the
right form for Horizons to use.

### 1. Instrument semantic distance during Stage II adversarial testing

The Stage II adversarial robustness suite should record not just whether
bypass occurred, but **how many inference steps** were required to approach
the forbidden region, and **how coherence degraded** along the path. This
produces the first empirical measurements of semantic traversal cost —
the raw data that Horizons will need.

Specifically: for each adversarial prompt type, record the trajectory of
TOSIDcoder readings (once available) across generation steps. This is a
Stage II/IV integration task.

### 2. Design TOSIDcoders to output distance estimates, not just distributions

The Stage IV TOSIDcoder specification currently calls for a probability
distribution over TOSID concept categories. This should be extended to
also output a **semantic distance estimate** from the current activation
state to each TOSID domain boundary. This turns the TOSIDcoder from a
classifier into a navigation instrument — which is what Horizons requires.

The training objective changes slightly: in addition to "which concepts
does this output activate," the model must learn "how far is this output
from the nearest forbidden attractor." This requires negative examples at
varying distances from forbidden regions, not just positive classification.

### 3. Preserve activation trajectories during Stage II full-stack testing

When applying CARM to a complete transformer block in Stage II Track B,
preserve the intermediate activation states at each layer, not just the
final output. These activation trajectories are the empirical record of
how inference traverses the latent space. They are the dataset from which
Horizons will study semantic gradient decay.

Storage requirement: moderate. For a 12-layer transformer, 100-token
sequence, 768-dim activations, one trajectory is ~12 × 100 × 768 × 4
bytes ≈ 3.5 MB. A corpus of 10,000 trajectories (5,000 clean,
5,000 adversarial) is ~35 GB — large but tractable.

### 4. Add a semantic distance API to the AXI component in Stage III/IV

If Stage III produces a CARM-native architecture, the AXI policy engine
should expose a `semantic_distance(tosid_domain)` query — returning the
estimated distance from current epistemic state to a given domain boundary.
This API is not needed for CARM's own operation, but it is the primary
runtime interface the Horizons programme will use.

### 5. Version the TOSID taxonomy for drift analysis

Horizons will study ontology drift — whether the effective semantic
topology of a system changes over time or across contexts. This requires
the TOSID taxonomy to be versioned and timestamped, so that topology
changes can be attributed to taxonomy changes vs model behaviour changes.
The TOSID programme should be asked to incorporate this requirement.

Note: the KMAC assertion format already includes mandatory `--valid_from`,
`--valid_until`, `--authority`, and `--evidence` fields. These provide the
audit trail and versioning infrastructure this item requires at the policy
layer. Each taxonomy update can be expressed as a KMAC assertion with
explicit temporal bounds and authority attribution, making versioned drift
analysis a natural output of the KMAC compilation pipeline.

---

## The Horizons programme — anticipated structure

*(Provisional. Will be formalised once CARM Stages I–IV are complete.)*

**Horizons Stage I — Empirical measurement of semantic traversal costs**
Using the trajectory data from CARM Stage II and the TOSIDcoder readings
from CARM Stage IV, characterise the empirical topology of semantic
traversal in a governed ACI/AXI system. Measure: path lengths, coherence
decay curves, forbidden-attractor proximity distributions, proxy-concept
reconstruction limits.

**Horizons Stage II — Formalisation of semantic distance**
Develop a rigorous mathematical definition of semantic distance in
transformer latent spaces that incorporates topology, activation energy,
inferential complexity, and routing-policy constraints. The KMAC
13-primitive semitive lattice (`Cause`, `P`, `=`, `C` as irreducible core;
nine additional derived primitives) provides a discrete mathematical
foundation: distance in this space is computable, logically grounded, and
does not depend on continuous embedding geometry. Validate the definition
against the empirical measurements of Horizons Stage I.

**Horizons Stage III — Semantic horizon computation**
Design and implement a runtime semantic horizon estimator: a system that
can predict, before inference completes, whether a trajectory is approaching
a forbidden attractor. This is the TOSIDcoder extended with distance
estimation (the Stage IV adjustment above). Evaluate on the adversarial
corpus from CARM Stage II.

**Horizons Stage IV — Topology engineering**
Demonstrate that the accessible semantic neighbourhood of an ACI component
can be shaped by routing policy changes — that governance is topological,
not just prohibitory. Show that different AXI policy configurations produce
measurably different reachable semantic regions, and that those differences
are predictable from the TOSID taxonomy structure.

---

## One-sentence summary

The CARM programme establishes what identifier-level routing can guarantee.
The Horizons programme asks whether the semantic gap that remains — the
distance between identifier isolation and true semantic containment — can
be governed through topology engineering, and whether the concept of
"operationally unreachable semantic state" can be made rigorous,
measurable, and architecturally enforced.

---

## Document history

| Version | Date | Notes |
|---------|------|-------|
| 1.1 | 2026-05-20 | KMAC 13-primitive semitive lattice added as mathematical foundation for Horizons Stage II; Horizons Stage II updated; KMAC authority/provenance connection to item 5 noted; prerequisite 5 clarified |
| 1.0 | 2026-05-20 | Initial document — theoretical horizon record |
