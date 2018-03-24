#
# automake file for Report Generator xml2html utility
# Process it with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

miscdatadir = $(datadir)/$(subdir)

dist_miscdata_DATA = style.css \
                     filter_conf.html

EXTRA_DIST = filter_conf.html.in
BUILT_SOURCES = filter_conf.html
CLEANFILES=filter_conf.html

dist-hook:
	rm -f $(distdir)/filter_conf.html

filter_conf.html: $(srcdir)/filter_conf.html.in
	cat $(srcdir)/filter_conf.html.in | \
        sed 's,\@\@DATADIR\@\@,${miscdatadir}/..,g;' > \
        filter_conf.html
