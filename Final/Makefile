RM := rm -f
CC := gcc
CFLAGS := -std=c99 -O0 -g
LD_FLAGS := -pthread -lm
LD_FLAGS_ocl := -lOpenCL
PROF_FLAGS := -pg -ggdb
OMP_FLAGS := -fopenmp -Wall

CXXFLAGS := -I./src -MMD -MP -std=gnu++0x -g -Wall -Wno-sign-compare
CXX      := g++

CXXFLAGS_ocl := -DUSE_OPENCL

all: q3 q4 q5 q6

q3: bin bin/edge_detection

q4: bin bin/zeta bin/zeta_omp

q5: bin bin/solver bin/solver_fast

q6: bin bin/election

bin/edge_detection: q3/canny_edge.c q3/hysteresis.c q3/pgm_io.c
	@printf "Compiling Q3: Edge Detection\n"
	$(CC) $(CFLAGS) $(PROF_FLAGS) -o $@  q3/hysteresis.c q3/canny_edge.c q3/pgm_io.c $(LD_FLAGS)
	
bin/zeta: q4/zeta.c
	@printf "Compiling Q4: Zeta\n"
	$(CC) $< $(CFLAGS) -o $@ $(LD_FLAGS)

bin/zeta_omp: q4/zeta_omp.c
	@printf "Compiling Q4: Zeta OpenMP\n"
	$(CC) $< $(CFLAGS) $(OMP_FLAGS) -o $@ $(LD_FLAGS)

bin/solver: q5/solver.cpp q5/gatsp.cpp
	@printf "Compiling Q5: Route Solver Normal\n"
	$(CXX) $(CXXFLAGS) -o $@ q5/solver.cpp q5/gatsp.cpp
	
bin/solver_fast: q5/solver.cpp q5/gatsp_fast.cpp
	@printf "Compiling Q5: Route Solver Fast\n"
	$(CXX) $(CXXFLAGS) $(OMP_FLAGS) -o $@ q5/solver.cpp q5/gatsp_fast.cpp

bin/election: q6/election.cpp
	@printf "Compiling Q6: Election\n"
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_ocl) -o $@ q6/election.cpp $(LD_FLAGS) $(LD_FLAGS_ocl)


bin:
	mkdir -p bin

clean:
	$(RM) -r bin


.PHONY: all q3 q4 q5 q6