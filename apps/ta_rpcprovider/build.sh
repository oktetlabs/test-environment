#! /bin/bash
#
# Helper script to build ta_rpcprovider application.
# The script uses environment variables exported by the TE_TA_APP macro.
#
# Copyright (C) 2020 OKTET Labs.
#
# Author Damir.Mansurov <Damir.Mansurov@oktetlabs.ru>
#

set -e

declare -a meson_args
te_cppflags=
for f in ${TE_CPPFLAGS} ; do
    test -z "${te_cppflags}" || te_cppflags+=","
    te_cppflags+="${f}"
done
meson_args+=(-Dte_cppflags="${te_cppflags}")

te_libdir=
te_libs=
for l in ${TE_LDFLAGS} ; do
    if test "${l:0:2}" = "-L" ; then
        if test -n "${te_libdir}" ; then
            echo "ERROR: only one -L flag is supported" >&2
            exit 1
        fi
        te_libdir="${l:2}"
    elif test "${l:0:2}" = "-l" ; then
        test -z "${te_libs}" || te_libs+=","
        te_libs+="${l:2}"
    else
        echo "ERROR: unsupported item '${l}' in TE_LDFLAGS" >&2
        exit 1
    fi
done
meson_args+=(-Dte_libdir="${te_libdir}")
meson_args+=(-Dte_libs="${te_libs}")

echo "${meson_args[@]}" >meson.args.new

if test ! -f meson.args ; then
    meson setup ${PWD} ${EXT_SOURCES} "${meson_args[@]}"
    echo "${meson_args[@]}" >meson.args
elif diff -q meson.args meson.args.new 2>/dev/null ; then
    meson configure "${meson_args[@]}"
fi

which ninja &>/dev/null && NINJA=ninja || NINJA=ninja-build
${NINJA} -v
