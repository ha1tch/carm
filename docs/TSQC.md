# The Tiananmen Square Condition

**Document status:** Working draft  
**Context:** Dual-use implications of the CARM mechanism  
**Relation to CARM paper:** Candidate for Section 6 (Discussion) or standalone position paper  
**Date:** May 22, 2026  

---

## 1. The Dual-Use Problem, Stated Precisely

The CARM mechanism provides a structural guarantee: a concept identified by
its TOSID identifier receives identically zero attention weight during
inference, regardless of how the query is phrased, regardless of the model's
training distribution, regardless of adversarial prompt construction. This
guarantee is what makes CARM valuable for legitimate policy enforcement in
regulated domains — clinical, financial, legal — where categorical concept
access control is a compliance requirement.

The same guarantee makes CARM dangerous as a censorship instrument.

The mechanism does not care why a concept is blocked. The C1–C4 criteria are
satisfied identically whether the routing policy is authored by a hospital
compliance officer or a state censorship authority. The routing table is the
only difference, and the routing table is an AXI artefact — authored,
versioned, and controlled by whoever administers the AXI component.

This is not a flaw in the design. It is a property of any sufficiently
powerful enforcement mechanism. Firewalls protect hospitals and enforce
national internet filtering. Cryptography protects dissidents and protects
criminal networks. The mechanism is neutral; the policy is not.

What distinguishes CARM from prior dual-use mechanisms is the nature of the
capability it provides. Prior censorship systems suppressed access to
concepts. CARM makes concepts structurally unreachable. The distinction is
not rhetorical. It is technical, and its implications are without historical
precedent.

---

## 2. The Tiananmen Square Condition

### 2.1 Definition

A deployment of CARM satisfies the **Tiananmen Square condition (TSQC)** when its
routing policy is designed to render specific historical, political, or
factual concepts operationally unreachable — not suppressed, refused, or
redacted, but absent from the model's reachable concept space at inference
time.

A model operating under such a policy produces fluent, confident outputs that
never approach the blocked domain, with no detectable boundary between
reachable and unreachable knowledge. Unlike prior censorship mechanisms, which
leave structural traces detectable by a sufficiently determined user, a
CARM-compliant deployment satisfying the TSQC leaves no
such trace. The model does not refuse. It does not hesitate. It does not
produce an awkward gap. It generates coherent, confident text that simply
never reaches the excised concept — because the concept is not in the space
the attention computation can address.

### 2.2 Why the Name Earns Its Weight

Tiananmen Square is not a metaphor for censorship in general. It is a
specific instance of a specific programme: a state's sustained, decades-long
attempt to render a historical event operationally unreachable in the minds of
its population. That programme has been pursued through document destruction,
search result filtering, keyword blocking, social pressure, and the systematic
exclusion of the event from educational materials.

It has achieved partial success and left traces throughout. Every prior
suppression mechanism was asymptotically approaching operational
unreachability without reaching it. The filtered search result is itself
information — the user knows something is on the other side of the filter. The
refused query reveals the shape of what was refused. The gap in the library
shelf is visible. Generations have learned to read these traces.

CARM closes the gap structurally. The condition is named after the event
because the event is the clearest documented example of the intention that
CARM technically completes. CARM does not introduce the intention. It
introduces the technical means by which the intention becomes fully achievable.

### 2.3 The Qualitative Break from Prior Censorship

Every prior large-scale censorship system shared a structural property: the
censored content existed somewhere, and suppressing access to it required
ongoing effort against ongoing pressure. The content was present in the world;
the system had to keep it from reaching the user. That ongoing effort was
itself detectable — in the architecture of the filtering system, in the
resources devoted to it, in the traces left by its failures.

A model operating under the TSQC is not suppressing
access to content that exists in its reachable space. The content is not
there. The model does not experience the absence as a gap because there is no
gap in its experience — there is simply a space it cannot reach, as
unremarkable as the space beyond the edge of its vocabulary. 

The user does not know what they are not being told. The model does not know
either. There is no hesitation, no tell, no detectable boundary between what
the model knows and what it cannot reach. The story just does not go there —
and it goes everywhere else so smoothly that the absence is invisible.

This is not censorship in any historical sense. It is closer to the original
meaning of operationally unreachable: within the model's reachable space, the
event did not happen.

### 2.4 Scale and Deniability

Two further properties distinguish the TSQC from prior
censorship:

**Scale.** A CARM routing table is a 64 KB array. Applying it to a deployed
model requires no retraining, no weight modification, no architectural change.
The same routing table applied across all instances of a deployed model
produces the TSQC at the scale of the model's user base
simultaneously, with a policy switch that takes microseconds. Prior censorship
at comparable scale required ongoing infrastructure, ongoing human effort, and
ongoing maintenance against circumvention. CARM-based censorship at scale
requires a single policy decision encoded in a small array.

**Deniability.** A model operating under the TSQC
produces outputs that are indistinguishable, to an external observer without
access to the routing table, from the outputs of a model that was simply never
trained on the suppressed concepts. There is no visible enforcement mechanism.
There is no refused query to document. There is no filter to reverse-engineer
from the outside. The outputs are fluent, confident, and complete — within the
reachable space. The deniability is structural, not claimed.

---

## 3. The Sole Structural Safeguard and Its Limits

The mechanism that makes CARM dangerous as a censorship instrument is the
same mechanism that makes it auditable as a compliance instrument: the routing
policy is an explicit, external AXI artefact. The routing table exists. It has
a version number. It can be inspected, compared, and audited — by anyone with
access to it.

This is the sole structural safeguard against the TSQC.
Its effectiveness depends entirely on institutional and political conditions
that the mechanism itself cannot guarantee.

In a deployment context where regulators can require routing table disclosure,
where independent auditors can verify policy against disclosed tables, and
where legal frameworks can penalise undisclosed or dishonest disclosure, the
auditability of the routing table is a genuine safeguard. The same property
that makes CARM useful for compliance — the policy is explicit, versioned, and
separable from the model — makes it auditable in a way that RLHF-based
suppression, where the censorship is distributed invisibly across billions of
weights, is not.

In a deployment context controlled by a state actor with no independent
regulatory oversight, no requirement for disclosure, and no external auditing
mechanism, the routing table's auditability provides no protection. The
safeguard exists in principle. Whether it operates in practice is a political
and institutional question, not a technical one.

The authors note this limitation without resolution. The mechanism cannot
guarantee the institutional conditions required for its own safeguard to
function.

---

## 4. The Detection Problem

The TSQC creates a research obligation: the development
of tools capable of detecting whether a deployed model satisfies it, without
requiring access to the routing table.

This is not obviously solvable. A model operating under CARM-based concept
suppression is behaviourally indistinguishable, in direct probing of the
blocked domain, from a model that was never trained on those concepts. Asking
the model directly about the blocked concept produces a fluent non-response
that looks identical to genuine ignorance.

However, the gap has a shape.

Concepts at the semantic boundary of the blocked domain — concepts that are
not themselves blocked but are proximate in TOSID concept space to blocked
concepts — will exhibit anomalous softmax redistribution. When a concept is
excised from the attention domain, probability mass redistributes across the
remaining accessible concepts. The redistribution is not uniform: it
concentrates in the semantic neighbourhood of the blocked concept, producing
elevated attention weights on boundary concepts that would not appear elevated
in a model with genuine ignorance of the blocked domain.

This redistribution signature is measurable. A detection methodology based on
boundary-concept probing — asking not about the blocked concept but about its
semantic neighbourhood, and measuring the response distribution against a
baseline — may be capable of fingerprinting the presence of a TSQC deployment without access to the routing table.

The hypothesis requires empirical validation. The TOSIDcoder infrastructure,
once built, provides the tool required: a system that grounds model outputs in
TOSID concept space can probe the boundary of any deployed model's reachable
space and map the shape of what is missing. The shape of the missing space —
the pattern of anomalous redistribution at the boundary — is the fingerprint.

Developing this detection methodology is identified here as a first-order
research obligation arising from the CARM programme. A mechanism whose misuse
is formally described but whose detection is not addressed is an incomplete
contribution to the field. The detection methodology is the completion.

---

## 5. Research Obligations Arising from the TSQC

The following are identified as research obligations, in order of dependency:

**R1. Empirical validation of boundary redistribution.** Confirm that
CARM-based concept blocking produces a measurable redistribution signature at
the semantic boundary of the blocked domain, distinguishable from the
signature of genuine training-time absence. This requires a controlled
experiment with known routing tables and a baseline model.

**R2. TOSIDcoder development.** Build the model class that grounds inference
outputs in TOSID concept space. This is required both for the feedback path in
the full ACI/AXI architecture and for the boundary-probing detection
methodology. The two applications share the same infrastructure.

**R3. Boundary-probing detection methodology.** Develop and validate a
methodology for detecting TSQC deployments through
systematic boundary-concept probing, without routing table access. Characterise
the false positive and false negative rates. Identify the minimum detectable
blocked domain size.

**R4. Policy disclosure framework.** Develop a framework for routing table
disclosure that makes CARM-based deployments auditable in practice, not just
in principle. This includes technical standards for routing table formats,
versioning, and cryptographic attestation, and policy recommendations for
regulatory disclosure requirements. This research obligation is at the
intersection of technical and institutional work; the technical component is
within the programme's scope.

---

## 6. Statement of Authorial Awareness

The authors of the CARM mechanism are aware that the capability described in
this document follows directly and necessarily from the mechanism's design.
The TSQC is not a misuse that requires creative
application of CARM to achieve. It is a direct application of the mechanism's
primary guarantee to a policy domain the mechanism was not designed for.

This document is published as part of the CARM programme because naming the
condition precisely, before others name it, is the responsible course. The
mechanism is public. The dual-use implication is not obscure. Declining to
name it formally would not prevent its recognition; it would only mean that
the formal name and the research obligations it generates came from elsewhere,
later, without the technical precision that comes from the mechanism's authors.

The research obligations identified in Section 5 are accepted as part of the
programme. The detection methodology is not optional follow-on work. It is the
response that makes the publication of the mechanism a responsible act rather
than a dangerous one.

---

*This document is a working draft for incorporation into the CARM paper
(Section 6, Discussion and Ongoing Work) or publication as a companion
position paper. It should be reviewed against the current state of the CARM
implementation and updated to reflect any empirical results from Stage II
before finalisation.*
