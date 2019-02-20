#! /bin/bash
#
# Test Environment Builder
#
# Script for extracting of the test objective from .c files.
#
# Usage: te_tests_info.sh <directory> 
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
#
# $Id$
#

SRC_DIR=$1
AWK_PROGRAM=$2

if [ -z $AWK_PROGRAM ] ; then
    AWK_PROGRAM=$(which $0 | sed "s/\.sh/\.awk/")
fi

echo -en \
"<?xml version=\"1.0\"?>\\n"\
"<tests-info>\\n"

for i in `find $SRC_DIR -maxdepth 1 -name \*.c` ; do
    TEST_NAME=`basename $i`
    TEST_NAME=${TEST_NAME/.c/}

    cat $i | awk --posix -f $AWK_PROGRAM | sed "s/TAKE_FROM_C_FILE/$TEST_NAME/g"
done

for i in `find $SRC_DIR -maxdepth 1 -name \*.xml` ; do
    TEST_NAME=`basename $i`
    TEST_NAME=${TEST_NAME/.xml/}

    cat $i | $TE_BASE/scripts/xml2dox | awk --posix -f $AWK_PROGRAM | sed "s/TAKE_FROM_C_FILE/$TEST_NAME/g"
done

echo "</tests-info>"
