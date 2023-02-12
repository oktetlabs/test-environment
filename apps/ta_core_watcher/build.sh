#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
#
# Helper script to build ta_core_watcher application.
# The script uses environment variables exported by the TE_TA_APP macro.
#

set -e

declare -a meson_args
te_cppflags=
CROSS_FILE=

process_opts()
{
    while test -n "$1" ; do
        case "$1" in
            --cross-file=*)
                CROSS_FILE="${1#--cross-file=}" ;;

            *)  echo "Unknown option: $1" >&2;
                exit 1 ;;
        esac
        shift 1
    done
}

process_opts "$@"

for f in ${TE_CPPFLAGS} ; do
    test -z "${te_cppflags}" || te_cppflags+=","
    te_cppflags+="${f}"
done
test -z "${te_cppflags}" || meson_args+=(-Dte_cppflags="${te_cppflags}")

test -z "${TE_PREFIX}" || meson_args+=(-Dte_libdir="${TE_PREFIX}/lib")

if test -n "${TE_LIBS}" ; then
    echo "TE_LIBS should be empty for ta_core_watcher, fix TE_TA_APP " \
         "for it in your builder configuration file" >&2;
    exit 1
fi

echo "${meson_args[@]}" >meson.args.new

if test ! -f meson.args ; then
    test -z "${CROSS_FILE}" || meson_args+=(--cross-file="${CROSS_FILE}")
    meson setup ${PWD} ${EXT_SOURCES} "${meson_args[@]}"
    echo "${meson_args[@]}" >meson.args
elif ! diff -q meson.args meson.args.new 2>/dev/null ; then
    meson configure "${meson_args[@]}"
fi

which ninja &>/dev/null && NINJA=ninja || NINJA=ninja-build
${NINJA} -v
