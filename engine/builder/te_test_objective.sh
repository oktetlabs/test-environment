#! /bin/sh

# Test Environment
#
# Script for extracting of the test objective from .c file.
#
# Usage: te_test_objective <c file> <output file>
#
# Copyright (C) 2005 Test Environment authors (see file AUTHORS in the 
# root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as 
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
# 
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
# MA  02111-1307  USA
#
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
# 
# $Id: $


echo -en \
"<?xml version=\"1.0\"?>\\n"\
"<tests-info>\\n"

for i in `find $1 -name *.c` ; do
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
    if test -n "${OBJECTIVE}" ; then
        TEST_NAME=`basename $i`
        TEST_NAME=${TEST_NAME/.c/}
        echo "  <test name=\"${TEST_NAME}\">"
        echo "    <objective>${OBJECTIVE}</objective>"
        echo "  </test>"
    fi
done

echo "</tests-info>"
