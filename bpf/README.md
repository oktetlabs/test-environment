Requirements for BPF agent
==========================

* Linux >= 4.18 (kernel version and headers)
* libelf-dev
* llvm, clang

Build
=====

These programs can be built together with TA with help of `TE_TA_APP`
macro in builder.conf. This example builds `tc_delay.c` and `tc_drop.c,`
defining frame size for `tc_delay.c` to 1000, and copies resulting
object files in "bpf" directory inside TA directory on the test
host.

```
TE_TA_APP([bpf], [${$1_TA_TYPE}], [${$1_TA_TYPE}],
          [${TE_BASE}/bpf], [], [], [],
          [\${EXT_SOURCES}/build.sh --inst-dir=bpf \
           --progs=tc_delay,tc_drop -DTC_DELAY_FRAME_SIZE=1000])
```

There is also a legacy Makefile for building just `tc_delay.c`, `tc_drop.c`
and `tc_dup.c`. It can be used this way:

1. Copy the sources to the Test Agent directory
2. Call `make` in Test Agent directory
3. Some programs (only `tc_delay` by now) use environment variables to control
some build parameters. To set them a user should specify the variable prior
calling the `make`.

List of supported env variables:
- `TC_DELAY_FRAME_SIZE` - size of a frame to delay in `tc_delay` (1514 by default)

Run
===

For `tc_delay`, `tc_drop` and `tc_dup` programs `tapi_bpf_stim` should be used in
tests, see its documentation.
