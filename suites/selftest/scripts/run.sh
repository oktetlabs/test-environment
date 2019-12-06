#!/bin/bash

export TS_TOPDIR="$(cd "$(dirname "$(which "$0")")"; pwd -P)"
if [ "$(basename ${TS_TOPDIR})" == "scripts" ]; then
    export TS_TOPDIR=$(dirname ${TS_TOPDIR})
fi
. ${TS_TOPDIR}/scripts/guess.sh
[ -n "$TE_BASE" ] || exit 1

TS_OPTS=
for opt ; do
    case "${opt}" in
        *)
            TS_OPTS+="${opt} ";;
    esac
    shift 1
done

TS_CONF_DIRS=
TS_CONF_DIRS+="\"${TS_TOPDIR}\"/conf:"

TS_DEFAULT_OPTS=

TS_DEFAULT_OPTS+="--conf-dirs=${TS_CONF_DIRS} "
TS_DEFAULT_OPTS+="--build-parallel "
TS_DEFAULT_OPTS+="--build-meson "

eval "${TE_BASE}/dispatcher.sh ${TS_DEFAULT_OPTS} ${TS_OPTS}"
TS_RESULT=$?

if test ${TS_RESULT} -ne 0 ; then
    echo FAIL
    echo ""
    exit ${TS_RESULT}
fi