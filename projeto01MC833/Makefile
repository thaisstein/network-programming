CFLAGS=-g -Wall -Wextra -Wunreachable-code -Wwrite-strings -pedantic -std=c23 -fno-common -fno-builtin
LDFLAGS=-lsqlite3

# $@ is the name of the target
# $^ is the list of dependencies
# $< is the first dependency

# add @ in front of a command to make it silent

all: server client

server: server.o database.o request.o
	$(CC) $^ -o $@ $(LDFLAGS)

client: client.o request.o
	$(CC) $^ -o $@ $(LDFLAGS)

# .PHONY is a target that is always rebuilt (useful if there are already files named clean or mrproper in the current directory,
# as they would be considered newer than their dependencies, and the rule would therefore never be executed).
.PHONY: clean

clean:
	rm -rf *.o server client .depend

rebuild: clean all
