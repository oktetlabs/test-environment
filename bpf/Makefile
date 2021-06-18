CLANG = clang
LLC = llc

# Default size for a delayed frame in tc_delay.c BPF program.
TC_DELAY_FRAME_SIZE ?= 1514

# Size of a frame chunk stored in a single entry of the map containing delayed
# frame in tc_delay.c. Such chunks are used due to RCF restrictions.
# See bug 11019.
TC_DELAY_CHUNK_SIZE = 256

SRCS = \
tc_delay.c \
tc_drop.c \
tc_dup.c

OBJS = $(patsubst %.c,%.o,$(SRCS))
IRS = $(patsubst %.c,%.ll,$(SRCS))

CFLAGS ?=

all: ${OBJS}

clean:
	rm -f ${OBJS} ${IRS} tc_delay.c

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

# Special target for tc_delay BPF program. Before compiling we need to
# generate the C source and update it with the specified size of a frame
# we're going to delay.
tc_delay.c: tc_delay.m4
	m4 -Dframe_size=$(TC_DELAY_FRAME_SIZE) -Dchunk_size=$(TC_DELAY_CHUNK_SIZE) \
	$< > $@

# Make the "make" tool always re-generate tc_delay.c regardless of whether
# tc_delay.m4 was modified or not.
.PHONY: tc_delay.c
