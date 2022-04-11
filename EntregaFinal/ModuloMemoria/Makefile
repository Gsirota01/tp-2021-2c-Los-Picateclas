CC = gcc
CFLAGS = -g
LIBS = -lpthread -lcommons -lm

BIN = bin
OBJ = obj
SRC = src

TARGET = $(BIN)/ModuloMemoria
SRCS = $(wildcard $(SRC)/*.c)
HEADERS = $(wildcard $(SRC)/*.h)
OBJS = $(patsubst $(SRC)/%.c, obj/%.o, $(SRCS))

.PHONY: default all clean

all: makefolder $(TARGET)

release: CFLAGS = -Wall -O2 -DNDEBUG
release: clean
release: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ $(CFLAGS) -o $@ $(LIBS)

$(OBJ)/%.o: $(SRC)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(BIN)/* $(OBJ)/*
	
makefolder:
	if [ ! -d ./obj ]; then mkdir obj; fi
	if [ ! -d ./bin ]; then mkdir bin; fi