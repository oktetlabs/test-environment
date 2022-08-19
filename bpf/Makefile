# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
CLANG = clang
LLC = llc

SRCS = \
tc_delay.c \
tc_drop.c \
tc_dup.c

OBJS = $(patsubst %.c,%.o,$(SRCS))
IRS = $(patsubst %.c,%.ll,$(SRCS))

CFLAGS ?=
ifdef TC_DELAY_FRAME_SIZE
CFLAGS += -DTC_DELAY_FRAME_SIZE=$(TC_DELAY_FRAME_SIZE)
endif

all: ${OBJS}

clean:
	rm -f ${OBJS} ${IRS}

%.o: %.c
	$(CLANG) -S \
	-target bpf \
	$(CFLAGS) \
	-Wall \
	-Wno-unused-value \
	-Wno-pointer-sign \
	-Wno-compare-distinct-pointer-types \
	-Werror \
	-O2 -emit-llvm -c -g $<
	$(LLC) -march=bpf -filetype=obj -o $@ ${@:.o=.ll}
