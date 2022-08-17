#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

cd ${TE_BASE}/doc/sphinx/generated || exit -1;

for F in `ls`
do
    TMP_FILE="tmp_$F"
    (echo ":orphan:"; echo; cat $F) > $TMP_FILE
    mv $TMP_FILE $F
done
