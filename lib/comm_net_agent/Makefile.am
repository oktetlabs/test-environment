
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 

lib_LIBRARIES = libcomm_net_agent.a

libcomm_net_agent_a_SOURCES = comm_net_agent.c

install-exec-hook:
	touch -c -r libcomm_net_agent.a \
		$(DESTDIR)/$(prefix)/lib/libcomm_net_agent.a

all: install
