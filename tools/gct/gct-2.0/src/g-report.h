/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

/* Header file for g-report.c:  tools to manipulate greport(1) output. */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/g-report.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

#include <stdio.h>
#include <string.h>
#include <errno.h>

extern FILE *Gct_input_stream;	/* Often defined in g-tools.c */
extern char *Gct_input;

void get_report_line();
void emit_report_line_id();
void emit_report_rest();
void skip_report_rest();
