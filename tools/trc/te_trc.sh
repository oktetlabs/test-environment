#!/bin/sh

RAW_LOG_FILE=$1
DB_FILE=$2
HTML_FILE=$3

if test $# -lt 2 -o $# -gt 3; then
    echo "USAGE: te_trc.sh <raw-log> <trc-db> [<html-report>]"
    exit 1
fi

RGT_FILTER=`mktemp`
echo -e \
"<?xml version=\"1.0\"?>\\n"\
"<filters>\\n"\
"<entity-filter><exclude entity=\"\"/></entity-filter>\\n"\
"</filters>\\n" > $RGT_FILTER

XML_LOG_FILE=`mktemp`

rgt-conv --no-cntrl-msg -m postponed -c $RGT_FILTER -f $RAW_LOG_FILE -o $XML_LOG_FILE
if test $? -eq 0 ; then
    if test -n "$HTML_FILE" ; then
        HTML_FILE="--html=$HTML_FILE"
    fi
    te_trc --db=$DB_FILE $HTML_FILE $XML_LOG_FILE
fi

rm $XML_LOG_FILE
rm $RGT_FILTER
