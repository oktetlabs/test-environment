# SUBDIRS = unh_target

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) \
	    -I$(srcdir)/unh_target/common \
	    -I$(srcdir)/unh_target/target

AM_LDFLAGS=@TE_LDFLAGS@ 

COM_P = unh_target/common

SEC_P = unh_target/security

noinst_HEADERS=tad_iscsi_impl.h \
	       unh_target/target/my_login.h \
	       unh_target/target/scsi_target.h \
	       unh_target/target/iscsi_portal_group.h \
	       unh_target/userland_lib/my_memory.h \
	       unh_target/userland_lib/thread_communication.h \
	       $(COM_P)/list.h \
	       $(COM_P)/iscsi_common.h \
	       $(COM_P)/debug.h \
	       $(COM_P)/range.h \
	       $(COM_P)/crc.h \
	       $(COM_P)/text_param.h \
	       $(COM_P)/target_negotiate.h \
	       $(COM_P)/tcp_utilities.h \
	       $(SEC_P)/misc/misc_func.h \
	       $(SEC_P)/chap/chap.h \
	       $(SEC_P)/srp/srp.h

noinst_LIBRARIES=libtad_iscsi.a

UNH_TGT = unh_target/target/my_login.c \
	  unh_target/target/iscsi_portal_group.c



UNH_COMMON = $(COM_P)/range.c  \
	     $(COM_P)/debug.c\
	     $(COM_P)/crc.c\
	     $(COM_P)/tcp_utilities.c\
	     $(COM_P)/text_param.c\
	     $(COM_P)/default_param.c\
	     $(COM_P)/target_negotiate.c

UNH_UL = unh_target/userland_lib/my_memory.c \
	 unh_target/userland_lib/thread_communication.c


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

