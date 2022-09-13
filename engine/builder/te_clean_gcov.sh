#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) -2022 OKTET Labs Ltd. All rights reserved.
#
# Helper script to clean gcov files
#
# $1 build directory

build=$1

test \( -d "${build}" \) -a \( -O ${build} \) || \
    exit 1

cd "${build}" 2> /dev/null || \
    exit 1

files=$(find . -name \*.gcda -o -name \*.gcov 2> /dev/null) || \
    exit 1

echo "${files}" | \
    while read f ; do
        test -z "$f" && continue ;
        rm -f "${build}/$f" 2> /dev/null || exit 1 ;
    done
:
