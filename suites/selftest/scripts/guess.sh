#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2023 OKTET Labs Ltd. All rights reserved.
#
# Script should be sources to guess TE parameters if they're not
# specified in the shell environment.

TEST_TE_BASE=(
    "${TS_TOPDIR}/../.."
    "../../.."
    "../.."
    ".."
    "."
)

for loop in once ; do
    test -e "${TE_BASE}/dispatcher.sh" && break

    for path in ${TEST_TE_BASE[*]}; do
        test -e "${path}/dispatcher.sh" || continue

        export TE_BASE="$(cd "${path}"; pwd)"
        echo "Guessing TE_BASE=$TE_BASE"
        break
    done

    test -e "${TE_BASE}/dispatcher.sh" || echo "Cannot guess TE_BASE" >&2
done
export TE_TS_DIR=${TS_TOPDIR}/ts
export TE_TS_SSH_KEY_DIR=${TS_TOPDIR}/conf/keys

if [[ -z "${TE_BUILD}" ]] ; then
    if [[ "$(pwd -P)" = "${TS_TOPDIR}" ]] ; then
        export TE_BUILD="${TS_TOPDIR}/build"
        mkdir -p "${TE_BUILD}"
    fi
fi
