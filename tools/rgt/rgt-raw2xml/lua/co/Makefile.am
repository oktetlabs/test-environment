#
# automake file for RGT raw2xml/lua/co directory
# Process with automake
#
# Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
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
# Author: Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
#

lua_codir = $(datadir)/lua/co

dist_lua_co_DATA = \
    manager.lua     \
    chunk.lua

dist_noinst_SCRIPTS = test

TESTS_ENVIRONMENT = \
    PATH="$(top_builddir)/../../lib/lua/src:$$PATH"         \
    LUA_PATH="$(builddir)/../?.lua;$(datadir)/lua/?.lua"

TESTS = test

