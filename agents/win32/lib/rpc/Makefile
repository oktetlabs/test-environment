# SUN RPC Library
#
# Makefile for building XDR part of SUN RPC library for MS Windows
#
# Copyright (C) 2006 Test Environment authors (see file AUTHORS in the
# root directory of the distribution).
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
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>


CFLAGS=-I. 

CC=cl

SOURCES=\
xdr.c \
xdr_array.c \
xdr_intXX_t.c \
xdr_mem.c \
xdr_ref.c \
xdr_sizeof.c 

OBJECTS=\
xdr.o \
xdr_array.o \
xdr_intXX_t.o \
xdr_mem.o \
xdr_ref.o \
xdr_sizeof.o 

librpc.lib: $(OBJECTS)
	lib /OUT:$@ *.obj

clean:
	rm *.lib *.obj
