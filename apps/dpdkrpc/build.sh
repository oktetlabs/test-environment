#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Helper script to build dpdkrpc application.
#
# Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

set -e

# pkg-config does not handle -Wl,-whole-archive flags gracefully and
# reoders t. Extract the information here, pass via meson argument and
# enforce whole linkage using linker arguments directly.
dpdk_get_libs() {
    local arg
    local whole_libs=""
    local extra_args=""
    local whole_archive=false
    local name

    for arg in $(pkg-config --static --libs libdpdk); do
        case "${arg}" in
            -Wl,--whole-archive)
                whole_archive=true
                ;;
            -Wl,--no-whole-archive)
                whole_archive=false
                ;;
            -l:lib*.a)
                name=${arg#-l:lib}
                name=${name%.a}
                whole_libs+="${whole_libs:+,}${name}"
                ;;
            -lrte_*)
                # DPDK before 20.x use simple form of libs specification
                ${whole_archive} && whole_libs+="${whole_libs:+,}${arg#-l}"
                # Ignore DPDK libs since already listed in static
                ;;
            -l*)
                extra_args+=" ${arg}"
                ;;
        esac
    done
    DPDK_STATIC_LIBS="${whole_libs}"
    DPDK_EXTRA_ARGS="${extra_args}"
}

build_using_meson() {
    local meson_opts=()
    local c_args="${TE_PLATFORM_CPPFLAGS} ${TE_PLATFORM_CFLAGS} ${TE_CPPFLAGS}"
    # Do not pass TE_LDFLAGS since we do it via TE_PREFIX and TE_LIBS
    local c_link_args="${TE_PLATFORM_LDFLAGS}"

    dpdk_get_libs

    # Applications are installed to TA root directory
    meson_opts+=("--prefix" "/")
    meson_opts+=("-Dc_args=${c_args}")
    meson_opts+=("-Dc_link_args=${c_link_args}${DPDK_EXTRA_ARGS}")
    meson_opts+=("-Dte_prefix=${TE_PREFIX}")
    meson_opts+=("-Dte_libs=$(echo ${TE_LIBS} | tr ' ' ,)")
    meson_opts+=("-Drte_libs=${DPDK_STATIC_LIBS}")

    if test "${TE_DO_TCE}" = 1 ; then
        meson_opts+=(-Db_coverage=true)
    fi

    if test -f build.ninja ; then
        meson configure "${meson_opts[@]}"
    else
        meson setup "${meson_opts[@]}" "${EXT_SOURCES}" "$(pwd -P)"
    fi
    if which ninja >/dev/null 2>&1 ; then
        ninja -v
    else
        ninja-build -v
    fi
    for ta_type in ${TE_TA_TYPES} ; do
        DESTDIR="${TE_AGENTS_INST}/${ta_type}" meson install
    done
}

RTE_SDK_BIN="$(pwd -P)/../../../ext/dpdk/build"

build_using_meson
