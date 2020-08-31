#! /bin/bash
#
# Helper script to build dpdkrpc application.
#
# Copyright (C) 2020 OKTET Labs. All rights reserved.

set -e

# Meson does not handle -Wl,-whole-archive flags gracefully and reoders
# it. Extract the information here, pass via meson argument and enforce
# whole linkage using linker arguments directly.
dpdk_whole_libs() {
    local arg
    local start_whole_libs=false
    local whole_libs=""
    local name

    for arg in $(pkg-config --static --libs libdpdk); do
        case "${arg}" in
            -Wl,--whole-archive)    start_whole_libs=true ;;
            -Wl,--no-whole-archive) start_whole_libs=false ;;
            -l:lib*.a)
                if ${start_whole_libs} ; then
                    name=${arg#-l:lib}
                    name=${name%.a}
                    whole_libs+="${whole_libs:+,}${name}"
                fi
                ;;
        esac
    done
    echo "${whole_libs}"
}

build_using_make() {
    cp -a "${EXT_SOURCES}"/Makefile .
    make V=1 RTE_SDK_BIN="${RTE_SDK_BIN}" O="$(pwd -P)"
    # DPDK install rules for make do not fit us
    for ta_type in ${TE_TA_TYPES} ; do
        cp -t "${TE_AGENTS_INST}/${ta_type}/" dpdkrpc
    done
}

build_using_meson() {
    meson_opts=()

    # Applications are installed to TA root directory
    meson_opts+=("--prefix" "/")
    meson_opts+=("-Dc_args=${TE_CPPFLAGS}")
    meson_opts+=("-Dte_prefix=${TE_PREFIX}")
    meson_opts+=("-Dte_libs=$(echo ${TE_LIBS} | tr ' ' ,)")
    meson_opts+=("-Drte_libs=$(dpdk_whole_libs)")

    if test "${TE_DO_TCE}" = 1 ; then
        meson_opts+=(-Db_coverage=true)
    fi

    if test -f build.ninja ; then
        meson configure "${meson_opts[@]}"
    else
        meson setup "${meson_opts[@]}" "${EXT_SOURCES}" "$(pwd -P)"
    fi
    ninja -v
    for ta_type in ${TE_TA_TYPES} ; do
        DESTDIR="${TE_AGENTS_INST}/${ta_type}" meson install
    done
}

# DPDK build location depends on meson vs autotools build
RTE_SDK_BIN="$(pwd -P)/../../../ext/dpdk/build"
test -d ${RTE_SDK_BIN} || RTE_SDK_BIN="$(pwd -P)/../../lib/dpdk"

if test -f "${RTE_SDK_BIN}/.config" ; then
    build_using_make
else
    build_using_meson
fi
