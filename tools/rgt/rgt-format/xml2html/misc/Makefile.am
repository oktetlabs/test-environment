#
# automake file for Report Generator xml2html utility
# Process it with automake
#
# Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
# root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA  02111-1307  USA
#
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
