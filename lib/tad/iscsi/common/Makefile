#	common/Makefile
#
#	vi: set autoindent tabstop=4 shiftwidth=4 :
#
#	Copyright (C) 2001-2003 InterOperability Lab (IOL)
#							University of New Hampshier (UNH)
#							Durham, NH 03824
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

SECURITY_DIR = ../security

CC = cc
CFLAGS = -Wall
LDFLAGS = 

all: common_functions.o target_negotiate.o
 
target_negotiate.o:	target_negotiate.c
	$(CC)   $(CFLAGS) -c target_negotiate.c

text_param.o:	text_param.c
	$(CC)	$(CFLAGS) -c text_param.c

iscsi_common.o:	iscsi_common.c
	$(CC)	$(CFLAGS) -c iscsi_common.c 

range.o:	range.c 
	$(CC)	$(CFLAGS) -c range.c

debug.o:	debug.c
	$(CC)	$(CFLAGS) -c debug.c

crc.o:	crc.c crc.h
	$(CC)	$(CFLAGS) -c crc.c

tcp_utilities.o:	tcp_utilities.c
	$(CC)	$(CFLAGS) -c tcp_utilities.c

common_functions.o:		iscsi_common.o range.o debug.o crc.o tcp_utilities.o text_param.o 
	$(LD)	$(LDFLAGS) -r -o common_functions.o iscsi_common.o range.o debug.o crc.o tcp_utilities.o text_param.o ../userland_lib/my_memory.o ../userland_lib/thread_communication.o

.PHONY: clean

clean:
	$(RM) *.o

