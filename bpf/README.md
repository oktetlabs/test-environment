Requirements for BPF agent
==========================

* Linux >= 4.18 (kernel version and headers)
* libelf-dev
* llvm, clang

Build
=====

1. Copy the sources to the Test Agent directory
2. Call `make` in Test Agent directory

Run
===

`tapi_bpf_stim` is responsible fot attaching to inerface and executing these program.

If you want to use this in your tests look at the `tapi_bpf_stim` doc.

