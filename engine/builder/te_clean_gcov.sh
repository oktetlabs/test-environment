#!/bin/bash
#
# Helper script to clean gcov files
#
# $1 build directory

build=$1

test -n "${build}" || \
    exit 1

cd "${build}" 2> /dev/null || \
    exit 1

files=$(find . -name \*.gcda -o -name \*.gcov 2> /dev/null) || \
    exit 1

echo "${files}" | \
    sudo bash -c "
        cd \"${build}\" || exit 1 ;
        while read f ; do
            test -z \"\$f\" && continue ;
            rm \"\$f\" 2> /dev/null || exit 1 ;
        done ;
        :"
