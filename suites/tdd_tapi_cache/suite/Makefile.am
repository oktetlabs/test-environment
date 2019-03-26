tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TS_CPPFLAGS@
AM_CFLAGS = @TS_CFLAGS@
AM_LDFLAGS = @TS_LDFLAGS@

tetest_PROGRAMS =\
tc_register \
tc_invalidate_all \
tc_invalidate \
tc_actualize_unregistered \
tc_actualize_root \
tc_actualize_sub \
tc_actualize_star \
tc_actualize_indirect \
tc_operation_get \
tc_operation_add_get \
tc_operation_del \
tc_operation_find \
prologue

MYLDADD = @TS_TE_LIBS@
MYDEPS = @TS_TE_DEPS@

tc_register_SOURCES = tc_register.c
tc_register_LDADD = $(MYLDADD)
tc_register_DEPENDENCIES = $(MYDEPS)

tc_invalidate_all_SOURCES = tc_invalidate_all.c
tc_invalidate_all_LDADD = $(MYLDADD)
tc_invalidate_all_DEPENDENCIES = $(MYDEPS)

tc_invalidate_SOURCES = tc_invalidate.c
tc_invalidate_LDADD = $(MYLDADD)
tc_invalidate_DEPENDENCIES = $(MYDEPS)

tc_actualize_unregistered_SOURCES = tc_actualize_unregistered.c
tc_actualize_unregistered_LDADD = $(MYLDADD)
tc_actualize_unregistered_DEPENDENCIES = $(MYDEPS)

tc_actualize_root_SOURCES = tc_actualize_root.c
tc_actualize_root_LDADD = $(MYLDADD)
tc_actualize_root_DEPENDENCIES = $(MYDEPS)

tc_actualize_sub_SOURCES = tc_actualize_sub.c
tc_actualize_sub_LDADD = $(MYLDADD)
tc_actualize_sub_DEPENDENCIES = $(MYDEPS)

tc_actualize_star_SOURCES = tc_actualize_star.c
tc_actualize_star_LDADD = $(MYLDADD)
tc_actualize_star_DEPENDENCIES = $(MYDEPS)

tc_actualize_indirect_SOURCES = tc_actualize_indirect.c
tc_actualize_indirect_LDADD = $(MYLDADD)
tc_actualize_indirect_DEPENDENCIES = $(MYDEPS)

tc_operation_get_SOURCES = tc_operation_get.c
tc_operation_get_LDADD = $(MYLDADD)
tc_operation_get_DEPENDENCIES = $(MYDEPS)

tc_operation_add_get_SOURCES = tc_operation_add_get.c
tc_operation_add_get_LDADD = $(MYLDADD)
tc_operation_add_get_DEPENDENCIES = $(MYDEPS)

tc_operation_del_SOURCES = tc_operation_del.c
tc_operation_del_LDADD = $(MYLDADD)
tc_operation_del_DEPENDENCIES = $(MYDEPS)

tc_operation_find_SOURCES = tc_operation_find.c
tc_operation_find_LDADD = $(MYLDADD)
tc_operation_find_DEPENDENCIES = $(MYDEPS)

prologue_SOURCES = prologue.c
prologue_LDADD = $(MYLDADD)
prologue_DEPENDENCIES = $(MYDEPS)
