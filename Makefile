CXX ?= g++
MPICXX ?= mpicxx

CXXFLAGS_COMMON = -O3 -std=c++17 -Wall

ifeq ($(CXX),icpc)
CXXFLAGS_COMMON += -xHost #-no-vec
CXXFLAGS_COMMON += -qopt-report=5
CXXFLAGS_COMMON += -D__ALIGNMENT=32
endif

ifeq ($(CXX),g++)
CXXFLAGS_COMMON += -mtune=native
#CXXFLAGS_COMMON += -march=skylake-avx512
endif

CXXFLAGS_OMP = $(CXXFLAGS_COMMON) -fopenmp

EXEC_SERIAL = sir_sim
EXEC_OMP = sir_sim_omp
EXEC_MPI = sir_sim_mpi

SRC_SERIAL = main_hpc_serial.cpp
SRC_OMP = main_hpc_openmp.cpp
SRC_MPI = main_hpc_mpi.cpp

OBJ_SERIAL = $(SRC_SERIAL:.cpp=.o)
OBJ_OMP = $(SRC_OMP:.cpp=.o)
OBJ_MPI = $(SRC_MPI:.cpp=.o)

all: $(EXEC_SERIAL) $(EXEC_OMP) $(EXEC_MPI)

$(EXEC_SERIAL): $(OBJ_SERIAL)
	$(CXX) $(CXXFLAGS_COMMON) -o $(EXEC_SERIAL) $(OBJ_SERIAL)

$(EXEC_OMP): $(OBJ_OMP)
	$(CXX) $(CXXFLAGS_OMP) -o $(EXEC_OMP) $(OBJ_OMP)

$(EXEC_MPI): $(OBJ_MPI)
	$(MPICXX) $(CXXFLAGS_COMMON) -o $(EXEC_MPI) $(OBJ_MPI)

clean:
	/bin/rm -fv $(EXEC_SERIAL) $(EXEC_OMP) $(EXEC_MPI) $(OBJ_SERIAL) $(OBJ_OMP) $(OBJ_MPI) *.optrpt