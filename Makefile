.DEFAULT_GOAL = all
.PHONY : all clean

all : main

clean :
	-rm main

main:
	gcc -std=c99 -O3 main.c -o main