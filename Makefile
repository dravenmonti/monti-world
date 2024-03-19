make: 
	gcc -lm $(shell pkg-config --cflags --libs gtk4) -o ./noise.out *.c
win:
	$(CROSS)gcc -lm $(shell $(CROSS)pkg-config --cflags --libs gtk4) -o ./noise.exe *.c