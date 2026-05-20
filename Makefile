CXX = g++
CXXFLAGS = -std=c++17 -O2

SHARED = src/order_book.cpp src/order_generator.cpp

all: main tests

main: src/main.cpp $(SHARED)
	$(CXX) $(CXXFLAGS) $^ -o $@

tests: src/tests.cpp $(SHARED)
	$(CXX) $(CXXFLAGS) $^ -o $@

bench: main
	./main
	python3 plot.py

clean:
	rm -f main tests bench_runs.csv bench_scaling.csv bench_workload.csv
	rm -rf plots/
