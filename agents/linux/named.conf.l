%x COMMENT
%%
include { return DNS_INCLUDE_STATEMENT; }
options { return DNS_OPTIONS_STATEMENT; }
forwarders { return DNS_FORWARDERS_STATEMENT; }
[[:digit:]]+(\.[[:digit:]])* { return DNS_DOTTED_STRING; }
[[:digit:]]+(:[[:digit:]])* { return DNS_COLON_STRING; }
yes|true|no|false { return DNS_BOOLEAN; }
[[:digit:]]+ { return DNS_NUMBER; }
[[:digit:]]+[kKmMgG] { return DNS_SCALED_NUMBER; }
[[:alpha:]][[:alnum:]]* { return DNS_WORD; }
\"[^"]*\" { return DNS_QUOTED_STRING; }
"{" { return '{'; }
"}" { return '}'; }
";" { return ';'; }
"/" { return '/'; }
"!" { return '!'; }
"#".*\n {}
"//".*\n {}
[[:space:]]+ {}
"/*" { BEGIN(COMMENT); }
<COMMENT>[^*]* {}
<COMMENT>"*"+[^*/]* {}
<COMMENT>"*"+"/" { BEGIN(INITIAL); }


