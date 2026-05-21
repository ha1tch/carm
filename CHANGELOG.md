# Changelog

All notable changes to this project will be documented here.

## [0.2.15] — 2026-05-20

### Added
- `PROGRAMME.md` v1.4: new section documenting the June 2025 Prolog
  proof-of-concept as the ground-truth semantic oracle for the programme;
  five patient cases formalised as automated regression test targets with
  expected treatment paths; TOSID 5-tuple taxonomy codes documented with
  real examples; three-tier KMAC architecture traced to its empirical
  origin in Prolog execution patterns; automation plan for Stage II
  regression testing specified

---

## [0.2.14] — 2026-05-20

### Added
- `make show` target: opens `paper/carm.pdf` using the system PDF viewer
  (`open` on macOS, `xdg-open` on Linux, `explorer.exe` on Windows);
  prints a helpful error if the PDF has not been built yet

### Changed
- `make clean` now also removes pdflatex residual files:
  `paper/carm.aux`, `paper/carm.log`, `paper/carm.out`, `paper/carm.toc`

---

## [0.2.13] — 2026-05-20

### Fixed
- `PROGRAMME.md`: removed invented `github.com/ha1tch/kmac` URL — the KMAC
  repository does not yet exist; replaced with accurate description
  (specification documented, repository not yet created)

---

## [0.2.12] — 2026-05-20

### Changed
- `PROGRAMME.md` v1.3: KMAC named as AXI compilation target throughout;
  overview expanded to name three sibling programmes (TOSID, KMAC, PTAC);
  Stage I key design decisions updated; Stage II Track A adds KMAC compiler
  sub-task; Relationship section expanded with October 2025 KMAC specification
  details (13 primitives, three-tier architecture, VHDL transpilation path,
  authority/provenance fields)
- `HORIZONS.md` v1.1: KMAC 13-primitive semitive lattice added as mathematical
  foundation for semantic distance formalisation; Horizons Stage II updated;
  KMAC authority/provenance fields connected to item 5 (taxonomy versioning);
  prerequisite 5 clarified to include KMAC semitive stabilisation

---

## [0.2.11] — 2026-05-20

### Added
- `HORIZONS.md` — post-programme theoretical territory document covering
  semantic traversability, bounded epistemic topology, semantic horizon
  computation, topology engineering, and the Horizons programme structure
- `PROGRAMME.md` v1.2: five Horizons-readiness adjustments added covering
  semantic distance instrumentation, TOSIDcoder distance estimation,
  activation trajectory preservation, AXI semantic distance API, and
  TOSID taxonomy versioning for drift analysis
- Horizons programme added to relationship section of PROGRAMME.md

---

## [0.2.10] — 2026-05-20

### Fixed
- README embeddings section corrected: pipeline runs through
  `build_vocabulary.py` (which writes the .bin), not `load_glove.py`
  (which is an exploration/testing script only). All three output files
  documented (.bin, .txt, _metadata.json). Pipeline is complete end to end.

---

## [0.2.9] — 2026-05-20

### Fixed
- `embeddings/` directory now exists in repo (with `.gitkeep`); previously
  the README referenced it but it did not exist
- README embeddings section rewritten: clearly marked as optional, removed
  misleading C snippet implying GloVe is wired into the demo, updated
  phase reference to Phase 1.2

---

## [0.2.8] — 2026-05-20

### Changed
- Paper: table of contents rendered at \small (10% smaller) scoped to that
  section only

---

## [0.2.7] — 2026-05-20

### Added
- `make paper` target: builds `paper/carm.pdf` via two pdflatex passes
  (requires TeX Live / pdflatex)

---

## [0.2.6] — 2026-05-20

### Added
- `make help` target (also the default when running `make` with no arguments)
  showing all targets with descriptions and quick-start examples

---

## [0.2.5] — 2026-05-20

### Changed
- Paper: page breaks added before Contents, Introduction, and References

---

## [0.2.4] — 2026-05-20

### Changed
- Phase numbering aligned with programme: `Phase 04.01` → `Phase 1.1`,
  `Phase 04.02` → `Phase 1.2` throughout paper, benchmarks, tools,
  vocabulary file, and PROGRAMME.md
- All `src/`, `benchmarks/routing_bench.c`, and `tools/` files now carry
  consistent copyright and description headers (Apache 2.0, haitch 2026)
- README status table updated: additive masking variant, MRE, Phase 3
  and Phase 4 entries added
- `.gitignore` clarified: release zips annotated as belonging in GitHub
  Releases, not committed to tree

---

## [0.2.3] — 2026-05-20

### Changed
- Author email updated to `h@ual.li`
- Mastodon address removed from paper author block

---

## [0.2.2] — 2026-05-20

### Added
- `benchmarks/mre_additive.c` — micro-universe MRE using additive masking
  variant (Stage II, Track A). 4 concepts, 4 dimensions, identity weights,
  adversarial query. Verifies both scenarios against analytic expected values.
  Corrects reviewer's Scenario A output: scaled dot product gives
  [0.007, 0.980, 0.007, 0.007], not [0, 1, 0, 0].
- `make mre` target added to Makefile
- `PROGRAMME.md` v1.1: micro-universe canonical parameters formalised in
  Stage II Track A deliverables; maximum-affinity and three additional
  adversarial query types added to Track B

---

## [0.2.1] — 2026-05-20

### Added
- `PROGRAMME.md` — integrated four-stage research programme document
- Expanded §6 in paper: identifier vs semantic isolation, TOSIDcoders,
  full programme roadmap with Stage I–IV descriptions, RAG analogy,
  feedback path analysis
- `\bibitem{Lewis2020}` RAG reference added to bibliography

---

## [0.2.0] — 2026-05-20

### Added
- Phase 2: GloVe embedding loader (`src/embeddings.h`, `src/embeddings.c`)
- O(1) routing array (64 KB, fits L2 cache) replacing linear rule scan
- 1,000-concept vocabulary with TOSID assignments (`examples/vocabulary_1000.txt`)
- Vocabulary and embedding build tools (`tools/build_vocabulary.py`,
  `tools/load_glove.py`)
- Comprehensive benchmark suite covering B1–B7 (`benchmarks/carm_benchmark.c`):
  routing overhead vs concept count, zero-leakage verification (5,000
  measurements), output divergence vs blocking ratio, latency distribution
  (20,000 samples), overhead vs allowed/blocked ratio, O(1) vs linear
  latency comparison
- Routing microbenchmark (`benchmarks/routing_bench.c`)
- Paper: *CARM: Controlled Attention Routing and Masking* (`paper/carm.pdf`)

### Changed
- `TOSID_GET_DOMAIN` macro conflict resolved: `multihead_attention.h` now
  uses `TOSID_GET_DOMAIN_CATEGORY` (bits 23–16) to avoid collision with
  `routing_table.h`'s authoritative definition (bits 31–24)
- All `strncpy` calls replaced with `memcpy` + explicit null termination

### Fixed
- Test 6e: updated to use `TOSID_GET_DOMAIN_CATEGORY` after macro rename

---

## [0.1.0] — 2026-03-01

### Added
- Initial proof-of-concept: multi-head attention (8 heads, 64-dim) with
  CARM routing enforcement
- Synthetic clustered embeddings (medical, financial, PHI domains)
- Six-test suite verifying: basic functionality, consistency across 800
  attention computations (0 violations), head specialisation, information
  leakage isolation, performance overhead (<5%), edge cases
- Clinical context routing policy: medical domain allowed, financial and
  PHI domains blocked
- `src/routing_table.h` / `src/routing_table.c`: routing rule engine
- `src/multihead_attention.h` / `src/multihead_attention.c`: three-phase
  CARM computation (routing → attention over accessible set → output)
- `src/multihead_attention_test.c`: test suite
- `src/multihead_demo.c`: interactive demonstration
