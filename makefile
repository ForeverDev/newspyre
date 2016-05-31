CC = gcc
CF = -std=c99
OBJ = build/spyre.o build/main.o build/api.o build/asmlexer.o build/assembler.o

all: spy.exe

spy.exe: $(OBJ)
	$(CC) $(CF) $(OBJ) -o spy.exe
	rm -Rf build/*.o

build/spyre.o: 
	$(CC) $(CF) -c interpreter/spyre.c -o build/spyre.o

build/api.o:
	$(CC) $(CF) -c interpreter/api.c -o build/api.o

build/asmlexer.o:
	$(CC) $(CF) -c assembler/asmlexer.c -o build/asmlexer.o

build/assembler.o:
	$(CC) $(CF) -c assembler/assembler.c -o build/assembler.o

build/main.o:
	$(CC) $(CF) -c main.c -o build/main.o

clean:
	rm -Rf build/*.o
	rm spy.exe
