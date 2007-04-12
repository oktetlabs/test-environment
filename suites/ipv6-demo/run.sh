#!/bin/sh

if test -z "${TE_BASE}" ; then
    if test -e dispatcher.sh ; then
        export TE_BASE=`pwd` ;
    else
        echo "TE_BASE environment variable should be set" >&2
        exit 1
    fi
fi

RUN=`which $0`
RUN_DIRNAME=`dirname $RUN`
pushd $RUN_DIRNAME >/dev/null
RUNDIR=`pwd`
popd >/dev/null

export TE_TS_IPV6_DEMO=${RUNDIR}/tests

RCF_CONF=""
CONFIGURATOR_CONF=""
case $1 in
    --cfg=*)
        CFG="${1#--cfg=}"
        RCF_CONF="--conf-rcf=rcf.conf.${CFG}"
        CONFIGURATOR_CONF="--conf-cs=configurator.conf.${CFG}"
        shift 1
        ;;
esac

echo "Run tests (script arguments are passed to dispatcher.sh)"
echo ""

if test -e run.sh ; then
    mkdir -p build
    pushd build >/dev/null
else
    pushd . >/dev/null
fi

${TE_BASE}/dispatcher.sh --conf-dir=${RUNDIR}/conf \
    ${RCF_CONF} ${CONFIGURATOR_CONF} --log-html=html-out $@

if [ $? -eq 0 ] ; then
    echo OK
else
    echo FAIL
fi

popd >/dev/null
echo -e "\a"
