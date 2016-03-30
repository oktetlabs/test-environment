#
# Guessing:
#  - CONFIG_SUB script location
#  - PLATFORM
#  - TE_LOG_RAW
#  - TE_INSTALL
#

pushd "$(dirname "$(which "$0")")" >/dev/null
MYDIR="$(pwd -P)"
popd >/dev/null

if test -z "${CONFIG_SUB}" ; then
    if test -e "${TE_BASE}/auxdir/config.sub" ; then
        CONFIG_SUB="${TE_BASE}/auxdir/config.sub"
    else    
        CONFIG_SUB=`find /usr/share/automake* -name config.sub | tail -n 1`
    fi    
fi

if test -z "${PLATFORM}" ; then
    PLATFORM=default
fi

if test -z "${TE_LOG_RAW}" ; then
    if test -e tmp_raw_log ; then
        TE_LOG_RAW="$(pwd -P)/tmp_raw_log"
    elif test -e "${MYDIR}/tmp_raw_log" ; then
        TE_LOG_RAW="${MYDIR}/tmp_raw_log"
    fi
fi
if test -z "${TE_INSTALL}" ; then
    if test -d "inst" ; then
        TE_INSTALL="$(pwd -P)/inst"
    elif test -d "${MYDIR}/inst" ; then
        TE_INSTALL="${MYDIR}/inst"
    elif test -d "${MYDIR}/build/inst" ; then
        TE_INSTALL="${MYDIR}/build/inst"
    else
        echo "Failed to guess TE_INSTALL" >&2
        exit 1
    fi
fi

if test -d "conf" ; then
    export CONFDIR="$(pwd -P)/conf"
elif test -d "${MYDIR}/conf" ; then
    export CONFDIR="${MYDIR}/conf"
elif test -d "${MYDIR}/../conf" ; then
    export CONFDIR="${MYDIR}/../conf"
fi

if test -z "$TRC_DB" ; then
    if test -e "sockapi-ts" ; then
        TRC_DB="trc-sockapi-ts.xml"
    elif test -e "iscsi-ts" ; then
        TRC_DB="trc-iscsi-ts.xml"
    elif test -e "layer2-ts" ; then
        TRC_DB="trc-layer2-ts.xml"
    fi
fi

export PATH="$PATH:${TE_INSTALL}/${PLATFORM}/bin"
export LD_LIBRARY_PATH="${TE_INSTALL}/${PLATFORM}/lib:${LD_LIBRARY_PATH}"
