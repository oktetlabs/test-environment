dnl Test Environment
dnl
dnl Auxiliary file for preparing C module for compiling by cl
dnl
dnl Copyright (C) 2006 Test Environment authors (see file AUTHORS in the 
dnl root directory of the distribution).
dnl
dnl Test Environment is free software; you can redistribute it and/or 
dnl modify it under the terms of the GNU General Public License as 
dnl published by the Free Software Foundation; either version 2 of 
dnl the License, or (at your option) any later version.
dnl 
dnl Test Environment is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
dnl MA  02111-1307  USA
dnl
dnl Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
dnl
dnl $Id$
 
changequote([,])

define([INCLUDE],[#include <$1>])
