#!/bin/bash

cd ${TE_BASE}/doc/sphinx/generated || exit -1;

for F in `ls`
do
    TMP_FILE="tmp_$F"
    (echo ":orphan:"; echo; cat $F) > $TMP_FILE
    mv $TMP_FILE $F
done
