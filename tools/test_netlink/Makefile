# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2017-2022 OKTET Labs Ltd. All rights reserved.
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
