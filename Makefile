CXX ?= g++
CXXFLAGS = -O3 -std=c++17 -Wall

ifeq ($(CXX),icpc)
CXXFLAGS += -xHost #-no-vec
CXXFLAGS += -qopt-report=5
CXXFLAGS += -D__ALIGNMENT=32
endif

ifeq ($(CXX),g++)
CXXFLAGS += -mtune=native
#CXXFLAGS += -march=skylake-avx512
endif

EXEC = sir_sim
SRC = main_hpc.cpp
OBJ = main_hpc.o

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(OBJ)

clean:
	/bin/rm -fv $(EXEC) $(OBJ) *.optrpt
