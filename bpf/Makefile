CLANG = clang
LLC = llc

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
IRS = $(patsubst %.c,%.ll,$(SRCS))

CFLAGS ?=

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
