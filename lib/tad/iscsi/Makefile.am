# SUBDIRS = unh_target

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) \
	    -I$(srcdir)/unh_target/common \
	    -I$(srcdir)/unh_target/target

AM_LDFLAGS=@TE_LDFLAGS@ 

noinst_HEADERS=tad_iscsi_impl.h

noinst_LIBRARIES=libtad_iscsi.a

UNH_TGT = unh_target/target/my_login.c \
	  unh_target/target/iscsi_portal_group.c


COM_P = unh_target/common

UNH_COMMON = $(COM_P)/range.c  \
	     $(COM_P)/debug.c\
	     $(COM_P)/crc.c\
	     $(COM_P)/tcp_utilities.c\
	     $(COM_P)/text_param.c\
	     $(COM_P)/default_param.c\
	     $(COM_P)/target_negotiate.c

UNH_UL = unh_target/userland_lib/my_memory.c \
	 unh_target/userland_lib/thread_communication.c

SEC_P = unh_target/security

UNH_SEC =   $(SEC_P)/chap/chap.c 	\
	    $(SEC_P)/srp/srp.c  	\
	    $(SEC_P)/misc/misc_func.c	\
	    $(SEC_P)/hash/sha1.c	\
	    $(SEC_P)/hash/md5.c		\
	    $(SEC_P)/math/bigint.c

libtad_iscsi_a_SOURCES=tad_iscsi_stack.c tad_iscsi_layer.c \
		       tad_iscsi_csap.c  $(UNH_UL) \
		       $(UNH_TGT) $(UNH_COMMON) $(UNH_SEC)


#tad_iscsi_target.c


install-exec-hook:
	touch -c -r libtad_iscsi.a $(DESTDIR)/$(prefix)/lib/libtad_iscsi.a

all: install

