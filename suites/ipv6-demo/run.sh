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

while test -n "$1" ; do
    case $1 in
        --help) usage ;;
        --cfg=*) cfg=${1#--cfg=} ;;
        *)  RUN_OPTS="${RUN_OPTS} $1" ;;
    esac
    shift 1
done

if test -z "$cfg" ; then
    cfg=ulmo
fi

RUN_OPTS="--script=env.$cfg --conf-cs=cs.conf --conf-rcf=rcf.conf"

echo "Run tests (script arguments are passed to dispatcher.sh)"
echo ""

if test -e run.sh ; then
    mkdir -p build
    pushd build >/dev/null
else
    pushd . >/dev/null
fi

${TE_BASE}/dispatcher.sh --conf-dir=${RUNDIR}/conf \
    --log-html=html-out --trc-db=trc.xml \
    --trc-html=trc-report.html --trc-no-total \
    --trc-no-unspec --trc-comparison=normalised ${RUN_OPTS}

if [ $? -eq 0 ] ; then
    echo OK
else
    echo FAIL
fi

popd >/dev/null
echo -e "\a"
