
AUTOMAKE_OPTIONS = foreign

SUBDIRS = lib

#
# XXXX: need to install here paths to include/ and lib/ directories
# from some TE variables
#
AM_CFLAGS = -g -I$(srcdir)/include -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/lib -L@abs_top_builddir@/lib \
             -L@abs_top_builddir@/..

TESTS = init01 init02 reply01 close01 \
        wait01 wait02 wait03 wait04 wait05 \
        wait07 wait08 wait09 wait10 wait11 \
        wait12 wait13 wait14 wait15 wait16 wait17 \
	sanity_close01 sanity_init01 sanity_init02 sanity_reply01 \
	sanity_reply02 sanity_wait01 sanity_wait02

check_PROGRAMS = $(TESTS)

LDADD = -laux -lcomm_net_agent

CLEANFILES = -r html latex

# installation directory for automatically generated documentation
htmldir = $(prefix)/comm_net_agent/tests/doc/html
all_htmldir = $(prefix)/comm_net_agent/tests/doc/html
docdir=$(prefix)/comm_net_agent/tests/doc
html_DATA = html/index.html
all_html_DATA = $(wildcard html/*)
doc_DATA=latex/refman.ps latex/refman.pdf

# 
# This will cause to create all the files in the html and latex 
# subdirectories.
# Generally, we don't know the names of all of the files.
#
latex/refman.ps: html/index.html
	cd latex ; $(MAKE) ps > /dev/null 

latex/refman.pdf: html/index.html
	cd latex ; $(MAKE) pdf > /dev/null 

html/index.html:
	export DOXYGEN_OUTPUT_DIRECTORY=`pwd`; \
	echo "Setting DOXYGEN_OUTPUT_DIRECTORY to $DOXYGEN_OUTPUT_DIRECTORY"; \
	cd @abs_srcdir@; \
	doxygen > /dev/null 

