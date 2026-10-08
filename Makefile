CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wpedantic
INCLUDE  := -Iinclude -Itests

SANFLAGS := -fsanitize=address -fno-omit-frame-pointer -g

TEST  := build/backend_consistency_test
BENCH := $(patsubst tests/benchmarks/%.cpp,build/%,$(wildcard tests/benchmarks/bench_*.cpp))
BINS  := $(TEST) $(BENCH)

.PHONY: all test asan bench clean
all: $(BINS)

$(TEST): tests/unit/backend_consistency_test.cpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP $< -o $@

$(TEST)_asan: tests/unit/backend_consistency_test.cpp | build
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(INCLUDE) -MMD -MP $< -o $@

build/bench_%: tests/benchmarks/bench_%.cpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP $< -o $@

test: $(TEST)
	./$(TEST)

asan: $(TEST)_asan
	./$(TEST)_asan

bench: $(BENCH)
	./tests/benchmarks/scripts/run.sh

build:
	mkdir -p build

clean:
	rm -rf build

# Rebuild when an included header changes
-include $(wildcard build/*.d)
