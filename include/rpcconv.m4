m4_dnl GNU m4 defines platform-specific macros that collide
m4_dnl with GCC-provided ones, so just undefine them
m4_undefine(`__unix__')m4_dnl
m4_undefine(`__windows__')m4_dnl
m4_dnl
m4_dnl Use C-style comments
m4_changecom(`/*', `*/')m4_dnl
m4_dnl
m4_dnl The macros are intended to work around a limitation
m4_dnl of C preprocessor that disallows preprocessor directives
m4_dnl (including conditionals) inside a macro definition.
m4_dnl The equivalent of the following macro in pseudo-C is:
m4_dnl
m4_dnl #define RPC2H_CHECK(_name) \
m4_dnl    #ifdef _name            \
m4_dnl    case RPC_##_name: return _name; \
m4_dnl    #endif  \
m4_dnl
m4_dnl H2RPC_CHECK is analogous.
m4_define(`RPC2H_CHECK',
`
#ifdef `$1'
#line m4___line__ "m4___file__"
case RPC_`$1': return `$1';
#endif
#line m4___line__ "m4___file__"
(void)0')m4_dnl
m4_dnl
m4_define(`H2RPC_CHECK',
`
#ifdef `$1'
#line m4___line__ "m4___file__"
case `$1': return RPC_`$1';
#endif
#line m4___line__ "m4___file__"
(void)0')m4_dnl
m4_dnl
m4_dnl These macros are similar to RPC2H_CHECK() and H2RPC_CHECK(),
m4_dnl but they check whether HAVE_DECL_<name> is defined to 1,
m4_dnl not whether <name> is defined.
m4_dnl They should be used when <name> is not introduced by #define,
m4_dnl but rather is listed in a enum, and its presence is checked
m4_dnl in configure.ac or meson.build.
m4_dnl
m4_define(`RPC2H_CHECK_DECL',
`
#if HAVE_DECL_`$1'
#line m4___line__ "m4___file__"
case RPC_`$1': return `$1';
#endif
#line m4___line__ "m4___file__"
(void)0')m4_dnl
m4_dnl
m4_define(`H2RPC_CHECK_DECL',
`
#if HAVE_DECL_`$1'
#line m4___line__ "m4___file__"
case `$1': return RPC_`$1';
#endif
#line m4___line__ "m4___file__"
(void)0')m4_dnl
m4_dnl
m4_dnl RPC2H_FLAG_CHECK and H2RPC_FLAG_CHECK should
m4_dnl be used for flags conversion, like
m4_dnl
m4_dnl unsigned int res = 0;
m4_dnl RPC2H_FLAG_CHECK(res, value, FLAG1);
m4_dnl RPC2H_FLAG_CHECK(res, value, FLAG2);
m4_dnl return res;
m4_dnl
m4_dnl unsigned int res = 0;
m4_dnl H2RPC_FLAG_CHECK(res, value, FLAG1);
m4_dnl H2RPC_FLAG_CHECK(res, value, FLAG2);
m4_dnl if (value != 0)
m4_dnl     res |= RPC_FLAG_UNKNOWN;
m4_dnl return res;
m4_dnl
m4_dnl Note, they modify value, removing checked flag
m4_dnl from it, so that at the end it may be checked whether
m4_dnl some unknown flags are present.
m4_dnl
m4_define(`RPC2H_FLAG_CHECK',
`
#ifdef `$3'
#line m4___line__ "m4___file__"
`$1' |= ((`$2' & RPC_`$3') ? `$3' : 0);
`$2' &= ~RPC_`$3';
#endif
#line m4___line__ "m4___file__"
(void)0')m4_dnl
m4_dnl
m4_define(`H2RPC_FLAG_CHECK',
`
#ifdef `$3'
#line m4___line__ "m4___file__"
`$1' |= ((`$2' & `$3') ? RPC_`$3' : 0);
`$2' &= ~`$3';
#endif
#line m4___line__ "m4___file__"
(void)0')m4_dnl
m4_dnl
m4_dnl __SOURCE__ macro is provided at command line
m4_dnl to specify the name of the actual source file
#line 1 "__SOURCE__"
m4_dnl Just to be on a safe side, get rid of it
m4_undefine(`__SOURCE__')m4_dnl
