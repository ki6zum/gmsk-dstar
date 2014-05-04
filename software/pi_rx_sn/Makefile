all: gmsktest

gmsktest: main.o
	g++ main.o -o gmsktest -l wiringPi

main.o: main.c
	g++ -c main.c

clean:
	rm *o gmsktest

