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

opts=""
while test "${1#-}" != "$1" ; do
    opts="$opts $1"
    shift 1
done

if test -z "$1" -o -n "$2" ; then
    echo "USAGE: te_trc.sh [<te-trc-report options>] <raw_log_file>"
    te-trc-report --help
    exit 1
fi

raw_log_file="$1"

te-trc-log "$raw_log_file" | te-trc-report ${opts}

exit $?
