CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -g

all: main test driver

bounded_queue.o: bounded_queue.cpp bounded_queue.hpp
	$(CXX) $(CXXFLAGS) -c bounded_queue.cpp -o bounded_queue.o

main: main.cpp bounded_queue.o bounded_queue.hpp
	$(CXX) $(CXXFLAGS) main.cpp bounded_queue.o -o main

test: test_bounded_queue.cpp bounded_queue.o bounded_queue.hpp
	$(CXX) $(CXXFLAGS) test_bounded_queue.cpp bounded_queue.o -o test

driver: driver.cpp bounded_queue.o bounded_queue.hpp
	$(CXX) $(CXXFLAGS) driver.cpp bounded_queue.o -o driver

clean:
	rm -f main test driver bounded_queue.o