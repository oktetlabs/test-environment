
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS=forwarder.h send_queue.h

noinst_LIBRARIES=libtad_forw.a

libtad_forw_a_SOURCES= forw_task.c send_queue.c
