
AUTOMAKE_OPTIONS = foreign 

if JUST_DIST

EXTRA_DIST=vxworks.c

install-exec-hook:

else

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/../@host@/include 
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib

libtavxworks_a_DEPENDENCIES=@TE_DEPENDENCIES@

lib_LIBRARIES=libtavxworks.a

libtavxworks_a_SOURCES=vxworks.c

install-exec-hook:
	if test -n "${libtavxworks_a_DEPENDENCIES}" ; \
	then \
	    rm -rf tmp ; \
	    mkdir tmp ; \
	    cd tmp ; \
	    for i in ${libtavxworks_a_DEPENDENCIES} ; do \
	        $(AR) x $$i ; \
	    done ; \
	    $(AR) rcs ${prefix}/lib/${lib_LIBRARIES} *.o ; \
	    rm -rf tmp ; \
	else \
	    touch -r ${lib_LIBRARIES} ${prefix}/lib/${lib_LIBRARIES} ; \
	fi

endif
