#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Script to generate test log.
#
# Copyright (C) 2012-2022 OKTET Labs Ltd. All rights reserved.

TS_GUESS_SH="$(dirname "$(which "$0")")"/guess.sh
if test -f "${TS_GUESS_SH}" ; then
    . ${TS_GUESS_SH}
else
    . "${TE_BASE}"/scripts/guess.sh
fi

RUN_DIR="${PWD}"
: ${HTML_OPTION:=true}
: ${EXEC_NAME:=$0}

export TMPDIR="${TMPDIR:-${TE_TMP:-/tmp/}}"

declare -a UNKNOWN_OPTS

CONF_LOGGER="${CONFDIR}"/logger.conf
SNIFF_LOG_DIR=
SNIFF_LOGS_INCLUDED=false
OUTPUT_LOCATION=
OUTPUT_TXT=true
OUTPUT_HTML=false
OUTPUT_STDOUT=false
TMP_LOG_FILE=
INPUT_FILE=

cleanup() {
    if [[ -n "${TMP_LOG_FILE}" ]] ; then
        rm "${TMP_LOG_FILE}"
        TMP_LOG_FILE=
    fi
}

trap cleanup EXIT INT TERM

usage()
{
cat <<EOF
Usage: `basename "${EXEC_NAME}"` [<options>] [raw log or bundle file]
  --sniff-log-dir=<path>        Path to the *TEN* side capture files.
  --output-to=<path>            Location where you want the log to be stored.
EOF
if $HTML_OPTION ; then
cat <<EOF
                                It should either not exist, or be a file, if
                                you want a text log, or a directory for html.
  -p, --sniff-log               Include sniffer dumps.
                                WARNING: if you changed sniffer log directory
                                 in the way it could not be guessed easily
                                 (e.g. via one of run.sh options --logger-conf,
                                 --conf-dir, --cfg, --opts or --sniff-log-dir),
                                 you'd better pass --sniff-log-dir to this
                                 script or --log-html to run.sh (dispatcher.sh)
                                 next time you use it.
  -P, --detailed-packets        Print more detailed packet dumps.
  --html                        Generate log in html format (instead of text).
  --mi                          Generate a stream of MIs
  -t <timeout_ms>               Same as:
  --txt-timeout=<timeout_ms>        When creating a test log, group similar
                                messages under one header if they have the
                                same non-timestamp fields and timestamp
                                differ less than the specified timeout
EOF
else
cat <<EOF
                                It should either not exist, or be a directory.
EOF
fi
cat <<EOF
  -h, --help                    Show this help message.

EOF
}

process_opts()
{
    while test -n "$1" ; do
        case "$1" in
            -h) ;&
            --help) usage ; exit 0 ;;

            -p) ;&
            --sniff-log)
                SNIFF_LOGS_INCLUDED=true
                PROC_OPTS+=("$1")
                ;;

            -P) ;&
            --detailed-packets)
                PROC_OPTS+=("$1")
                ;;

            --sniff-log-dir=*)
                SNIFF_LOG_DIR="${1#--sniff-log-dir=}"
                PROC_OPTS+=("$1")
                ;;

            -t)
                shift
                PROC_OPTS+=("--txt-timeout=$1")
                ;;
            --txt-timeout)
                PROC_OPTS+=("$1")
                ;;

            --txt-*)
                PROC_OPTS+=("$1")
                ;;
            --mi)
                PROC_OPTS+=("--txt-mi-only" "--txt-mi-raw" "--txt-no-prefix")
                OUTPUT_TXT=false
                OUTPUT_STDOUT=true
                ;;

            --output-to=*) OUTPUT_LOCATION="${1#--output-to=}" ;;

            --html)
                OUTPUT_HTML=true
                OUTPUT_TXT=false
                ;;

            -*) ;&
            --*)
                echo "WARNING: unknown option $1 will be passed to rgt-proc-raw-log" >&2;
                UNKNOWN_OPTS+=("$1")
                ;;

            *)
                if [[ -n "${INPUT_FILE}" ]] ; then
                    echo "ERROR: more than one input file is specified" >&2;
                    exit 1
                fi
                INPUT_FILE=$1
                ;;
        esac
        shift 1
    done
}


process_opts "$@"

if [[ -z "${INPUT_FILE}" ]] ; then
    PROC_OPTS+=("--raw-log=${TE_LOG_RAW}")
else
    if [[ "${INPUT_FILE}" == *.tpxz ]] ; then
        TMP_LOG_FILE="$(mktemp "${TMPDIR}/log_XXXXXX.raw")"

        rgt-log-bundle-get-original --bundle="${INPUT_FILE}" \
            --output="${TMP_LOG_FILE}"
        if [[ $? -ne 0 ]] ; then
            echo "ERROR: failed to get RAW log from bundle" >&2
            exit 1
        fi
        INPUT_FILE="${TMP_LOG_FILE}"
    fi

    PROC_OPTS+=("--raw-log=${INPUT_FILE}")
fi

# Check the output location for consistency and create path, if necessary.
if test -z "${OUTPUT_LOCATION}" ; then
    $OUTPUT_TXT && OUTPUT_LOCATION='log.txt'
    $OUTPUT_HTML && OUTPUT_LOCATION='html'
    $OUTPUT_STDOUT && OUTPUT_LOCATION='/dev/stdout'
elif test -e "${OUTPUT_LOCATION}" ; then
    if [ -f "${OUTPUT_LOCATION}" ] && $OUTPUT_HTML ; then
        echo "ERROR: html log requested, while output location" \
             "${OUTPUT_LOCATION} is a file"
        exit 1
    elif [ -d "${OUTPUT_LOCATION}" ] && ! $OUTPUT_HTML ; then
        echo "ERROR: text log requested, while output location" \
             "${OUTPUT_LOCATION} is a directory"
        exit 1
    fi
else
    OUTPUT_DIRNAME=`dirname "${OUTPUT_LOCATION}"`
    if [ -e "${OUTPUT_DIRNAME}" ] && [ ! -d "${OUTPUT_DIRNAME}" ] ; then
        echo "ERROR: ${OUTPUT_DIRNAME} is not a directory"
        exit 1
    elif test ! -e "${OUTPUT_DIRNAME}" ; then
        mkdir -p "${OUTPUT_DIRNAME}"
        if test $? -ne 0 ; then
            exit 1
        fi
    fi
fi

if $SNIFF_LOGS_INCLUDED && $OUTPUT_HTML ; then
    echo "WARNING: html log includes detailed packet dumps by default" >&2
fi

if ! $SNIFF_LOGS_INCLUDED && $OUTPUT_HTML ; then
    PROC_OPTS+=("--sniff-log")
    SNIFF_LOGS_INCLUDED=true
fi

if $SNIFF_LOGS_INCLUDED && test -z "${SNIFF_LOG_DIR}" ; then
    if test -f "${CONF_LOGGER}" ; then
        tmp=`te_log_get_path "${CONF_LOGGER}"`
        if test -n "${tmp}" ; then
            SNIFF_LOG_DIR="${tmp}"
            PROC_OPTS+=("--sniff-log-dir=${SNIFF_LOG_DIR}")
        fi
    fi
fi

if $OUTPUT_HTML ; then
    PROC_OPTS+=("--html=${OUTPUT_LOCATION}")
elif $OUTPUT_TXT || $OUTPUT_STDOUT ; then
    PROC_OPTS+=("--txt=${OUTPUT_LOCATION}")
fi

rgt-proc-raw-log "${PROC_OPTS[@]}" "${UNKNOWN_OPTS[@]}"

if $OUTPUT_TXT ;  then
    # If text log was generated, show it to the user
    if [[ -e "${OUTPUT_LOCATION}" ]] ; then
        ${PAGER:-less} "${OUTPUT_LOCATION}"
        echo "Log was saved in ${OUTPUT_LOCATION}"
    else
        echo "Failed to generate log in text format" >&2
    fi
fi
