#
# automake file for RGT idx directory
# Process with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
#

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ \
              -D_FILE_OFFSET_BITS=64 $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ -Wextra $(TE_CFLAGS_VAR)
GEN_LDADD = @TE_LDFLAGS@

noinst_HEADERS = common.h

bin_PROGRAMS = rgt-idx-make rgt-idx-apply rgt-idx-sort-mem

rgt_idx_make_SOURCES = make.c
rgt_idx_make_LDADD = $(GEN_LDADD)

rgt_idx_apply_SOURCES = apply.c
rgt_idx_apply_LDADD = $(GEN_LDADD)

rgt_idx_sort_mem_SOURCES = sort-mem.c
rgt_idx_sort_mem_LDADD = $(GEN_LDADD)

check_PROGRAMS = rgt-idx-fake rgt-idx-sort-vrfy

rgt_idx_fake_SOURCES = fake.c
rgt_idx_fake_LDADD = $(GEN_LDADD)

rgt_idx_sort_vrfy_SOURCES = sort-vrfy.c
rgt_idx_sort_vrfy_LDADD = $(GEN_LDADD)

check_SCRIPTS = check-sort-mem

TESTS_ENVIRONMENT = PATH="$$PATH:$(builddir):$(srcdir)"

TESTS = check-sort-mem
EXTRA_DIST = check-sort-mem
