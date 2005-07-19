#! /bin/sh
#
# Test Coverage Estimation
# Gather TCE dumps for all NUT images
#
#
# Copyright (C) 2005 Test Environment authors (see file AUTHORS in
# the root directory of the distribution).
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
#
# Author: Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
#
# $Id$
#

PARSER="`dirname $0`/../share/te_builder/nut.m4"
cat "$PARSER" "$1" | m4 > "${TE_TMP}/nut.conf.processed"
. "${TE_TMP}/nut.conf.processed"

for nut in $NUTS ; do
    tas="`eval echo '$NUT_'${nut}'_TCE_TA'`"
    if test -z "$tas" ; then continue; fi
    for ta in $tas ; do
        te_tce_dump "${nut}" "${ta}" "${TE_LOG_DIR}/tce_${nut}_${ta}.tar"
    done                                      
done

