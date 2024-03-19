make: 
	gcc -lm $(shell pkg-config --cflags --libs gtk4) -o ./noise.out *.c
