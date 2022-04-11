#MAKEFILE para prueba de matelib

CC = gcc

BUILD = build
TARGET = $(BUILD)/libmatelib.so
LIBS = -lcommons -lpthread

SRC = $(shell find ./src -type f -iname "*.c" -exec basename {} \;)
HEADERS = $(wildcard src/*.h)
OBJS = $(SRC:%.c=./obj/%.o)


all: makefolder $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared -o "$@" $(OBJS) $(LIBS)
	
./obj/%.o: ./src/%.c
	$(CC) -c -fPIC -o "$@" "$<"


makefolder:
	if [ ! -d ./obj ]; then mkdir obj; fi

install: all
	sudo cp -u $(TARGET) /usr/lib
	sudo cp -u $(HEADERS) /usr/include

uninstall:
	sudo rm -f /usr/lib/libmatelib.so
	sudo rm -f -r /usr/include

reinstall:
	sudo make uninstall
	sudo make install

clean:
	$(RM) build/*
