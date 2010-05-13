
AM_LDFLAGS = -rdynamic -Wl,--export-dynamic

include_HEADERS = \
	lauxlib.h \
	lua.h \
	luaconf.h \
	lualib.h

noinst_HEADERS = \
	lapi.h \
	lcode.h \
	ldebug.h \
	ldo.h \
	lfunc.h \
	lgc.h \
	llex.h \
	llimits.h \
	lmem.h \
	lobject.h \
	lopcodes.h \
	lparser.h \
	lstate.h \
	lstring.h \
	ltable.h \
	ltm.h \
	lundump.h \
	lvm.h \
	lzio.h

# Programs are not build since they are not needed currently
# bin_PROGRAMS = lua luac

lib_LTLIBRARIES = liblua.la

lua_SOURCES = lua.c
lua_LDADD = liblua.la

luac_SOURCES = luac.c print.c
luac_LDFLAGS= -static
luac_LDADD = liblua.la

liblua_la_SOURCES = \
	lapi.c \
	lauxlib.c \
	lbaselib.c \
	lcode.c \
	ldblib.c \
	ldebug.c \
	ldo.c \
	ldump.c \
	lfunc.c \
	lgc.c \
	linit.c \
	liolib.c \
	llex.c \
	lmathlib.c \
	lmem.c \
	loadlib.c \
	lobject.c \
	lopcodes.c \
	loslib.c \
	lparser.c \
	lstate.c \
	lstring.c \
	lstrlib.c \
	ltable.c \
	ltablib.c \
	ltm.c  \
	lundump.c \
	lvm.c \
	lzio.c

liblua_la_LIBADD = -lm

