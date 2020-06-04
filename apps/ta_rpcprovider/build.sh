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
te_ldflags=

# There is way to use meson cross files to build m32, but the 'pkg_config_libdir'
# property is supported on meson starting from 0.54.0
if test "${TE_IUT_CFLAGS_VAR/-m32/}" != "${TE_IUT_CFLAGS_VAR}" ; then
    te_cppflags+="-m32"
    te_ldflags+="-m32"
    pkg_cfg_path="$(pkg-config --variable pc_path pkg-config)"
    if test "${pkg_cfg_path/x86_64/}" != "${pkg_cfg_path}" ; then
        # Debian/Ubuntu
        export PKG_CONFIG_PATH="${pkg_cfg_path//x86_64/i386}"
    elif test "${pkg_cfg_path/lib64/}" != "${pkg_cfg_path}" ; then
        # RHEL7, RHEL8, Fedora
        export PKG_CONFIG_PATH="${pkg_cfg_path//lib64/lib}"
    else
        echo "ERROR: unknown PKG_CONFIG_PATH: $pkg_cfg_path" >&2
        exit 1
    fi
fi

for f in ${TE_CPPFLAGS} ; do
    test -z "${te_cppflags}" || te_cppflags+=","
    te_cppflags+="${f}"
done
meson_args+=(-Dte_cppflags="${te_cppflags}")
meson_args+=(-Dte_ldflags="${te_ldflags}")

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
