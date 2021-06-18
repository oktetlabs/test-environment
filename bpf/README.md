Requirements for BPF agent
==========================

* Linux >= 4.18 (kernel version and headers)
* libelf-dev
* llvm, clang

Build
=====

1. Copy the sources to the Test Agent directory
2. Call `make` in Test Agent directory
3. Some programs (only `tc_delay` by now) use environment variables to control
some build parameters. To set them a user should specify the variable prior
calling the `make`.

List of supported env variables:
- `TC_DELAY_FRAME_SIZE` - size of a frame to delay in `tc_delay` (1514 by default)

Run
===

`tapi_bpf_stim` is responsible fot attaching to inerface and executing these program.

If you want to use this in your tests look at the `tapi_bpf_stim` doc.

