CC = gcc
CFLAGS = -Wall -Wextra -O2  --std=c11

BIN = test_netlink
SRC = $(BIN).c

all: $(BIN)

$(BIN) : $(SRC)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm $(BIN)
