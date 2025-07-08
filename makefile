SHELL := /usr/bin/env bash
config = release

CXX      = g++
CXXFLAGS = -O2 -Wall $(W_FMT) -DNDEBUG -std=c++2a

ifeq ($(config),debug)
	CXXFLAGS = -O0 $(W_FMT) -Wswitch -g -std=c++2a
endif


src = src
obj = obj
bin = bin
program = numlockw

INCLUDES = -I$(src)
LIB_INC  = 
LIB_LNK  = 


# W_FMT = -Wformat=1 -Wformat-contains-nul -Wformat-diag -Wformat-extra-args \
# 		-Wformat-overflow=1 -Wformat-truncation=1 -Wformat-zero-length


################################################################


.PHONY: all
all: $(bin)/$(program)

# Run program as test
.PHONY: run
run: ./$(bin)/$(program)
	./$(bin)/$(program) toggle

.PHONY: clean
clean:
	rm -rf '$(obj)' '$(bin)'
	@echo "Project cleaned."


################################################################


$(obj)/InputDevice.o:
$(obj)/main.o: 

$(bin) $(obj):
	mkdir -p "$@"

$(bin)/$(program): $(wildcard $(src)/* ) | $(bin)
	$(CXX) $(filter %.cpp, $^) $(LIB_INC) $(LIB_LNK) $(CXXFLAGS) -o "$@"
