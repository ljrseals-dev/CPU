# Neuro-Silica build entry points
# Author: Lewis James Seals Jr. (ljrseals-dev)
# Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
CXX ?= clang++
WASM_CXX ?= clang++
WASM_LD ?= wasm-ld
NATIVE_FLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Isrc
WASM_FLAGS ?= --target=wasm32 -std=c++20 -O2 -fno-exceptions -fno-rtti -nostdlib -fno-builtin -Isrc -Wall -Wextra
WASM_LDFLAGS ?= --no-entry --export-dynamic --allow-undefined --initial-memory=33554432 --max-memory=33554432
.PHONY: all native wasm test clean
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
wasm: build/silica-core.wasm
test: native wasm
	./build/native_core_test
	node tests/memt1_fallback.test.js
	node tests/critic_harness.test.mjs
clean:
	rm -rf build
