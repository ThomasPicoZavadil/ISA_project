CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC = src/main.c src/args.c src/dns.c src/filter.c src/resolver.c
BIN = dns

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
	rm -f $(BIN)
