(coverage branch loop multi relational)
(logfile /usr/tmp/GCTLOG)

# These are all the files that are new for GCT or have had GCT changes
# made to them.  This includes utility programs.

(c-decl.c (options -instrument)
	grokdeclarator
	finish_struct
	store_parm_decls
	pushdecl
)
(c-parse.tab.c (options instrument-included-files)
	-hash
	-is_reserved_word
	-skip_white_space
	-readescape
	-yyerror
	-yylex
)

(cccp.c (options -instrument)
	gct_init_macro_data
	gct_print_macro_data
	gct_finish_macro_data
	main
	rescan
	output_line_command
	macroexpand
)
g-report.c
g-tools.c

(gcc.c (options -instrument)
	gct_set_style
	gct_call_remap
	gct_compile_aux
	choose_gct_temp_base
	process_command
	do_spec_1
	main
)

gct-build.c
gct-commen.c
gct-contro.c
gct-decl.c
gct-defs.c
gct-exit.c
gct-files.c
gct-lookup.c
gct-macros.c
gct-mapfil.c
gct-print.c
gct-race.c
gct-strans.c
gct-tbuild.c
gct-tcompa.c
gct-temps.c
gct-tgroup.c
gct-trans.c
gct-util.c
gct-utrans.c
gct-write.c
gedit.c
gfilter.c
gmerge.c
gnewer.c
greport.c
grestore.c
gsummary.c
(toplev.c (options -instrument)
	compile_file
	main
)
version.c
