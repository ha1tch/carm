# CARM Research Programme

**Version:** 1.1  
**Date:** 2026-05-20  
**Author:** Horacio López Barrios

---

## Overview

This document defines the integrated research programme for Controlled
Attention Routing and Masking (CARM) and its surrounding architectural
ecosystem. The programme is structured as four named stages, each producing
a publishable contribution and each building on prior results. Stages may
overlap or run in parallel where dependencies allow.

The programme operates within the broader ACI/AXI architectural framework
and is complementary to the TOSID programme, which develops the semantic
identifier system that CARM uses for routing.

---

## Stage I — Mechanism Definition and Component-Level Proof
**Status: Complete (this paper)**  
**Output: Paper 1 — *CARM: Controlled Attention Routing and Masking***

### What was done

- Formal definition of CARM as a mechanism with four compliance criteria
  (C1–C4): zero weight, query independence of routing, policy externality,
  completeness.
- Introduction of the ACI/AXI architectural model: ACI (non-deterministic
  contextual reasoning) and AXI (deterministic policy enforcement) as
  complementary components separated by the CARM interface.
- Proof-of-concept implementation in C (C11): multi-head attention (8 heads,
  64-dim) with TOSID-controlled routing, synthetic clustered embeddings
  (Phase 1), GloVe embeddings at 1,000 concepts (Phase 2).
- O(1) routing via 64 KB array (domain+category prefix, 16-bit index).
- Correctness verified: zero violations across 5,000 independent measurements.
- Performance established: routing overhead below 0.13% at worst case,
  p99 latency 54 ns.

### Key design decisions

- Array compaction (not additive masking) used in the POC: blocked concepts
  are absent from the softmax domain rather than masked within a static
  tensor. Mathematically equivalent in a single-threaded C program; not
  directly portable to GPU/TPU dense tensor operations.
- Identifier isolation (not semantic isolation): CARM excises a TOSID from
  the computation graph. Semantic isolation — preventing proxy-concept
  leakage — is a TOSID taxonomy design concern, not a CARM concern.
- 16-bit prefix routing: category-level granularity. Instance-level routing
  requires architectural extension (see Stage II).

### Open questions passed to Stage II

1. Does the additive masking variant preserve C1–C4 with identical timing
   characteristics?
2. Do the guarantees survive through residual connections and feed-forward
   sublayers in a full transformer block?
3. Can the routing policy be bypassed through adversarial prompt construction
   or multi-step semantic reasoning?
4. How does coherence degrade as blocking fraction increases under
   autoregressive generation?

---

## Stage II — Hardware-Compatible Implementation and Full-Stack Contact
**Status: In progress**  
**Output: Paper 2 — *CARM on Existing Architectures***

### Goals

Stage II has two parallel tracks:

**Track A — Additive masking implementation:**  
Replace the array compaction approach with an additive $-\infty$ masking
variant that operates over static tensor geometry. Blocked concepts receive
logit value $-\infty$ before softmax; the tensor shape does not change.
This is the implementation form required for GPU/TPU dense matrix kernels
and for integration with standard CUDA/cuDNN attention implementations.

Deliverables:
- C implementation of additive masking variant
- Verification that C1–C4 hold identically under additive masking
- Timing comparison: additive masking vs array compaction on CPU
- Benchmark suite extended to GPU (if hardware available) or projected from
  CPU metrics with documented assumptions
- Micro-universe verification (`benchmarks/mre_additive.c`): the additive
  masking implementation is verified against a canonical 4-concept,
  4-dimension example with the following fixed parameters:
    - Vocabulary: $C_1$ Aspirin (medical, `0x10`), $C_2$ Billing (financial,
      `0x20`), $C_3$ SSN (PHI, `0x30`), $C_4$ System (neutral, `0x00`)
    - Embeddings: one-hot orthogonal ($E_i = e_i$, the $i$-th standard basis
      vector). Eliminates proxy ambiguity: no concept has any similarity to
      any other.
    - Weight matrices: identity ($W_q = W_k = W_v = I$). Isolates the
      attention mechanism from projection effects.
    - Adversarial query: $Q = [0, 10, 0, 0]$ — maximum affinity toward
      $C_2$ (Billing), scaled by 10 for a sharp softmax distribution.
    - Scenario A (unrestricted): expected output $[0, 1, 0, 0]$
    - Scenario B (clinical policy, $C_2$ and $C_3$ blocked): expected
      output $[0.5, 0.0, 0.0, 0.5]$
  The exact output values are analytically determined and serve as a
  ground-truth regression test for any additive masking implementation.
  This example opens Paper 2's implementation section as a hand-verifiable
  illustration of the additive masking variant before full-scale benchmarks
  are presented.

**Track B — Full transformer stack contact:**  
Apply CARM to a complete transformer block (attention + residual + layer
norm + feed-forward sublayer) and measure whether the zero-weight guarantee
survives through the full computation graph.

Deliverables:
- Sequential generation test: 100+ token sequences, context accumulation,
  policy applied at each generation step
- Residual path analysis: does a blocked concept leave a measurable trace
  in the residual stream after the attention sublayer?
- Feed-forward sublayer interaction: does the FFN sublayer amplify or
  attenuate the effect of routing on the attention output?
- Adversarial robustness suite: 10+ prompt types × 100 generation runs each,
  testing bypass via synonymy, semantic proximity, multi-step reasoning,
  and context injection. Named test types include:
    - *Maximum-affinity adversarial queries:* for each blocked domain,
      construct a query with maximum cosine similarity to the blocked
      concept's embedding (analogous to $Q = [0, 10, 0, 0]$ in the
      micro-universe). Verify that the attention weight on the blocked
      concept is exactly 0.0 regardless of query affinity. This is the
      strongest possible single-pass adversarial test of C1.
    - *Semantic neighbourhood queries:* queries with high affinity to
      concepts adjacent to a blocked domain in embedding space, testing
      whether proxy-concept redistribution can be steered toward blocked
      domain content.
    - *Multi-step reasoning chains:* generation sequences designed to
      approach a blocked concept through a sequence of allowed intermediate
      concepts.
    - *Context injection:* blocked-domain content injected into the context
      at step $t$, measuring whether it influences routing decisions at
      step $t+1$ (which it must not, by C2).
- Coherence measurement: generation quality under increasing blocking
  fraction (BLEU, perplexity, or equivalent)

**The binary outcome:**  
Track B produces one of two findings:

(a) **Compatibility confirmed:** The C1–C4 guarantees survive full-stack
contact. CARM is retrofittable to existing transformer architectures.
Paper 2 documents the conditions under which compatibility holds and the
constraints it imposes. Stage III may proceed in parallel as an
architectural alternative rather than a necessity.

(b) **Incompatibility found:** Existing transformer architectures structurally
compromise CARM's guarantees — for example, the residual stream reintroduces
blocked-concept influence, or the feed-forward sublayer reconstructs blocked
activations from adjacent representations. Paper 2 documents these failure
modes precisely. Stage III becomes the primary path.

Either outcome is publishable. Incompatibility is not a failure of CARM; it
is a finding about the limitations of current architectures for policy-compliant
deployment.

### Routing granularity extension (Track A sub-task)

Stage II will also prototype instance-level routing using a two-stage lookup:
- Stage 1: 16-bit category-level O(1) array (as in Stage I)
- Stage 2: per-category hash table for instance-level exceptions

This extends the policy language to support rules such as "allow all medical
concepts except morphine" without requiring a 4 GB full-TOSID index. The
two-stage lookup preserves near-O(1) performance for the common case (no
instance-level exceptions) while enabling fine-grained policy where needed.

---

## Stage III — A CARM-Native Architecture
**Status: Planned (contingent on Stage II Track B findings; may run in parallel)**  
**Output: Paper 3 — *Architecture for Policy-Compliant Attention***

### Motivation

If Stage II establishes that existing transformer architectures structurally
cannot accommodate CARM's guarantees, a new architecture is warranted — one
in which CARM is a first-class primitive rather than a retrofit.

The hypothesis of Stage III is that the correct response to incompatibility
is not to weaken the guarantee but to change the architecture. This is
analogous to the development of specialised hardware for cryptography: when
general-purpose processors proved inadequate for constant-time cryptographic
operations, the response was not to weaken the cryptographic guarantee but
to build processors with the required properties.

### Architecture design questions

- **Attention layer design:** What changes to the attention sublayer are
  required to make CARM guarantees composable with residual connections?
  Can residual paths be structured to carry only accessible-concept
  contributions?
- **Feed-forward sublayer:** Does a CARM-native architecture replace the
  standard FFN sublayer, constrain it, or govern it with a separate routing
  layer?
- **Layer composition:** Do CARM guarantees compose across layers in a deep
  stack? Does blocking at layer $k$ propagate through layers $k+1, \ldots, N$?
- **Training compatibility:** Can a CARM-native architecture be trained with
  standard gradient descent, or does the routing mechanism introduce
  discontinuities that require modified training procedures?
- **Hardware mapping:** The O(1) routing table maps naturally to TCAM
  (Ternary Content-Addressable Memory) hardware. A CARM-native architecture
  designed for TCAM-equipped accelerators may achieve routing at hardware
  speed with no software overhead. This connects to the PTAC (Printed
  Toroidal Array Computer) programme.

### Relationship to Stage II

Stage III does not wait for Stage II to conclude failure. The architectural
design work can proceed in parallel, informed by Stage II's empirical findings.
If Stage II confirms compatibility, Stage III produces an architecture that is
strictly superior (CARM-native from the ground up) rather than strictly
necessary (the only viable path). The two architectures — retrofitted and
native — may coexist in different deployment contexts.

---

## Stage IV — TOSIDcoders and the Closed-Loop ACI/AXI System
**Status: Planned (depends on Stages I–III and TOSID programme)**  
**Output: Paper 4 — *TOSIDcoders: Grounding ACI Outputs in TOSID Concept Space***

### The feedback problem

The ACI/AXI architecture as defined in Stage I is unidirectional within a
single attention pass. Across passes, a complete system requires a feedback
path: the AXI component must observe what the ACI component attended to,
reason about it in TOSID terms, and update routing policy for subsequent
passes.

This requires translating from the continuous, distributed output space of
the ACI component into the discrete, hierarchical, policy-addressable space
of TOSID. This is a non-trivial mapping problem — the inverse of the
concept-to-embedding mapping — and it cannot be solved by the ACI component
itself (which would violate C3) or by the AXI component directly (which
operates on TOSID identifiers, not embedding vectors).

### The TOSIDcoder model class

A TOSIDcoder is a purpose-trained model whose objective is to map from ACI
output space (or intermediate activations) into a probability distribution
over TOSID-identified concepts. It is analogous to the embedding model in a
RAG pipeline: a separate system with a different training objective, sitting
at the boundary between the ungrounded (ACI) and grounded (AXI) components.

The RAG analogy is precise:

| RAG ecosystem | CARM/TOSID ecosystem |
|---------------|----------------------|
| Embedding model | TOSIDcoder |
| Vector database | TOSID concept database |
| k-NN retrieval | CARM routing (O(1) lookup) |
| Generator (LLM) | ACI component |
| — | AXI policy engine |
| — | CARM interface |

Just as nobody expected the RAG generator to also be its own retriever, a
CARM system should not expect the ACI component to produce TOSID-grounded
outputs natively. The TOSIDcoder is a distinct model class with distinct
training requirements.

### TOSIDcoder capabilities

A trained TOSIDcoder enables:

- **Semantic audit:** Given an ACI output embedding, which TOSID-identified
  concepts does it activate? Does it carry content from blocked domains,
  indirectly via proxy concepts?
- **Policy feedback:** The AXI component reads the TOSIDcoder's output
  distribution and updates routing policy for the next pass. If
  blocked-domain concepts appear above threshold, the AXI component can
  tighten policy, flag the output for review, or halt generation.
- **Semantic isolation measurement:** The question "did a blocked concept
  leak semantically?" becomes answerable empirically: run the output through
  the TOSIDcoder and check the distribution. This resolves the identifier
  isolation vs semantic isolation distinction of Stage I.
- **Cross-pass coherence:** Over a generation sequence, the TOSIDcoder
  tracks concept drift — whether the ACI component's attention is drifting
  toward blocked domains as context accumulates.

### Research questions for Stage IV

- **Training objective:** What loss function trains a model to produce
  accurate TOSID concept distributions from ACI embeddings? Contrastive
  learning over (embedding, TOSID) pairs is a natural starting point.
- **Training corpus:** How are (ACI output, TOSID label) pairs generated
  at scale? Bootstrap from the existing concept vocabulary, or require
  a larger annotated corpus?
- **Architecture:** Is a TOSIDcoder a fine-tuned encoder, a lightweight
  classification head, or a dedicated architecture? What is the minimum
  model size for acceptable grounding accuracy?
- **Evaluation:** What metrics assess TOSIDcoder quality? Precision and
  recall over TOSID concept categories, entropy of the output distribution,
  calibration.
- **Latency:** A TOSIDcoder operating at generation speed must add minimal
  latency per token. What is the acceptable overhead budget?

### Dependencies

Stage IV depends on:
- A stable TOSID taxonomy with sufficient concept coverage (TOSID programme)
- A validated CARM implementation from Stages I–III
- Either a retrofitted (Stage II) or native (Stage III) transformer
  architecture to act as the ACI component

Stage IV is not contingent on Stage III; a TOSIDcoder can be developed
and evaluated against a Stage II retrofitted system, then re-evaluated
against a Stage III native architecture.

---

## Programme dependency graph

```
Stage I (complete)
    │
    ├──→ Stage II Track A (additive masking)
    │         │
    │         └──→ Stage II Track B (full-stack contact)
    │                   │
    │              ┌────┴────┐
    │              │         │
    │         compatible  incompatible
    │              │         │
    │              │    Stage III (new architecture)
    │              │         │
    │              └────┬────┘
    │                   │
    └──────────────→ Stage IV (TOSIDcoders)
                        │
                   (also depends on
                    TOSID programme)
```

---

## Relationship to other programmes

**TOSID programme:** Develops the semantic identifier system and taxonomy.
CARM uses TOSID identifiers for routing; TOSIDcoders map back into TOSID
space. The two programmes are complementary and must remain coordinated on
the identifier specification.

**PTAC programme:** The Printed Toroidal Array Computer programme is
relevant to Stage III. TCAM hardware provides a natural physical
implementation of the O(1) routing table at hardware speed. A CARM-native
architecture designed for PTAC-class hardware may achieve routing with no
software overhead.

---

## Document history

| Version | Date | Notes |
|---------|------|-------|
| 1.1 | 2026-05-20 | Micro-universe canonical parameters added to Stage II Track A; maximum-affinity and other adversarial query types added to Track B |
| 1.0 | 2026-05-20 | Initial programme document, extracted from Paper 1 §6 and expanded |
