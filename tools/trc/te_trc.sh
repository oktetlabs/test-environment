#!/bin/sh

OPTS=""
while test "${1#-}" != "$1" ; do
    OPTS="$OPTS $1"
    shift 1
done

if test -z "$1" -o -n "$2" ; then
    echo "USAGE: te_trc.sh <te_trc options> <raw_log_file>"
    te_trc --help
    exit 1
fi

RAW_LOG_FILE=$1

RGT_FILTER=`mktemp /tmp/tmp.XXXXXX`
echo -e \
"<?xml version=\"1.0\"?>\\n"\
"<filters>\\n"\
"<entity-filter><exclude entity=\"\"/></entity-filter>\\n"\
"</filters>\\n" > $RGT_FILTER

XML_LOG_FILE=`mktemp /tmp/tmp.XXXXXX`

rgt-conv -m postponed -c $RGT_FILTER -f $RAW_LOG_FILE \
         -o $XML_LOG_FILE
if test $? -eq 0 ; then
    te_trc ${OPTS} $XML_LOG_FILE
fi

rm $XML_LOG_FILE
rm $RGT_FILTER
