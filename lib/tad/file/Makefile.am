
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -I$(top_srcdir)
AM_CFLAGS = @TE_CFLAGS@ 


noinst_HEADERS=tad_file_impl.h

noinst_LIBRARIES=libtad_file.a

libtad_file_a_SOURCES=tad_file_stack.c tad_file_layer.c tad_file_csap.c 


install-exec-hook:
	touch -c -r libtad_file.a $(DESTDIR)/$(libdir)/libtad_file.a

all: install

