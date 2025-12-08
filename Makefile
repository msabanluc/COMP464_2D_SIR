CXX ?= g++
MPICXX ?= mpicxx

# Common flags
CXXFLAGS_COMMON = -O3 -std=c++17 -Wall

# Intel specific flags
CXXFLAGS_ICPC = -xHost -qopt-report=5 -D__ALIGNMENT=32
OMP_FLAG_ICPC = -qopenmp

# GCC specific flags
CXXFLAGS_GCC = -mtune=native
OMP_FLAG_GCC = -fopenmp

# Configure CXX flags
ifeq ($(CXX),icpc)
	CXXFLAGS_HOST = $(CXXFLAGS_COMMON) $(CXXFLAGS_ICPC)
	OMP_FLAG = $(OMP_FLAG_ICPC)
else
	CXXFLAGS_HOST = $(CXXFLAGS_COMMON) $(CXXFLAGS_GCC)
	OMP_FLAG = $(OMP_FLAG_GCC)
endif

# Configure MPI flags
# Check if MPICXX is mpiicpc (Intel)
ifneq (,$(findstring mpiicpc,$(MPICXX)))
	CXXFLAGS_MPI_BUILD = $(CXXFLAGS_COMMON) $(CXXFLAGS_ICPC)
else
	# Assume GCC for mpicxx
	CXXFLAGS_MPI_BUILD = $(CXXFLAGS_COMMON) $(CXXFLAGS_GCC)
endif

EXEC_SERIAL = sir_sim
EXEC_OMP = sir_sim_omp
EXEC_MPI = sir_sim_mpi
EXEC_MPI_NO_BUFF = sir_sim_mpi_nobuff

SRC_SERIAL = main_hpc_serial.cpp
SRC_OMP = main_hpc_openmp.cpp
SRC_MPI = main_hpc_mpi.cpp
SRC_MPI_NO_BUFF = main_hpc_mpi_nb

OBJ_SERIAL = $(SRC_SERIAL:.cpp=.o)
OBJ_OMP = $(SRC_OMP:.cpp=.o)
OBJ_MPI = $(SRC_MPI:.cpp=.o)
OBJ_MPI_NO_BUFF = $(SRC_MPI_NO_BUFF:.cpp=.o)

all: $(EXEC_SERIAL) $(EXEC_OMP) $(EXEC_MPI) $(EXEC_MPI_NO_BUFF)

# Compile Serial Object
$(OBJ_SERIAL): $(SRC_SERIAL)
	$(CXX) $(CXXFLAGS_HOST) -c $< -o $@

# Compile OpenMP Object (needs OMP flag)
$(OBJ_OMP): $(SRC_OMP)
	$(CXX) $(CXXFLAGS_HOST) $(OMP_FLAG) -c $< -o $@

# Compile MPI Object (uses MPI flags)
$(OBJ_MPI): $(SRC_MPI)
	$(MPICXX) $(CXXFLAGS_MPI_BUILD) -c $< -o $@

# Compile MPI NO Buffer (uses MPI flags)
$(OBJ_MPI_NO_BUFF): $(SRC_MPI_NO_BUFF)
	$(MPICXX) $(CXXFLAGS_MPI_BUILD) -c $< -o $@

# Link Serial Executable
$(EXEC_SERIAL): $(OBJ_SERIAL)
	$(CXX) $(CXXFLAGS_HOST) -o $(EXEC_SERIAL) $(OBJ_SERIAL)

# Link OpenMP Executable
$(EXEC_OMP): $(OBJ_OMP)
	$(CXX) $(CXXFLAGS_HOST) $(OMP_FLAG) -o $(EXEC_OMP) $(OBJ_OMP)

# Link MPI Executable
$(EXEC_MPI): $(OBJ_MPI)
	$(MPICXX) $(CXXFLAGS_MPI_BUILD) -o $(EXEC_MPI) $(OBJ_MPI)

# Link MPI NO BUFFER Executable
$(EXEC_MPI_NO_BUFF): $(OBJ_MPI_NO_BUFF))
	$(MPICXX) $(CXXFLAGS_MPI_BUILD) -o $(EXEC_MPI_NO_BUFF) $(OBJ_MPI_NO_BUFF)

clean:
	/bin/rm -fv $(EXEC_SERIAL) $(EXEC_OMP) $(EXEC_MPI) $(EXEC_MPI_NO_BUFF) $(OBJ_SERIAL) $(OBJ_OMP) $(OBJ_MPI) $(OBJ_MPI_NO_BUFF) *.optrpt