
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(top_srcdir)
AM_LDFLAGS=@TE_LDFLAGS@ 

noinst_HEADERS=tad_iscsi_impl.h

noinst_LIBRARIES=libtad_iscsi.a

libtad_iscsi_a_SOURCES=tad_iscsi_stack.c tad_iscsi_layer.c tad_iscsi_csap.c 


install-exec-hook:
	touch -c -r libtad_iscsi.a $(DESTDIR)/$(prefix)/lib/libtad_iscsi.a

all: install

