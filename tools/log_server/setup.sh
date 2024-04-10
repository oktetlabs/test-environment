#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2024 OKTET Labs Ltd. All rights reserved.
#
# Setup log server.
#

LOGS_USER="te-logs"
WEB_SERVER_GROUP="www-data"
BUBLIK_URL="http://localhost/bublik"
LOGS_URL="http://localhost/logs"


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

Usage: setup.sh [-h] [-n]

The script must be run with superuser rights (under root or using sudo).

Options:
    -h                  show usage
    -n                  dry run, log actions instead of execution
    -u <user>           user to own logs
    -H <user-home>      user home directory
    -s <storage-dir>    logs storage directory
    -g <group>          Web server group
    -B <bublik-url>     Bublik root URL
    -L <logs-url>       Logs root URL
    -T <te-git-url>     Test Environment Git URL to clone from

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
# Output error message prefix to stderr and fail with exit code 1.
# Arguments:
#   Message
# Outputs:
#   Message with ERROR: prefix to stderr.
#######################################################################
function fail()
{
    error "$*"
    exit 1
}

#######################################################################
# Make substitutions in template.
# Arguments:
#   File to make substitutions in
# Globals:
#   BUBLIK_URL
#   DRY_RUN
#   LOGS_CGI_BIN
#   LOGS_BAD
#   LOGS_DIR
#   LOGS_INCOMING
#   LOGS_SHARE_SUBDIR
#   LOGS_URL
#   TE_INSTALL
#######################################################################
function make_template_subst()
{
    local file="$1"
    local logs_url_path="/${LOGS_URL#*://*/}"

    "${DRY_RUN[@]}" sed -s -i \
        -e "s,@@BUBLIK_URL@@,${BUBLIK_URL}," \
        -e "s,@@LOGS_BAD@@,${LOGS_BAD}," \
        -e "s,@@LOGS_CGI_BIN@@,${LOGS_CGI_BIN}," \
        -e "s,@@LOGS_DIR@@,${LOGS_DIR}," \
        -e "s,@@LOGS_INCOMING@@,${LOGS_INCOMING}," \
        -e "s,@@LOGS_SHARED_URL@@,${LOGS_URL}/${LOGS_SHARE_SUBDIR}," \
        -e "s,@@LOGS_URL@@,${LOGS_URL}," \
        -e "s,@@LOGS_URL_PATH@@,${logs_url_path}," \
        -e "s,@@TE_INSTALL@@,${TE_INSTALL}," \
        "${file}" \
        || fail "Cannot make logs handling wrappers"
}

MYDIR="$(cd "$(dirname "$(which "$0")")" && pwd)"

DRY_RUN=()
while getopts ":hnu:H:s:g:B:L:T:" opt ; do
    case "${opt}" in
        h) usage ;;
        n) DRY_RUN=("echo") ;;
        u) LOGS_USER="${OPTARG}" ;;
        H) LOGS_HOME="${OPTARG}" ;;
        s) LOGS_DIR="${OPTARG}" ;;
        g) WEB_SERVER_GROUP="${OPTARG}" ;;
        B) BUBLIK_URL="${OPTARG}" ;;
        L) LOGS_URL="${OPTARG}" ;;
        T) TE_URL="${OPTARG}" ;;
        :) usage "ERROR: option -${OPTARG} requires an argument" ;;
        *) usage "ERROR: illegal option -- ${OPTARG}" ;;
    esac
done
# Skip all options
shift $(($OPTIND - 1))


: ${LOGS_HOME:=/home/${LOGS_USER}}
: ${LOGS_DIR:=${LOGS_HOME}/logs}
: ${LOGS_INCOMING:=${LOGS_HOME}/incoming}
: ${LOGS_BAD:=${LOGS_HOME}/bad}
: ${LOGS_SHARE_SUBDIR:=.share/rgt-xml2html-multi}

: ${TE_URL:=$(git config --get remote.origin.url)}

: ${TE_BASE:=${LOGS_HOME}/te}
: ${TE_BUILD:=${LOGS_HOME}/te-build}
: ${TE_INSTALL:=${TE_BUILD}/inst}


"${DRY_RUN[@]}" useradd -m -U -d "${LOGS_HOME}" -s /usr/sbin/nologin "${LOGS_USER}" \
    || fail "Cannot create logs user"

USER_DO=("${DRY_RUN[@]}" "sudo" "-u" "${LOGS_USER}")

# Create required subdirectories
LOGS_BIN="${LOGS_HOME}/bin"
LOGS_CGI_BIN="${LOGS_HOME}/cgi-bin"
"${USER_DO[@]}" mkdir -p \
    "${LOGS_BAD}"  \
    "${LOGS_BIN}" \
    "${LOGS_CGI_BIN}" \
    "${LOGS_INCOMING}" \
    "${TE_BUILD}" \
    || fail "Cannot create logs home subdirectories"

"${USER_DO[@]}" git clone "${TE_URL}" "${TE_BASE}" \
    || fail "Cannot clone Test Environment"

"${USER_DO[@]}" /bin/bash -c "cd ${TE_BUILD} && ${TE_BASE}/dispatcher.sh -q --conf-builder=builder.conf.tools --no-run" \
    || fail "Cannot clone Test Environment"

# Support logs storage out of home, so create it under root and adjust rights
"${DRY_RUN[@]}" mkdir -p "${LOGS_DIR}" \
    || fail "Cannot create logs storage directory: ${LOGS_DIR}"
# Ensure that it is writable to the logs user
"${DRY_RUN[@]}" chown "${LOGS_USER}" "${LOGS_DIR}" \
    || fail "Cannot chown logs directory"

# Make a copy of static files which may be shared by multi-page HTML logs
LOGS_SHARE_DIR="${LOGS_DIR}/${LOGS_SHARE_SUBDIR}"
"${USER_DO[@]}" mkdir -p "${LOGS_SHARE_DIR}" \
    || fail "Cannot create HTML logs share directory"
"${USER_DO[@]}" rsync -a \
    "${TE_INSTALL}/default/share/rgt-format/xml2html-multi/misc/" \
    "${LOGS_SHARE_DIR}/" \
    || fail "Cannot copy HTML logs shared files"
"${USER_DO[@]}" rsync -a \
    "${TE_INSTALL}/default/share/rgt-format/xml2html-multi/images" \
    "${LOGS_SHARE_DIR}/" \
    || fail "Cannot copy HTML logs shared images"

# Web server should be able to write to the directory to generate
# log file upon request
"${DRY_RUN[@]}" chown -R "${LOGS_USER}:${WEB_SERVER_GROUP}" "${LOGS_DIR}" \
    || fail "Cannot chown logs directory"
# Just make group sticky for now. Publish script will make appropriate
# directories writable to the group.
"${DRY_RUN[@]}" find "${LOGS_DIR}" -type d -exec chmod g+s {} \; \
    || fail "Cannot make group sticky for directories"

#
# Process incoming logs publish template
#
generated="${LOGS_BIN}/publish-incoming-logs"
"${DRY_RUN[@]}" cp "${MYDIR}/publish-incoming-logs.template" "${generated}" \
    || fail "Cannot copy publish incoming logs template"

make_template_subst "${generated}"

"${DRY_RUN[@]}" chmod 750 "${generated}" \
    || fail "Cannot make logs publish wrapper executable"

"${DRY_RUN[@]}" chown "${LOGS_USER}:${LOGS_USER}" "${generated}" \
    || fail "Cannot chown logs scripts"

#
# Process CGI templates
#
for cgi_tmpl in te-logs-error404 te-logs-index ; do
    generated="${LOGS_CGI_BIN}/${cgi_tmpl}"

    "${DRY_RUN[@]}" cp "${MYDIR}/${cgi_tmpl}.template" "${generated}" \
        || fail "Cannot copy CGI handler template"

    make_template_subst "${generated}"

    "${DRY_RUN[@]}" chmod 750 "${generated}" \
        || fail "Cannot make logs handling wrappers executable"

    "${DRY_RUN[@]}" chown "${LOGS_USER}:${WEB_SERVER_GROUP}" "${generated}" \
        || fail "Cannot chown logs scripts"
done

#
# Process helper templates
#
for tmpl in apache2-te-log-server.conf ; do
    generated="${LOGS_HOME}/${tmpl}"

    "${DRY_RUN[@]}" cp "${MYDIR}/${tmpl}.template" "${generated}" \
        || fail "Cannot copy helper template"

    make_template_subst "${generated}"
done

# Make incoming directory group-sticky and writable for the group
# members
"${USER_DO[@]}" chmod g+rws "${LOGS_INCOMING}" \
    || fail "Cannot tune incoming directory mode"

echo "SUCCESS"
exit 0
