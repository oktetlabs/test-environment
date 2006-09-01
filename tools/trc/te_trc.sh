#! /bin/bash
#
# Test Environment: Testing Results Comparator
#
# Script to process TE raw log and run TRC report generator.
#
# Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
# in the root directory of the distribution).
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
# Author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
#
# $Id:$
#

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

RAW_LOG_FILE="$1"

RGT_FILTER="$(mktemp /tmp/tmp.XXXXXX)"
cat <<EOF >"$RGT_FILTER"
<?xml version="1.0"?>
<filters>
  <entity-filter>
    <exclude entity=""/>
    <include entity="">
        <user name="TRC Options"/>
        <user name="Control"/>
    </include>
  </entity-filter>
</filters>
EOF

XML_LOG_FILE=$(mktemp /tmp/tmp.XXXXXX)

rgt-conv --incomplete-log -m postponed -c "$RGT_FILTER" \
         -f "$RAW_LOG_FILE" -o "$XML_LOG_FILE"
if test $? -eq 0 ; then
    te-trc-report ${OPTS} "$XML_LOG_FILE"
    rm "$XML_LOG_FILE"
fi

rm "$RGT_FILTER"
