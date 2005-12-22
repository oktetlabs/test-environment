
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

lib_LIBRARIES = libcomm_net_agent.a

libcomm_net_agent_a_SOURCES = comm_net_agent.c

install-exec-hook:
	touch -c -r libcomm_net_agent.a \
		$(DESTDIR)/$(libdir)/libcomm_net_agent.a

all: install
