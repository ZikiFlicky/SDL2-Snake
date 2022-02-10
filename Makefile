CC=gcc
NAME=snake_sdl
CFLAGS=-std=c99 -Wall
LINK=-lSDL2
RM=rm -f

OBJECTS=main.o

.PHONY: all clean

all: clean $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(NAME) $(LINK)

clean:
	$(RM) $(OBJECTS) $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< $(LINK)
