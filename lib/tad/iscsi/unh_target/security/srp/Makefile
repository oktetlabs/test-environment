#	security/srp/Makefile
#
#	vi: set autoindent tabstop=8 shiftwidth=4 :
#
#	Copyright (C) 2001-2003 InterOperability Lab (IOL)
#	University of New Hampshier (UNH)
#	Durham, NH 03824
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2, or (at your option)
#	any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
#	USA.
#
#	The name of IOL and/or UNH may not be used to endorse or promote products
#	derived from this software without specific prior written permission.
#

all:		srp.o
 
srp.o:		srp.c
	cc	 -c srp.c -Wall

.PHONY: clean

clean:
	$(RM)	*.o
