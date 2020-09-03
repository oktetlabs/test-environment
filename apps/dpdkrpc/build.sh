#! /bin/bash
#
# Helper script to build dpdkrpc application.
#
# Copyright (C) 2020 OKTET Labs. All rights reserved.

set -e

build_using_make() {
    cp -a "${EXT_SOURCES}"/Makefile .
    make V=1 RTE_SDK_BIN="${RTE_SDK_BIN}" O="$(pwd -P)"
    # DPDK install rules for make do not fit us
    for ta_type in ${TE_TA_TYPES} ; do
        cp -t "${TE_AGENTS_INST}/${ta_type}/" dpdkrpc
    done
}

# DPDK build location depends on meson vs autotools build
RTE_SDK_BIN="$(pwd -P)/../../../ext/dpdk/build"
test -d ${RTE_SDK_BIN} ||
    RTE_SDK_BIN="$(pwd -P)/../../../platforms/$(basename "${TE_PREFIX}")/lib/dpdk"

if test -f "${RTE_SDK_BIN}/.config" ; then
    build_using_make
else
    echo "Failed to build dpdkrpc - no .config file" >&2
    exit 1
fi
