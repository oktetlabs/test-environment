#!/bin/sh
#
# Shell wrapper to generate symbol table.
#
# Copyright (C) 2018 OKTET Labs. All rights reserved.

MYDIR="$(dirname "$0")"

NM="$1" ; shift

${NM} --format=sysv "$@" |
    awk --posix -vTABLE_NAME=generated_table -f "${MYDIR}"/te_generate_symtbl
