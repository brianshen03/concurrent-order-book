all:
	g++ -std=c++17 src/main.cpp src/order_book.cpp -o main

clean:
	rm -rf ./main
