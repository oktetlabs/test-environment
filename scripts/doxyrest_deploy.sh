#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

# Test Environment:
#
# Script to deploy doxyrest that is required for doxygen->RST->SPHINX
# documentation conversion.
#
# Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
#

help() {
    cat <<EOF
Usage: doxyrest_deploy.sh [options]

  --how            : print info about doxyrest installation
  --put            : put doxygest in directory that is $TE_BASE/../
  --help           : print this help
EOF
}

# GitHub answers a release download with a redirect to a signed, short-lived
# URL, so the address cannot be pinned here -- it expires within the hour.
# Set DOXYREST_URL to fetch the same tarball from somewhere else instead,
# a local mirror for example.
DOXYREST_URL=${DOXYREST_URL:-https://github.com/vovkos/doxyrest/releases/download/doxyrest-2.1.3/doxyrest-2.1.3-linux-amd64.tar.xz}

doxyrest_put() {
    action=$1

    ${action} pushd ${TE_BASE}/.. || return 1

    # That redirected transfer sometimes stalls, and wget waits forever by
    # default.  Time the connection out and retry, so a bad moment costs a
    # minute instead of hanging the build, and resume rather than restart.
    ${action} wget --timeout=30 --tries=3 --continue "${DOXYREST_URL}" || {
        echo "Failed to download ${DOXYREST_URL}" >&2
        return 1
    }

    ${action} tar -xf doxyrest-2.1.3-linux-amd64.tar.xz || {
        echo "Downloaded archive is not readable" >&2
        return 1
    }

    ${action} export DOXYREST_PREFIX=${TE_BASE}/../doxyrest-2.1.3-linux-amd64
    ${action} popd
}

while test -n "$1" ; do
    case "$1" in
        --how)
            echo "Please set DOXYREST_PREFIX before:"
            echo "Last release: https://github.com/vovkos/doxyrest/releases"
            echo "------"
            doxyrest_put echo
            ;;
        --put)
            echo "Putting doxyrest"
            doxyrest_put || exit 1
            ;;
        --help)
            help
            exit 0
            ;;
        *)
            help
            exit 1
    esac

    shift
done
