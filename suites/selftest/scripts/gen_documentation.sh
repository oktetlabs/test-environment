#!/bin/bash
# Copyright (C) -2022 OKTET Labs Ltd. All rights reserved.
#
# - DOXYREST_PREFIX   - prefix path to doxyrest if you want to use sphinx to
#                       generate HTML
#

export TS_TOPDIR="$(cd "$(dirname "$(which "$0")")/.."; pwd -P)"
. ${TS_TOPDIR}/scripts/guess.sh
[ -n "$TE_BASE" ] || exit 1

if [ -z "$DOXYREST_PREFIX" ]; then
    $TE_BASE/scripts/doxyrest_deploy.sh --how
    exit -1
fi
[ -z "$DOXYREST_BIN" ] && DOXYREST_BIN="$DOXYREST_PREFIX/bin/doxyrest"

[ -z "$SPHINX_BUILD_BIN" ] && SPHINX_BUILD_BIN="sphinx-build"
if ! which $SPHINX_BUILD_BIN >/dev/null; then
    echo "Please install sphinx before:"
    echo " $ pip install sphinx sphinx_rtd_theme"
    exit -2
fi

pushd ${TS_TOPDIR} >/dev/null

doxygen ts/doc/Doxyfile >/dev/null

DOXYREST_OPTS=(
    --config=ts/doc/doxyrest-config.lua
    --frame-dir=$DOXYREST_PREFIX/share/doxyrest/frame/cfamily
    --frame-dir=$DOXYREST_PREFIX/share/doxyrest/frame/common
)

${DOXYREST_BIN} "${DOXYREST_OPTS[@]}" || exit 1

SPHINX_BUILD_OPTS=(
    -j auto                # use cpu count
    -q                     # be quite: warn and error only
    ts/doc                 # input
    ts/doc/generated/html  # output
)

${SPHINX_BUILD_BIN} "${SPHINX_BUILD_OPTS[@]}"
popd >/dev/null