all: build

build:
	gcc main.c -Wall -o ex1 -lpthread 

clean:
	rm -f ex1