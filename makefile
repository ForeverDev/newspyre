CC = gcc
CF = -std=c99 -O2
OBJ = build/spyre.o build/main.o build/api.o build/lexer.o build/assembler.o

all: spy.exe

spy.exe: $(OBJ)
	$(CC) $(CF) $(OBJ) -o spy.exe
	rm -Rf build/*.o

build/spyre.o: 
	$(CC) $(CF) -c spyre.c -o build/spyre.o

build/api.o:
	$(CC) $(CF) -c api.c -o build/api.o

build/lexer.o:
	$(CC) $(CF) -c lexer.c -o build/lexer.o

build/assembler.o:
	$(CC) $(CF) -c assembler.c -o build/assembler.o

build/main.o:
	$(CC) $(CF) -c main.c -o build/main.o

clean:
	rm -Rf build/*.o
	rm spy.exe
