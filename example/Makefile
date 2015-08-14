.PHONY: all

CC = g++
#~ CC = clang++
#~ CC = icc

CFLAGS = -O0 -rdynamic -std=c++11

all: test.cpp
	$(CC) $(CFLAGS) -o test  $<
	#$(CC) $(CFLAGS) -DDEBUGPRINTER_NO_CXXABI -DDEBUGPRINTER_NO_EXECINFO -o test $<
