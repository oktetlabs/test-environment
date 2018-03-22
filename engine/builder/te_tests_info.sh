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

echo -en \
"<?xml version=\"1.0\"?>\\n"\
"<tests-info>\\n"

for i in `find $1 -maxdepth 1 -name \*.c` ; do
    OBJECTIVE=`awk --posix '
    BEGIN { put = 0; }                               \
    {                                                \
        if ($1 == "*" && $2 == "@objective")         \
        {                                            \
            for (i = 3; i <= NF; i++)                \
                printf("%s ", $i);                   \
            put = 1;                                 \
        }                                            \
        else if (put == 1 && $1 == "*" && NF == 1)   \
            put = 0;                                 \
        else if (put)                                \
        {                                            \
            for (i = 2; i <= NF; i++)                \
                printf("%s ", $i);                   \
        }                                            \
    } ' $i`
    OBJECTIVE=`echo $OBJECTIVE | sed -e "s/@a //g"`
    OBJECTIVE=`echo $OBJECTIVE | sed -e "s/@b //g"`
    OBJECTIVE=`echo $OBJECTIVE | sed -e "s/@c //g"`
    OBJECTIVE=`echo $OBJECTIVE | sed -e "s/@e //g"`
    OBJECTIVE=`echo $OBJECTIVE | sed -e "s/@p //g"`
    PAGE=`awk --posix ' /@page/ { printf(" page=\"%s\"", $3); }' $i`
    if test -n "${OBJECTIVE}" ; then
        TEST_NAME=`basename $i`
        TEST_NAME=${TEST_NAME/.c/}
        echo "  <test name=\"${TEST_NAME}\"$PAGE>"
        echo "    <objective>${OBJECTIVE}</objective>"
        echo "  </test>"
    fi
done

echo "</tests-info>"
