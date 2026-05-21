## CARM - Controlled Attention Routing and Masking
## Root Makefile
##
## Targets:
##   all       - build core library, tests, demo, and benchmark
##   test      - build and run test suite (6 tests)
##   demo      - build and run interactive demo
##   bench     - build and run comprehensive benchmark
##   version   - print current version
##   clean     - remove build artefacts

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11 -march=native
LDFLAGS = -lm

## ── Version ─────────────────────────────────────────────────────────────
VERSION  := $(shell cat VERSION)
CFLAGS   += -DCARM_VERSION=\"$(VERSION)\"

## ── Sources ─────────────────────────────────────────────────────────────
CORE_SRC = src/routing_table.c \
           src/multihead_attention.c \
           src/carm_version.c

HEADERS  = src/routing_table.h \
           src/multihead_attention.h \
           src/carm_version.h

## ── Binaries ────────────────────────────────────────────────────────────
TEST_BIN  = build/carm_test
DEMO_BIN  = build/carm_demo
BENCH_BIN = build/carm_benchmark
MRE_BIN   = build/mre_additive

.PHONY: all test demo bench mre paper version clean help

## ── Default target ─────────────────────────────────────────────────────
.DEFAULT_GOAL := help

help:
	@echo ""
	@echo "CARM v$(VERSION) — Controlled Attention Routing and Masking"
	@echo ""
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@echo "  all       Build all binaries (test, demo, benchmark, mre)"
	@echo "  test      Build and run the test suite (6 tests)"
	@echo "  demo      Build and run the interactive demo"
	@echo "  bench     Build and run the comprehensive benchmark suite (B1-B7)"
	@echo "  mre       Build and run the micro-universe MRE (additive masking)"
	@echo "  paper     Build the PDF paper (requires pdflatex)"
	@echo "  version   Print current version"
	@echo "  clean     Remove build artefacts"
	@echo "  help      Show this message"
	@echo ""
	@echo "Quick start:"
	@echo "  make test          # verify everything works"
	@echo "  make mre           # see CARM in action (hand-verifiable example)"
	@echo "  make bench > r.csv # run benchmarks and save results"
	@echo "  make paper         # build paper/carm.pdf"
	@echo ""

all: $(TEST_BIN) $(DEMO_BIN) $(BENCH_BIN) $(MRE_BIN)

$(TEST_BIN): $(CORE_SRC) src/multihead_attention_test.c $(HEADERS) | build
	$(CC) $(CFLAGS) -o $@ $(CORE_SRC) src/multihead_attention_test.c $(LDFLAGS)

$(DEMO_BIN): $(CORE_SRC) src/multihead_demo.c $(HEADERS) | build
	$(CC) $(CFLAGS) -o $@ $(CORE_SRC) src/multihead_demo.c $(LDFLAGS)

$(BENCH_BIN): benchmarks/carm_benchmark.c | build
	$(CC) $(CFLAGS) -o $@ benchmarks/carm_benchmark.c $(LDFLAGS)

$(MRE_BIN): benchmarks/mre_additive.c | build
	$(CC) $(CFLAGS) -o $@ benchmarks/mre_additive.c $(LDFLAGS)

build:
	mkdir -p build

test: $(TEST_BIN)
	@echo ""
	@echo "=============================================="
	@echo "CARM Test Suite v$(VERSION)"
	@echo "=============================================="
	@echo ""
	./$(TEST_BIN)

demo: $(DEMO_BIN)
	@echo ""
	@echo "=============================================="
	@echo "CARM Interactive Demo v$(VERSION)"
	@echo "=============================================="
	@echo ""
	./$(DEMO_BIN)

bench: $(BENCH_BIN)
	@echo ""
	@echo "=============================================="
	@echo "CARM Comprehensive Benchmark v$(VERSION)"
	@echo "=============================================="
	@echo ""
	./$(BENCH_BIN)

mre: $(MRE_BIN)
	@echo ""
	@echo "=============================================="
	@echo "CARM Micro-Universe MRE v$(VERSION)"
	@echo "=============================================="
	@echo ""
	./$(MRE_BIN)

paper:
	@echo ""
	@echo "Building paper/carm.pdf..."
	@cd paper && pdflatex -interaction=nonstopmode carm.tex > /dev/null
	@cd paper && pdflatex -interaction=nonstopmode carm.tex > /dev/null
	@echo "Done: paper/carm.pdf"

version:
	@echo "CARM v$(VERSION)"

clean:
	@rm -rf build/
	@rm paper/*.aux
	@rm paper/*.out
	@rm paper/*.toc
	@rm paper/*.log
	@echo "Cleaned."

show:
	@open paper/carm.pdf
