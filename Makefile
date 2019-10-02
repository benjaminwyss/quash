quash: main.o
	g++ -std=c++11 main.o -o quash

main.o: main.cpp
	g++ -c -std=c++11 main.cpp

clean:
	rm -f quash *.o
