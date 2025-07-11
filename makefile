SHELL := /usr/bin/env bash
config = release

CXX      = g++
CXXFLAGS = -O2 -Wall -DNDEBUG -std=c++2a

ifeq ($(config),debug)
	CXXFLAGS = -O0 $(W_FMT) -Wswitch -g -std=c++2a
endif

program = numlockw
INCLUDES = -I'src'

W_FMT = -Wformat=1 -Wformat-contains-nul -Wformat-diag -Wformat-extra-args \
		-Wformat-overflow=1 -Wformat-truncation=1 -Wformat-zero-length


################################################################


.PHONY: all
all: bin/$(program)

.PHONY: clean
clean:
	rm -rf 'bin'
	@echo "Project cleaned."


################################################################


bin:
	mkdir -p "$@"

bin/$(program): $(wildcard src/* ) | bin
	$(CXX) $(filter %.cpp, $^) $(INCLUDES) $(CXXFLAGS) -o "$@"
