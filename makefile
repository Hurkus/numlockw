SHELL := /usr/bin/env bash
config = release
# config = debug


C++ = g++
STDC++ = -std=c++23


ifeq ($(config),debug)
	OPTIMIZE := -O0
	GDB := -g
	DEFINES += -D'DEBUG=1'
else
	OPTIMIZE := -O2
	GDB :=
	DEFINES += -D'DEBUG=0' -D'NDEBUG'
endif


W_FMT = -Wformat=1 -Wformat-contains-nul -Wformat-diag -Wformat-extra-args \
		-Wformat-overflow=1 -Wformat-truncation=1 -Wformat-zero-length
WARN := $(W_FMT) -Wno-error


INCLUDES += -I 'src'


# Name of program.
EXE=numlockw


################################################################


.PHONY: all
all: bin/$(EXE)

.PHONY: clean
clean:
	rm -rf 'bin'
	@echo "Project cleaned."


################################################################


bin/:
	mkdir -p "$@"

bin/$(EXE): $(wildcard src/*) | bin/
	$(CXX) $(filter %.cpp,$^) $(INCLUDES) $(OPTIMIZE) $(GDB) $(DEFINES) $(WARN) -o "$@"


################################################################
