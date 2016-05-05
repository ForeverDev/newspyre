CC = gcc
CF = -std=c11
OBJ = spyre.o main.o

all: spy.exe

spy.exe: $(OBJ)
	$(CC) $(CF) $(OBJ) -o spy.exe
	rm -Rf *.o

spyre.o: 
	$(CC) $(CF) -c spyre.c -o spyre.o

main.o:
	$(CC) $(CF) -c main.c -o main.o

clean:
	rm -Rf *.o
	rm spy.exe
