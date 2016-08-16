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
m4_dnl __SOURCE__ macro is provided at command line
m4_dnl to specify the name of the actual source file
#line 1 "__SOURCE__"
m4_dnl Just to be on a safe side, get rid of it
m4_undefine(`__SOURCE__')m4_dnl
