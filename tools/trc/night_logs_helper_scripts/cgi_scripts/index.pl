#!/usr/bin/perl

use strict;
use warnings;

# Next comment is special, it will be changed to
# correct "use lib" when deploying.

#USE_LIB
use night_logs_cgi_aux::aux_funcs;

my $form_str = join('', `cat form.html`);

my @caches = load_caches_list();
my $caches_html = "";
my $checked = " checked";

foreach my $cache (@caches)
{
    $caches_html .= "          ".
                    "<input type=\"radio\" name=\"cache\" ".
                    "value=\"".$cache->{"id"}."\"$checked>".
                    $cache->{"id"}."<br>\n";
    $checked = "";
}

$form_str =~ s/\s*<!-- CACHES_LIST -->\s*\n/\n$caches_html/;

print "Content-type:text/html\r\n\r\n";

print $form_str;

1;

