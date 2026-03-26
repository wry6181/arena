CC ?= gcc

CFLAGS = -m64 -std=c11 -Wall -Wextra -pedantic

DEBUG_FLAGS = -DDEBUG -O0 -g
RELEASE_FLAGS = -DNDEBUG -O3 -march=native -flto -fomit-frame-pointer

config ?= debug

ifeq ($(config),debug)
	CFLAGS += $(DEBUG_FLAGS)
else
	CFLAGS += $(RELEASE_FLAGS)
endif

all: main

main:
	$(CC) server.c $(CFLAGS) -o server.out

.PHONY: all main
