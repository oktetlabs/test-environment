#!/bin/sh

RAW_LOG_FILE=$1
DB_FILE=$2

RGT_FILTER=`mktemp`
echo \
"<?xml version="1.0"?>"\
"<filters>"\
"  <entity-filter>"\
"    <!-- Exclude log from all entities -->"\
"    <exclude entity=\"\"/>"\
"  </entity-filter>"\
"</filters>" > $RGT_FILTER

XML_LOG_FILE=`mktemp`

rgt-conv -m postponed -c $RGT_FILTER -f $RAW_LOG_FILE -o $XML_LOG_FILE
te_trc -d $DB_FILE $XML_LOG_FILE

rm $XML_LOG_FILE
rm $RGT_FILTER
