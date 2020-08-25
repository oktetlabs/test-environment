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
if test "${TE_IUT_RPCS_I386_CFLAGS/-m32/}" != "${TE_IUT_RPCS_I386_CFLAGS}" ; then
    source te_meson_m32_exports
    app_name="ta_rpcs_i386"
else
    app_name="ta_rpcs"
fi

for f in ${TE_CPPFLAGS} ; do
    test -z "${te_cppflags}" || te_cppflags+=","
    te_cppflags+="${f}"
done
test -z "${te_cppflags}" || meson_args+=(-Dte_cppflags="${te_cppflags}")
test -z "${te_ldflags}" || meson_args+=(-Dte_ldflags="${te_ldflags}")

test -z "${TE_PREFIX}" || meson_args+=(-Dte_libdir="${TE_PREFIX}/lib")
test -z "${TE_LIBS}" || meson_args+=(-Dte_libs="$(echo ${TE_LIBS} | tr ' ' ,)")
meson_args+=(-Dapp-name="${app_name}")

echo "${meson_args[@]}" >meson.args.new

if test ! -f meson.args ; then
    meson setup ${PWD} ${EXT_SOURCES} "${meson_args[@]}"
    echo "${meson_args[@]}" >meson.args
elif diff -q meson.args meson.args.new 2>/dev/null ; then
    meson configure "${meson_args[@]}"
fi

which ninja &>/dev/null && NINJA=ninja || NINJA=ninja-build
${NINJA} -v
