#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) -2022 OKTET Labs Ltd. All rights reserved.
#
# Helper script to process gcov files
#
# $1 build directory

build=$1

test -n "${build}" || \
    exit 1

cd "${build}" 2> /dev/null || \
    exit 1

files=$(find . -name \*.gcda  2> /dev/null) || \
    exit 1

for f in ${files} ; do
    gcov -p -l -c -b "$f" &> /dev/null || \
        exit 1
done

:
