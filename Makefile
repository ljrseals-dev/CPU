# Neuro-Silica build entry points
# Author: Lewis James Seals Jr. (ljrseals-dev)
# Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
CXX ?= clang++
WASM_CXX ?= clang++
WASM_LD ?= wasm-ld
NATIVE_FLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Isrc
WASM_FLAGS ?= --target=wasm32 -std=c++20 -O2 -fno-exceptions -fno-rtti -nostdlib -Isrc -Wall -Wextra
WASM_LDFLAGS ?= --no-entry --export-memory --export=init_cpu --export=step --export=set_reg --export=get_reg --export=get_fault --export=memory_base --export=get_yielded --export=clear_yielded --export=reset_cpu --allow-undefined --initial-memory=33554432 --max-memory=33554432
SILICA_ALLAH_DIR ?= ../Silica-Allah
.PHONY: all native wasm test differential mutation san clean
all: native wasm
build:
	mkdir -p build
build/native_core_test: tests/native_core_test.cpp src/silica/cpu.cpp src/silica/cpu.hpp | build
	$(CXX) $(NATIVE_FLAGS) tests/native_core_test.cpp src/silica/cpu.cpp -o $@
build/cpu.o: src/silica/cpu.cpp src/silica/cpu.hpp | build
	$(WASM_CXX) $(WASM_FLAGS) -c $< -o $@
build/silica_wasm.o: src/silica_wasm.cpp src/silica/cpu.hpp | build
	$(WASM_CXX) $(WASM_FLAGS) -c $< -o $@
build/silica-core.wasm: build/cpu.o build/silica_wasm.o
	$(WASM_LD) $(WASM_LDFLAGS) $^ -o $@
native: build/native_core_test

build/native_core_test_san: tests/native_core_test.cpp src/silica/cpu.cpp src/silica/cpu.hpp | build
	$(CXX) $(NATIVE_FLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tests/native_core_test.cpp src/silica/cpu.cpp -o $@

san: build/native_core_test_san
	./build/native_core_test_san

build/differential_probe_reference: tests/differential/probe.cpp $(wildcard $(SILICA_ALLAH_DIR)/include/silica/*.hpp) | build
	@test -f "$(SILICA_ALLAH_DIR)/include/silica/cpu.hpp" || { echo "Silica-Allah headers not found; set SILICA_ALLAH_DIR to its checkout" >&2; exit 2; }
	mkdir -p build/differential
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DSILICA_REFERENCE -I$(SILICA_ALLAH_DIR)/include tests/differential/probe.cpp -o $@

build/differential_probe_neuro: tests/differential/probe.cpp src/silica/cpu.cpp src/silica/cpu.hpp | build
	mkdir -p build/differential
	$(CXX) $(NATIVE_FLAGS) -DSILICA_NEURO tests/differential/probe.cpp src/silica/cpu.cpp -o $@

differential: build/differential_probe_reference build/differential_probe_neuro
	./build/differential_probe_reference > build/differential/reference.trace
	./build/differential_probe_neuro > build/differential/neuro.trace
	diff -u build/differential/reference.trace build/differential/neuro.trace
	@echo "Differential state traces match for the declared common subset."

mutation: native
	./build/native_core_test
	CXX="$(CXX)" sh tests/mutation/bl_link_mutant.sh
	CXX="$(CXX)" sh tests/mutation/madd_decode_mutant.sh

wasm: build/silica-core.wasm
test: native wasm
	./build/native_core_test
	node tests/memt1_fallback.test.js
	node tests/critic_harness.test.mjs
	$(MAKE) mutation CXX="$(CXX)"
clean:
	rm -rf build
