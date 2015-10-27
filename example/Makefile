.PHONY: all

GCC_WARN = -Werror -Wall -Wextra -Wpedantic
CLANG_WARN = -Wmost -Wextra
#~ CLANG_WARN = -Werror -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-exit-time-destructors -Wno-global-constructors -Wno-implicit-fallthrough -Wno-disabled-macro-expansion -Wno-documentation-unknown-command

CC = g++ $(GCC_WARN)
#~ CC = clang++ $(CLANG_WARN)
#~ CC = icc

CFLAGS = -O0 -rdynamic -std=c++14
OFF_FLAGS = -DDEBUGPRINTER_OFF -DDEBUGPRINTER_NO_CXXABI -DDEBUGPRINTER_NO_EXECINFO -DDEBUGPRINTER_NO_SEGVSTACK

SHARED_H = ../DebugPrinter.hpp

all: test

%: %.cpp $(SHARED_H)
	$(CC) $(CFLAGS) -o $@ $<
