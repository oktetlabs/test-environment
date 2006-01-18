#! /bin/sh
#
# Windows Test Agent
# Build standalone RPC server using native compiler
# Agruments: list of RPC server source files (pathnames).
# Pre-processor flags may be passed via CPPFLAGS environment variable.
#
# Copyright (C) 2006 Test Environment authors (see file AUTHORS in
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
# Author: Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
#
# $Id:  $
#

if test -z ${TE_WIN32_BUILD_HOST} ; then
    exit 0 
fi  

cat >>cl.m4 <<EOF
changequote([,])

define([INCLUDE],[#include <$1>])
EOF
  

FILES= 
for i in $* ; do 
    FILENAME=`basename ${i}` 
    FILES="${FILES} ${FILENAME}"
    ${CC} ${CPPFLAGS} -P -E ${i} -o ${FILENAME}.tmp ; 
    cat cl.m4 ${FILENAME}.tmp > ${FILENAME} ; 
    rm ${FILENAME}.tmp ; 
done
scp ${FILES} ${TE_WIN32_BUILD_HOST}:
ssh ${TE_WIN32_BUILD_HOST} cl -o tawin32tmpl_rpcserver  
    -lws2_32 -lmswsock -lwsock32 -liphlpapi ${FILES}
ssh ${TE_WIN32_BUILD_HOST} rm ${FILES}
scp ${TE_WIN32_BUILD_HOST}:tawin32tmpl_rpcserver .
ssh ${TE_WIN32_BUILD_HOST} rm tawin32tmpl_rpcserver
rm ${FILES}
rm cl.m4

exit 0
