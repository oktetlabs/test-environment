# Process this file with configure to produce a Makefile

# Test Environment
#
# include/ Makefile
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
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>

AUTOMAKE_OPTIONS = foreign 

te_headers = te_defs.h te_errno.h te_proto.h te_stdint.h \
             comm_agent.h rcf_methods.h rcf_common.h tad_common.h \
             rcf_rpc_defs.h tapi_rpcsock_defs.h \
             te_raw_log.h logger_api.h logger_defs.h logger_int.h \
             rcf_internal.h ta_rpc_log.h te_config.h \
             te_expand.h		

include_HEADERS = te_config_internal.h $(te_headers)

EXTRA_DIST=tarpc.x.m4 rpc_unsupported.m4

all: install

install-data-hook:
	TE_BUILD=.. ; $(srcdir)/../engine/builder/te_create_inst_dirs \
                          $(DESTDIR)/$(prefix)/..
	for i in $(te_headers) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done
	touch -c -r $(srcdir)/configure.ac \
                    $(DESTDIR)/$(prefix)/include/te_config_internal.h
