#!/bin/sh

RAW_LOG_FILE=$1
DB_FILE=$2
XML_LOG_FILE=log.xml

rgt-conv -m postponed -c rgt-filter.conf -f $RAW_LOG_FILE -o $XML_LOG_FILE
te_trc -d $DB_FILE $XML_LOG_FILE
rm $XML_LOG_FILE
