#!/bin/bash

export TS_TOPDIR="$(cd "$(dirname "$(which "$0")")"; pwd -P)"
if [ "$(basename ${TS_TOPDIR})" == "scripts" ]; then
    export TS_TOPDIR=$(dirname ${TS_TOPDIR})
fi
. ${TS_TOPDIR}/scripts/guess.sh
[ -n "$TE_BASE" ] || exit 1

# hg cannot save file with 600 permition but ssh require 600 permition
# for private key for, so force changing permission before run for each
# ssh key in TE_TS_SSH_KEY_DIR required here
for keys in ${TE_TS_SSH_KEY_DIR}/*.id_rsa; do
    chmod 600 $keys;
done

TS_OPTS=
for opt ; do
    case "${opt}" in
        --cfg=*)
            TS_CFG=${opt#--cfg=};;
        *)
            TS_OPTS+="${opt} ";;
    esac
    shift 1
done

test -z "${TS_CFG}" || TS_OPTS+="--opts=run/\"${TS_CFG}\" "

TS_CONF_DIRS=
TS_CONF_DIRS+="\"${TS_TOPDIR}\"/conf:"

TS_DEFAULT_OPTS=

TS_DEFAULT_OPTS+="--conf-dirs=${TS_CONF_DIRS} "
TS_DEFAULT_OPTS+="--build-parallel "
TS_DEFAULT_OPTS+="--build-meson "
TS_DEFAULT_OPTS+="--trc-db=\"${TS_TOPDIR}\"/conf/trc.xml "

eval "${TE_BASE}/dispatcher.sh ${TS_DEFAULT_OPTS} ${TS_OPTS}"
TS_RESULT=$?

if test ${TS_RESULT} -ne 0 ; then
    echo FAIL
    echo ""
    exit ${TS_RESULT}
fi
