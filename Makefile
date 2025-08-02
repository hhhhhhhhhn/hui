CFLAGS += -Wall -Werror -Wextra -Wpedantic -std=c99 -g
LINK_FLAGS += -lraylib

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
	$(CC) $(CFLAGS) $(LINK_FLAGS) -o main main.c hlib.o hui.o

todo: todo.c hlib.o hui.o
	$(CC) $(CFLAGS) $(LINK_FLAGS) -o todo todo.c hlib.o hui.o

hlib.o: $(wildcard hlib/*.c) $(wildcard hlib/*.h)
	$(CC) $(CFLAGS) hlib/hlib.c -c -o hlib.o

hui.o: $(wildcard hui/*.c) $(wildcard hui/*.h)
	$(CC) $(CFLAGS) hui/lib.c -c -o hui.o

clean:
	rm -f *.o *_test main
