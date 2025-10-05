CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -g

all: main

main: main.cpp bounded_queue.hpp bounded_queue.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o main

test: test_bounded_queue.cpp bounded_queue.hpp bounded_queue.cpp
	$(CXX) $(CXXFLAGS) test_bounded_queue.cpp -o test

clean:
	rm -f main test