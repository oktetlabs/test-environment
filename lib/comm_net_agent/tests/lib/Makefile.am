
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              -I$(srcdir)/../include $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_LIBRARIES = libaux.a

libaux_a_SOURCES = synch.c messages.c connection.c

