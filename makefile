# Makefile for DNS Proxy Project

CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2
TARGET=dns

# Source directory
SRCDIR=src

# Source files
SOURCES=$(SRCDIR)/main.c $(SRCDIR)/dns.c $(SRCDIR)/filter.c $(SRCDIR)/forwarder.c $(SRCDIR)/args.c
OBJECTS=$(SOURCES:.c=.o)
HEADERS=$(SRCDIR)/main.h $(SRCDIR)/dns.h $(SRCDIR)/filter.h $(SRCDIR)/forwarder.h $(SRCDIR)/args.h

# Test files
TEST_SCRIPT=test_dns.sh

# Default target
all: $(TARGET)

# Build the DNS proxy
$(TARGET): $(OBJECTS) 
	$(CC) -o $@ $^

# Compile object files from src/
$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(SRCDIR)/*.o
	rm -f test_filters.txt proxy.log empty_filters.txt test_comment_filter.txt test_output.txt

# Run tests
test: $(TARGET)
	@chmod +x $(TEST_SCRIPT)
	@./$(TEST_SCRIPT)

# Run with default settings
run: $(TARGET)
	@if [ ! -f serverlist.txt ]; then \
		echo "Error: serverlist.txt not found!"; \
		exit 1; \
	fi
	./$(TARGET) -s 8.8.8.8 -p 5353 -f serverlist.txt -v

# Help target
help:
	@echo "DNS Proxy Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make       - Build the DNS proxy"
	@echo "  make clean - Remove build artifacts"
	@echo "  make test  - Run test suite"
	@echo "  make run   - Run proxy with default settings"
	@echo "  make help  - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  ./dns -s 8.8.8.8 -p 5353 -f serverlist.txt"
	@echo "  ./dns -s 1.1.1.1 -p 5353 -f serverlist.txt -v"

.PHONY: all clean test run help