
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -I$(top_srcdir)
AM_CFLAGS = @TE_CFLAGS@


noinst_HEADERS = tad_iscsi_impl.h 

noinst_LIBRARIES = libtad_iscsi.a

libtad_iscsi_a_SOURCES = tad_iscsi_stack.c \
                         tad_iscsi_layer.c \
                         tad_iscsi_csap.c 


all: install
