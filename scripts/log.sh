#! /bin/bash
#
# Script to generate test log.
#
# Copyright (C) 2012 OKTET Labs, St.-Petersburg, Russia
#
# Author Roman Kolobov <Roman.Kolobov@oktetlabs.ru>
#
# $Id: $

RUN_DIR="${PWD}"
: ${OUTPUT_HTML:=false}
: ${HTML_OPTION:=true}
: ${EXEC_NAME:=$0}

rgt_conv_opts=

CONF_LOGGER="${CONFDIR}"/logger.conf
SNIFF_GUESSED_LOG_DIR=`dirname "${TE_LOG_RAW}"`/caps
SNIFF_LOGS_INCLUDED=false
SNIFF_DETAILED_PACKETS=
OUTPUT_LOCATION=

pushd "$(dirname "$(which "$0")")" >/dev/null
SCRIPT_DEF_LOC="$(pwd -P)"
popd >/dev/null

# Try to guess capture logs path
if test -f "${CONF_LOGGER}" ; then
    tmp=`te_log_get_path "${CONF_LOGGER}"`
    if test -n "${tmp}" ; then
        SNIFF_GUESSED_LOG_DIR="${tmp}"
    fi
elif test -e caps ; then
    SNIFF_GUESSED_LOG_DIR="$(pwd -P)/caps"
elif test -e "${SCRIPT_DEF_LOC}/caps" ; then
    SNIFF_GUESSED_LOG_DIR="${SCRIPT_DEF_LOC}/caps"
fi


usage()
{
cat <<EOF
Usage: `basename "${EXEC_NAME}"` [<options>]
  --sniff-log-dir=<path>        Path to the *TEN* side capture files.
                                Guessed: ${SNIFF_GUESSED_LOG_DIR}.
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
            --sniff-log) SNIFF_LOGS_INCLUDED=true ;;
            -P) ;&
            --detailed-packets) SNIFF_DETAILED_PACKETS="--detailed-packets" ;;
            --sniff-log-dir=*) SNIFF_LOG_DIR="${1#--sniff-log-dir=}" ;;

            --output-to=*) OUTPUT_LOCATION="${1#--output-to=}" ;;

            --html) OUTPUT_HTML=true ;;

            *) echo "WARNING: unknown option $1 will be passed to rgt-conv" >&2;
                rgt_conv_opts="${rgt_conv_opts} $1"
                ;;
        esac
        shift 1
    done
}


process_opts "$@"

# Check the output location for consistency and create path, if necessary.
if test -z "${OUTPUT_LOCATION}" ; then
    OUTPUT_LOCATION="log.txt"
    if $OUTPUT_HTML ; then
        OUTPUT_LOCATION="html"
    fi
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

# Check arguments for consistency
if [ "${SNIFF_LOG_DIR}" != "" ] && ! $SNIFF_LOGS_INCLUDED ; then
    echo "ERROR: --sniff-log-dir is supplied, whereas --sniff-log is not" >&2
    exit 1
fi
if [ "${SNIFF_DETAILED_PACKETS}" != "" ] && ! $SNIFF_LOGS_INCLUDED ; then
    echo "ERROR: --detailed-packets is supplied, whereas --sniff-log is not" >&2
    exit 1
fi
if $SNIFF_LOGS_INCLUDED && $OUTPUT_HTML ; then
    echo "WARNING: html log includes detailed packet dumps by default" >&2
fi

# Form full path to sniffer logs
if $OUTPUT_HTML ; then
    SNIFF_LOGS_INCLUDED=true
fi
if $SNIFF_LOGS_INCLUDED ; then
    if test -z "${SNIFF_LOG_DIR}" ; then
        SNIFF_LOG_DIR="${SNIFF_GUESSED_LOG_DIR}"
    fi
    [[ ${SNIFF_LOG_DIR} == /* ]] || [[ ${SNIFF_LOG_DIR} == \~/* ]] ||
        SNIFF_LOG_DIR="${RUN_DIR}/${SNIFF_LOG_DIR}"
    echo "Sniffer logs will be searched for in ${SNIFF_LOG_DIR}" >&2
fi

# Search for pcap files in potential sniffer logs directory,
#  convert them to XML and store names of converted files in sniff_logs
sniff_logs=""
if test ! -d "${SNIFF_LOG_DIR}" ; then
    SNIFF_LOGS_INCLUDED=false;
fi
if $SNIFF_LOGS_INCLUDED ; then
    for plog in `ls "${SNIFF_LOG_DIR}"/ | grep \.pcap$`; do
        plog="${SNIFF_LOG_DIR}/${plog}"
        xlog=${plog/%.pcap/.xml}
        # Actual conversion from pcap to TE XML
        rm -f "${xlog}"
        tshark -r "${plog}" -T pdml | rgt-pdml2xml - "${xlog}"
        if test -e "${xlog}" ; then
            sniff_logs="${sniff_logs} ${xlog}"
        else
            echo "Failed to convert ${plog} to xml" >&2
        fi
    done
    if test -z "${sniff_logs}" ; then
        SNIFF_LOGS_INCLUDED=false
    fi
fi

# Convert raw log to xml, merge it with converted pcap logs (if any)
#  and convert result to desired format (html or text)
LOG_XML=tmp_converted_log.xml
if ! $OUTPUT_HTML ; then
    rgt_conv_opts="${rgt_conv_opts} --no-cntrl-msg"
fi
rgt-conv -f "${TE_LOG_RAW}" -o "${LOG_XML}" ${rgt_conv_opts}
if test -e "${LOG_XML}" ; then
    LOG_XML_MERGED=tmp_converted_log_merged.xml

    if $SNIFF_LOGS_INCLUDED ; then
        rgt-xml-merge "${LOG_XML_MERGED}" "${LOG_XML}" \
                      ${sniff_logs}
        if test -e "${LOG_XML_MERGED}" ; then
            rm "${LOG_XML}"
        else
            LOG_XML_MERGED="${LOG_XML}"
            echo "Failed to merge sniffer logs with main log;" \
                 " main log would still be available" >&2
        fi
        rm ${sniff_logs}
    else
        LOG_XML_MERGED="${LOG_XML}"
    fi

    if $OUTPUT_HTML ; then
        rgt-xml2html-multi -f "${LOG_XML_MERGED}" -o "${OUTPUT_LOCATION}"
        if test $? -eq 0 ; then
            rm "${LOG_XML_MERGED}"
            echo "Logs delivered to ${OUTPUT_LOCATION} directory" >&2
        else
            echo "Failed to generate log in multidocumented HTML format" >&2
        fi
    else
        rgt-xml2text -f "${LOG_XML_MERGED}" -o "${OUTPUT_LOCATION}" \
                     ${SNIFF_DETAILED_PACKETS}
        if test -e "${OUTPUT_LOCATION}" ; then
            rm "${LOG_XML_MERGED}"
            ${PAGER:-less} "${OUTPUT_LOCATION}"
            echo "Log saved in ${OUTPUT_LOCATION}" >&2
        else
            echo "Failed to generate log in text format" >&2
        fi
    fi
else
    echo "Failed to parse raw log" >&2
fi
