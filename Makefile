CC=g++
SRC=src
BIN=bin
OBJ=obj

all: tsstore

tsstore: mkdirs tsstore.o
	$(CC) $(OBJ)/tsstore.o -o $(BIN)/tsstore

tsstore.o:
	$(CC) -c $(SRC)/tsstore.cc -o $(OBJ)/tsstore.o

mkdirs:
	mkdir $(BIN)
	mkdir $(OBJ)

clean:
	rm -rf $(OBJ)
	rm -rf $(BIN)
