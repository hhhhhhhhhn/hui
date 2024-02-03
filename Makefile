CFLAGS = -Wall -Werror -Wextra -Wpedantic --std=c99 -g -lraylib

ifdef debug
	CFLAGS += -DHLIB_DEBUG -fsanitize=undefined -fsanitize=address -fsanitize=leak
endif
ifdef optimize
	CFLAGS += -O3
endif
ifdef profile
	CFLAGS += -lprofiler
endif

all: hlib.o main

main: main.c hlib.o
	cc $(CFLAGS) -o main main.c hlib.o

hlib.o: $(wildcard hlib/*.c)
	cc $(CFLAGS) -c hlib/hlib.c -o hlib.o

clean:
	rm -f *.o *_test
