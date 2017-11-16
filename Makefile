CC = mpicc
CXX = mpicxx
LDFLAGS = -lpthread -lm
CFLAGS = -O3 -std=gnu99
CXXFLAGS = -O3 -std=gnu++11
TARGETS = APSP_Pthread APSP_MPI_sync APSP_MPI_async APSP_Hybrid

.PHONY: all
all: $(TARGETS)


%: %.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)


.PHONY: clean
clean:
	rm -f $(TARGETS) $(TARGETS:=.o)
