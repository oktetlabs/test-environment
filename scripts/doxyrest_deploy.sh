#!/bin/bash

# Test Environment:
#
# Script to deploy doxyrest that is required for doxygen->RST->SPHINX
# documentation conversion.
#
# Copyright (C) 2020-2022 OKTET Labs.
#

help() {
    cat <<EOF
Usage: doxyrest_deploy.sh [options]

  --how            : print info about doxyrest installation
  --put            : put doxygest in directory that is $TE_BASE/../
  --help           : print this help
EOF
}

doxyrest_put() {
    action=$1

    ${action} pushd ${TE_BASE}/..
    ${action} wget https://github.com/vovkos/doxyrest/releases/download/doxyrest-2.1.2/doxyrest-2.1.2-linux-amd64.tar.xz
    ${action} tar -xf doxyrest-2.1.2-linux-amd64.tar.xz
    ${action} export DOXYREST_PREFIX=${TE_BASE}/../doxyrest-2.1.2-linux-amd64
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
            doxyrest_put
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
