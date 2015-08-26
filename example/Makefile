.PHONY: all

CC = g++
#~ CC = clang++
#~ CC = icc

CFLAGS = -O0 -rdynamic -std=c++11 -Wall -Wextra -Wpedantic -Werror
OFF_FLAGS = -DDEBUGPRINTER_OFF -DDEBUGPRINTER_NO_CXXABI -DDEBUGPRINTER_NO_EXECINFO -DDEBUGPRINTER_NO_SEGVSTACK

all: test.cpp
	$(CC) $(CFLAGS) -o test  $<
