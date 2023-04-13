if [[ -z "${TS_RIGSDIR}" ]] ; then
    TS_RIGSDIR="${TE_BASE}"/../ts-rigs
    if [[ ! -d "${TS_RIGSDIR}" ]] ; then
       echo "TS_RIGSDIR points to a non-existent directory ${TS_RIGSDIR}"
    fi
    TS_RIGSDIR="$(realpath "${TS_RIGSDIR}")"
    export TS_RIGSDIR
fi

source "${TS_RIGSDIR}/scripts/lib/item"

TS_CONF_DIRS+="\"${TS_RIGSDIR}\":"

take_items "${TS_CFG%-*}" || exit 1

export TE_IUT_TA_SUDO=true
export TE_TST1_TA_SUDO=true

export TE_IUT_TA_NAME="IUT"
export TE_TST1_TA_NAME="TST"
export TE_LOG_LISTENER_TA_NAME="LogListener"
