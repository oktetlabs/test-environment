#!/bin/sh
#
# Clean up TE sources to build from scratch:
# 1. Delete all 'configure' scripts marked as ignored in Subversion.
#    It is sufficient to force dispatcher.sh to call aclocal, autoconf,
#    automake in all such directories.
#
# TE_BASE is required to be set. All actions are performed in this
# directory. The script uses Subversion to make sure that 'configure'
# to be removed in marked as ignored by user.
#
#
# Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
#
# @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
#
# $Id$
#

for i in `find . -name configure -or -name Makefile.in -or \
                 -name aclocal.m4 -or -name autom4te.cache` ; do
    i_status=`svn status $i` || echo "Failed to set SVN status of $i"
    # Remove only files which are ignored by Subversion
    if test "x$i_status" = "xI${i_status/#I/}" ; then
        rm -rf $i
    fi
done
