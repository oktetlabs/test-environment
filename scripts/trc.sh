#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

TRC_HTML=
TRC_OPTS=
TRC_RAW=
TRC_XML=
TRC_HTML_LOGS=
TRC_VERBOSE=
TRC_SEARCH=
TRC_SEARCH_TEST=
TRC_SEARCH_HASH=
TRC_SEARCH_PARAMS=

. `dirname \`which $0\``/guess.sh

pushd "$(dirname "$(which "$0")")" >/dev/null
TRCDIR="$(pwd -P)/trc/"
popd >/dev/null

TRC_DB="--db=${TRCDIR}/${TRC_DB}"
TRC_KEY2HTML="--key2html=${TRCDIR}/trc.key2html"

function help_info() {
    echo "trc.sh - Script to interact with TRC database via te-trc-report tool."
    echo "There are additional options that involve auxillary TRC-related scripts:"
    echo "  --xml=<xml-file-name> - wrapper around te-trc-log script to convert rawlog to striped xml that can be used for further TRC processing"
    echo "  --search --test=<test-name> --hash=<MD5-hash> | --params <parameters list>"
    echo "    Note: parameters list should be specified in format: 'name1 value1 ...'  "
    te_trc.sh --help
    exit 0
}

while (( "$#" )); do
    case $1 in
        -v)
            TRC_VERBOSE=y
            ;;
        --help)
            help_info
            ;;
        --sanity)
            # Full TRC report, filter only skipped iterations
            TRC_OPTS="${TRC_OPTS} --keys-only --keys-sanity --no-unspec --no-stats-not-run --comparison=normalised"
            if [ "x$TRC_HTML" == "x" ]; then
                TRC_HTML=trc-sanity.html
            fi
            ;;
        --keys-only)
            # Full TRC report, filter only skipped iterations
            TRC_OPTS="${TRC_OPTS} $1 --no-unspec --no-stats-not-run --comparison=normalised"
            if [ "x$TRC_HTML" == "x" ]; then
                TRC_HTML=trc-keys.html
            fi
            ;;
        --keys*)
            # Full TRC report, filter only skipped iterations
            TRC_OPTS="${TRC_OPTS} $1"
            ;;
        --full)
            # Full TRC report, filter only skipped iterations
            TRC_OPTS="${TRC_OPTS} --no-unspec --no-stats-not-run --comparison=normalised"
            if [ "x$TRC_HTML" == "x" ]; then
                TRC_HTML=trc-full.html
            fi
            ;;
        --brief)
            # Brief TRC report, not total statistics, no expected results
            TRC_OPTS="${TRC_OPTS} --no-unspec --no-expected --no-stats-not-run --comparison=normalised"
            if [ "x$TRC_HTML" == "x" ]; then
                TRC_HTML=trc-brief.html
            fi
            ;;
        --bad)
            # Bad TRC report, show only failed iterations
            TRC_OPTS="${TRC_OPTS} --no-unspec --no-exp-passed --no-stats-not-run --comparison=normalised"
            if [ "x$TRC_HTML" == "x" ]; then
                TRC_HTML=trc-bad.html
            fi
            ;;
        --html=*)
            TRC_HTML=${1#--html=}
            ;;
        --html-logs=*)
            TRC_HTML_LOGS=${1#--html-logs=}
            ;;
        --key2html=*)
            TRC_KEY2HTML="$1"
            ;;
        --db=*)
            TRC_DB="$1"
            ;;
        --xml=*)
            TRC_XML=${1#--xml=}
            ;;
        --search*)
            TRC_SEARCH="y"
            ;;
        --test=*)
            TRC_SEARCH_TEST=${1#--test=}
            ;;
        --hash=*)
            TRC_SEARCH_HASH=${1#--hash=}
            ;;
        --params*)
            shift 1
            TRC_SEARCH_PARAMS="${TRC_SEARCH_PARAMS} -p $@"
            break
            ;;
        --*)
            TRC_OPTS="$TRC_OPTS $1"
            ;;
        *)
            TRC_RAW="$TRC_RAW $1"
            ;;
    esac
    shift 1;
done

if [ "x$TRC_RAW" == "x" ]; then
    TRC_RAW=tmp_raw_log
fi

if [ "x$TRC_XML" != "x" ]; then
    te-trc-log ${TRC_RAW} > ${TRC_XML}
    exit 0
fi

if [ "x$TRC_SEARCH" != "x" ]; then
    if [ "x$TRC_SEARCH_TEST" == "x" ]; then
        TRC_SEARCH_TEST="*"
    fi
    if [ "x$TRC_SEARCH_HASH" != "x" ]; then
        TRC_SEARCH_PARAMS="${TRC_SEARCH_HASH} ${TRC_SEARCH_PARAMS}"
    fi
    TRC_DB=${TRC_DB#--db=}

    echo "te_trc_search ${TRC_DB} ${TRC_SEARCH_TEST} ${TRC_SEARCH_PARAMS}"
    te_trc_search ${TRC_DB} ${TRC_SEARCH_TEST} ${TRC_SEARCH_PARAMS}
    exit 0
fi

if [ "x$TRC_HTML" == "x" ]; then
    TRC_HTML=trc.html
fi

if [ "x$TRC_HTML_LOGS" == "x" ]; then
    TRC_HTML_LOGS=html
#    echo "Assume HTML logs to be in 'html/'. Please update logs with html-log.sh"
fi

TRC_OPTS="${TRC_OPTS} --html-logs=${TRC_HTML_LOGS}"

args_file=$(mktemp)
echo "$0 $@" > ${args_file}

TRC_OPTS="${TRC_OPTS} --show-cmd-file=${args_file}"

if [ "x$TRC_VERBOSE" != "x" ]; then
    echo "te_trc.sh ${TRC_DB} ${TRC_KEY2HTML} --html=$TRC_HTML $TRC_OPTS $TRC_RAW"
fi

te_trc.sh ${TRC_DB} ${TRC_KEY2HTML} --html=$TRC_HTML $TRC_OPTS $TRC_RAW

rm ${args_file}
