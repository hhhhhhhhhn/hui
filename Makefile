CFLAGS += -Wall -Werror -Wextra -Wpedantic --std=c99 -g -lraylib

ifdef debug
	CFLAGS += -DHLIB_DEBUG -fsanitize=undefined -fsanitize=address -fsanitize=leak
endif
ifdef optimize
	CFLAGS += -O3
endif
ifdef profile
	CFLAGS += -lprofiler
endif

all: hlib.o main todo

main: main.c hlib.o hui.o
	cc $(CFLAGS) -o main main.c hlib.o hui.o

todo: todo.c hlib.o hui.o
	cc $(CFLAGS) -lcurl -o todo todo.c hlib.o hui.o

hlib.o: $(wildcard hlib/*.c)
	cc $(CFLAGS) -c hlib/hlib.c -o hlib.o

hui.o: $(wildcard hui/*.c)
	cc $(CFLAGS) -c hui/lib.c -o hui.o

clean:
	rm -f *.o *_test main
