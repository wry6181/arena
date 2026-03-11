CC ?= gcc

CFLAGS = -m64 -std=c11 -Wall -Wextra -pedantic

DEBUG_FLAGS = -DDEBUG -O0 -g
RELEASE_FLAGS = -DNDEBUG -O2

config ?= debug

ifeq ($(config),debug)
	CFLAGS += $(DEBUG_FLAGS)
else
	CFLAGS += $(RELEASE_FLAGS)
endif

all: main

main:
	$(CC) main.c $(CFLAGS) -o main.out

.PHONY: all main
