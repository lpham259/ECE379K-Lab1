CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -g

all: main

main: main.cpp bounded_queue.hpp bounded_queue.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o main

clean:
	rm -f main