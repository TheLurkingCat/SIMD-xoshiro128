ifeq ($(BUILD),debug)
TARGET = libxoshiro128d.a
OBJECT = xoshiro128d.o
CXXFLAGS += -O0 -m64 -march=native -g -Wall -Wextra -Wpedantic -std=c++17 -fPIC
else
TARGET = libxoshiro128.a
OBJECT = xoshiro128.o
CXXFLAGS += -Ofast -m64 -march=native -s -DNDEBUG -Wall -Wextra -Wpedantic -std=c++17 -flto -fPIC -funroll-loops -fdisable-tree-cunrolli -fomit-frame-pointer -mprefer-vector-width=256
endif

all: $(TARGET)

debug:
	make "BUILD=debug"

$(TARGET): $(OBJECT)
	ar rvs $@ $(OBJECT)

$(OBJECT): xoshiro128.cpp xoshiro128.h
	g++ -o $@ -c xoshiro128.cpp $(CXXFLAGS)

clean:
	rm *.a *.o *.out
