SUBDIRS = unh_target

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir) \
	    -I$(srcdir)/unh_target/common \
	    -I$(srcdir)/unh_target/target

AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=unh_target/common/target_negotiate.h \
                unh_target/security/chap/chap.h \
		unh_target/security/misc_func.h \
		unh_target/common/list.h \
		unh_target/common/iscsi_common.h \
		unh_target/common/text_param.h

noinst_HEADERS=tad_iscsi_impl.h 

noinst_LIBRARIES=libtad_iscsi.a

libtad_iscsi_a_SOURCES=tad_iscsi_stack.c tad_iscsi_layer.c \
		       tad_iscsi_csap.c 

#tad_iscsi_target.c


install-exec-hook:
	touch -c -r libtad_iscsi.a $(DESTDIR)/$(prefix)/lib/libtad_iscsi.a

install-exec-local:
	for mod in common security target; do \
	    for obj in unh_target/$$mod/*.o; do \
			$(AR) $(ARFLAGS) libtad_iscsi.a $$obj; \
		done; \
	done
	$(RANLIB) libtad_iscsi.a 	

all: install

