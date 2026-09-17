CC = cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
BIN_DIR ?= $(HOME)/.local/bin

.PHONY: all install test fixtures clean
all: getting_started

getting_started: grader/main.c grader/catalog.h
	$(CC) $(CFLAGS) grader/main.c -o $@

install: getting_started
	sh tools/install.sh "$(BIN_DIR)"

test: getting_started
	sh tests/test.sh

build/generate: tools/generate.c tools/new_problems.h grader/catalog.h
	mkdir -p build
	$(CC) $(CFLAGS) tools/generate.c -o $@

fixtures: build/generate
	./build/generate

clean:
	rm -f getting_started
	rm -rf build
