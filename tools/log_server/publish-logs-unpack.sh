#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2023 OKTET Labs Ltd. All rights reserved.
#
# Check incoming directory for tar files with testing logs.
# If found, move logs to storage and, if requested, import it to Bublik.
#

DROP_USER="$(id -u -n)"

DONE_MARKER=".done"
META_DATA_JSON="meta_data.json"
META_VERSION=1
DATE_REGEX="^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[1-2][0-9]|3[0-1])$"
TIME_REGEX="^([0-1][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]$"
NAME_REGEX="^[-_0-9a-zA-Z]{1,32}$"

# Required files in log archive
MANDATORY_FILES=()
MANDATORY_FILES+=("${META_DATA_JSON}")
MANDATORY_FILES+=(raw_log_bundle.tpxz)

# Allowed files in the archive
ALLOWED_FILES=("${MANDATORY_FILES[@]}")
ALLOWED_FILES+=(trc-brief.html)
ALLOWED_FILES+=(trc-stats.txt)
ALLOWED_FILES+=("${DONE_MARKER}")


#######################################################################
# Output optional error message to stderr and usage information
# Arguments:
#   Optional message
# Outputs:
#   Message to stderr, usage and exit with failure in the case of error
#######################################################################
function usage()
{
    [[ $# -eq 0 ]] || echo "$*" >&2

    cat <<EOF

Usage: publish-logs-unpack.sh [-h] [-n] -i <incoming-dir> -s <storage-dir> \\
            [-b <bublik-import>] [-B <bad-incoming> ]

Options:
    -h                  show usage
    -n                  dry run, log actions instead of execution
    -i <incoming-dir>   directory with uploaded logs to publish
    -s <storage-dir>    directory to publish logs to
    -b <bublik-import>  URL prefix to import logs to Bublik
    -B <bad-incoming>   directory to save bad incoming to

EOF

    [[ $# -eq 0 ]] && exit 0 || exit 1
}


#######################################################################
# Output error message prefix to stderr.
# Arguments:
#   Message
# Outputs:
#   Message with ERROR: prefix to stderr.
#######################################################################
function error()
{
    echo "ERROR: $*" >&2
}

#######################################################################
# Get meta value.
# Arguments:
#   String with metas in JSON format
#   Meta name to get value
# Outputs:
#   Meta value or empty string
#######################################################################
function get_meta_value()
{
    local metas="$1"
    local name="$2"

    echo "${metas}" | jq -j -c ".metas[] | select(.name == \"${name}\").value"

}

#######################################################################
# Get mandatory meta value with optional check using regular expression.
# Globals:
#   tarfile
# Arguments:
#   String with metas in JSON format
#   Meta name to get value
#   (optional) Regular expression to check value
# Outputs:
#   Meta value or empty string and error message to stderr
#######################################################################
function get_mandatory_meta_value()
{
    local metas="$1"
    local name="$2"
    local regex="$3"
    local value="$(get_meta_value "${metas}" "${name}")"

    if [[ -z "${value}" ]] ; then
        error "${tarfile}: missing meta ${name}"
        echo return 1
        return 1
    fi
    if [[ -n "${regex}" && ! "${value}" =~ ${regex} ]] ; then
        error "${tarfile}: invalid meta ${name}"
        return 1
    fi
    echo "${value}"
}

#######################################################################
# Make log storage subpath to save logs to.
# Globals:
#   DATE_REGEX
#   META_DATA_JSON
#   META_VERSION
#   NAME_REGEX
#   TIME_REGEX
# Arguments:
#   Tar file
# Outputs:
#   Meta value or empty string and error message to stderr
#######################################################################
function mk_storage_path()
{
    local tarfile="$1"

    local metas="$(tar -xOf "${tarfile}" "${META_DATA_JSON}")"
    local version
    local cdate
    local ts_name
    local cfg
    local timestamp
    local user

    # Check meta data version
    version="$(echo ${metas} | jq -j -c '.version')"
    if [[ "${version}" != ${META_VERSION} ]] ; then
        error "${tarfile}: unsupported meta data version: ${version}"
        return 1
    fi

    # Get mandatory metas and sanity check values
    cdate="$(get_mandatory_meta_value "${metas}" CAMPAIGN_DATE "${DATE_REGEX}")" \
        || return 1
    ts_name="$(get_mandatory_meta_value "${metas}" TS_NAME "${NAME_REGEX}")" \
        || return 1
    cfg="$(get_mandatory_meta_value "${metas}" CFG "${NAME_REGEX}")" \
        || return 1
    timestamp="$(get_mandatory_meta_value "${metas}" START_TIMESTAMP)" \
        || return 1

    # USER is optional in metas, use tar file owner as fallback
    user="$(get_meta_value "${metas}" USER)"
    [[ -n "${user}" ]] || user="$(stat -c '%U' "${tarfile}")"
    [[ "${user}" == "${DROP_USER}" ]] && user=
    if [[ -n "${user}" && ! "${user}" =~ ${NAME_REGEX} ]] ; then
        error "${tarfile}: invalid user name ${user}"
        return 1
    fi

    # Cut date from timestamp
    timestamp="${timestamp#*T}"
    # Cut timezone from timestamp
    timestamp="${timestamp%+*}"
    if [[ ! "${timestamp}" =~ ${TIME_REGEX} ]] ; then
        error "${tarfile}: unexpected time format: ${timestamp}"
        return 1
    fi

    echo "${ts_name}/${cdate//-/\/}/${cfg}${user:+-}${user}-${timestamp}"
}

#######################################################################
# Extract meta information, unpack logs to storage and request import
# to Bublik.
#
# Globals:
#   ALLOWED_FILES
#   BUBLIK_URL
#   DONE_MARKER
#   DRY_RUN
#   MANDATORY_FILES
#   STORAGE_DIR
# Arguments:
#   Tar file
#######################################################################
function process_tar_file()
{
    local tarfile="$1"
    local missing=("${MANDATORY_FILES[@]}")
    local not_allowed_found=false
    local files
    local f
    local subpath

    files=($(tar -tf "${tarfile}")) || return 1
    for f in "${files[@]}"; do
        missing=("${missing[@]/$f}")

        local f_regex="\<${f}\>"
        if [[ ! "${ALLOWED_FILES[@]}" =~ ${f_regex} ]]; then
            error "${tarfile}: not allowed file: ${f}"
            not_allowed_found=true
        fi
    done

    if [[ "${#missing}" -ne 0 ]] ;  then
        error "${tarfile}: missing mandatory: ${missing[*]}"
        return 1
    fi
    ${not_allowed_found} && return 1

    subpath="$(mk_storage_path "${tarfile}")" || return 1

    local destdir="${STORAGE_DIR}/${subpath}"
    if [[ -d "${destdir}" ]] ; then
        error "${tarfile}: already published: ${subpath}"
        return 1
    fi

    ${DRY_RUN}mkdir -p "${destdir}" || return 1

    # Extract everything except done marker
    ${DRY_RUN}tar -C "${destdir}" -xf "${tarfile}" "${files[@]/${DONE_MARKER}|}"
    # Create done marker when everything is in place
    ${DRY_RUN}touch "${destdir}/${DONE_MARKER}"

    if [[ -n "${BUBLIK_URL}" ]] ; then
        ${DRY_RUN}curl curl --negotiate -u : "${BUBLIK_URL}/${subpath}"
    fi

    return 0
}

#######################################################################
# Check if specified file is a tar and psss it to process_tar_file.
#
# Arguments:
#   file name
#######################################################################
function process_file()
{
    local myfile="$1"

    if ! file "${myfile}" | grep -q 'POSIX tar archive (GNU)$' ; then
        error "not a tar file: ${myfile}"
    fi
    process_tar_file "${myfile}"
}

DRY_RUN=
while getopts ":hnt:i:s:b:B:" opt ; do
    case "${opt}" in
        h) usage ;;
        n) DRY_RUN="echo " ;;
        i) INCOMING_DIR="${OPTARG}" ;;
        s) STORAGE_DIR="${OPTARG}" ;;
        b) BUBLIK_URL="${OPTARG}" ;;
        B) BAD_INCOMING="${OPTARG}" ;;
        :) usage "ERROR: option -${OPTARG} requires an argument" ;;
        *) usage "ERROR: illegal option -- ${OPTARG}" ;;
    esac
done
# Skip all options
shift $(($OPTIND - 1))

[[ -n "${STORAGE_DIR}" ]] \
    || usage "ERROR: -s storage directory required"
[[ -d "${STORAGE_DIR}" ]] \
    || usage "ERROR: invalid storage directory"

if [[ $# -gt 0 ]] ; then
    [[ -z "${INCOMING_DIR}" ]] \
        || error "incoming direcotory ignored since files are specified"

    for f ; do
        if [[ ! -r "$f" ]] ; then
            error "no such file: $f"
        else
            process_file "$f"
        fi
    done
else
    [[ -n "${INCOMING_DIR}" ]] \
        || usage "ERROR: -i incoming directory required"
    [[ -n "${INCOMING_DIR}" && -d "${INCOMING_DIR}" ]] \
        || usage "ERROR: invalid incoming directory"
    [[ -z "${BAD_INCOMING}" || -d "${BAD_INCOMING}" ]] \
        || usage "ERROR: invalid directory for bad incoming"

    for f in $(find "${INCOMING_DIR}" -type f); do
        process_file "$f"
        if [[ $? -ne 0 && -n "${BAD_INCOMING}" ]] ; then
            ${DRY_RUN}mv -f "$f" "${BAD_INCOMING}"
        else
            ${DRY_RUN}rm -f "$f"
        fi
    done
fi
