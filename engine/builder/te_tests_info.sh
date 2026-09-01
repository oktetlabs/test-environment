#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Test Environment Builder
#
# Script for extracting of the test objective from .c files.
#
# Usage: te_tests_info.sh <directory>
#
# Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.

SRC_DIR=$1
AWK_PROGRAM=$2

if [ -z $AWK_PROGRAM ] ; then
    AWK_PROGRAM=$(which $0 | sed "s/\.sh/\.awk/")
fi

echo -en \
"<?xml version=\"1.0\"?>\\n"\
"<tests-info>\\n"

# Helper function for awk-based extraction (used when python unavailable or failed)
awk_extract() {
    for i in `find $SRC_DIR -maxdepth 1 -name \*.c` ; do
        TEST_NAME=`basename $i`
        TEST_NAME=${TEST_NAME/.c/}
        cat $i | awk --posix -f $AWK_PROGRAM | sed "s/TAKE_FROM_C_FILE/$TEST_NAME/g"
    done
}

# Parameter docs and scenarios need the python generator; without
# python3 fall back to the awk objective scrape so the build always
# succeeds, and say what is lost.
PYGEN="${TE_BASE}/scripts/scenario/tests_info.py"
if [ -n "${TE_BASE}" ] && command -v python3 >/dev/null 2>&1 && [ -r "${PYGEN}" ] ; then
    PYOUT=$(python3 "${PYGEN}" "${SRC_DIR}")
    if [ $? -eq 0 ] ; then
        printf '%s' "${PYOUT}"
        [ -n "${PYOUT}" ] && printf '\n'
    else
        echo "tests-info: python tests-info generator unavailable or failed;" >&2
        echo "    parameter descriptions and scenarios are not extracted" >&2
        echo "    (install python3, e.g. apt install python3)" >&2
        awk_extract
    fi
else
    echo "tests-info: python tests-info generator unavailable or failed;" >&2
    echo "    parameter descriptions and scenarios are not extracted" >&2
    echo "    (install python3, e.g. apt install python3)" >&2
    awk_extract
fi

for i in `find $SRC_DIR -maxdepth 1 -name \*.xml` ; do
    TEST_NAME=`basename $i`
    TEST_NAME=${TEST_NAME/.xml/}

    cat $i | $TE_BASE/scripts/xml2dox | awk --posix -f $AWK_PROGRAM | sed "s/TAKE_FROM_C_FILE/$TEST_NAME/g"
done

echo "</tests-info>"
