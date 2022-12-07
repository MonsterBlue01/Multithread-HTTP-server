# Make httpserver with httpserver.c, bind.c, queue.c, and filelinkedlist.c

CC = clang
CFLAGS = -Wall -Werror -Wextra -pedantic -lpthread -g

all: httpserver

httpserver: httpserver.c bind.c queue.c filelinkedlist.c
	$(CC) $(CFLAGS) -o httpserver httpserver.c bind.c queue.c filelinkedlist.c

clean:
	rm -f httpserver