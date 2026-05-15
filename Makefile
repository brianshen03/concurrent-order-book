CXX = g++
CXXFLAGS = -std=c++17 -O2

SRCS = src/main.cpp src/order_book.cpp src/order_generator.cpp
TARGET = main

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -rf $(TARGET)