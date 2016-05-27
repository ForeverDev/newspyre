CC = gcc
CF = -std=c99
OBJ = spyre.o main.o api.o lexer.o assembler.o

all: spy.exe

spy.exe: $(OBJ)
	$(CC) $(CF) $(OBJ) -o spy.exe
	rm -Rf *.o

spyre.o: 
	$(CC) $(CF) -c spyre.c -o spyre.o

api.o:
	$(CC) $(CF) -c api.c -o api.o

lexer.o:
	$(CC) $(CF) -c lexer.c -o lexer.o

assembler.o:
	$(CC) $(CF) -c assembler.c -o assembler.o

main.o:
	$(CC) $(CF) -c main.c -o main.o

clean:
	rm -Rf *.o
	rm spy.exe
