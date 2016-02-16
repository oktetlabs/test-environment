#!/usr/bin/perl

use strict;
use warnings;

# Next comment is special, it will be changed to
# correct "use lib" when deploying.

#USE_LIB
use night_logs_cgi_aux::aux_funcs;

use CGI;

my $form_str = join('', `cat form.html`);

my @caches = load_caches_list();
my $caches_html = "";
my $checked = "";

my $cgi = CGI->new;

my $tests = $cgi->param('tests');
my $log_path = $cgi->param('log_path');
my $selected_cache_id;

if (defined($log_path))
{
    foreach my $cache (@caches)
    {
        if ($log_path =~ /$cache->{"logs_source"}/)
        {
            $selected_cache_id = $cache->{"id"};
            last;
        }
    }
}

if (!defined($selected_cache_id) && scalar(@caches) > 0)
{
    $selected_cache_id = $caches[0]->{"id"};
}

foreach my $cache (@caches)
{
    if ($selected_cache_id eq $cache->{"id"})
    {
        $checked = " checked";
    }
    else
    {
        $checked = "";
    }
    $caches_html .= "          ".
                    "<input type=\"radio\" name=\"cache\" ".
                    "value=\"".$cache->{"id"}."\"$checked>".
                    $cache->{"id"}."<br>\n";
    $checked = "";
}
$caches_html .= "<input type=\"radio\" name=\"cache\" ".
                "value=\"none\">None<br>";

if (defined($tests))
{
    # Link to history from TRC report - go 3 month back
    $form_str =~ s/<!-- MONTHS_BACK -->/3/;
}
else
{
    # Go one month back by default
    $form_str =~ s/<!-- MONTHS_BACK -->/1/;
    $tests = "sockapi-ts/sockopts/bindtodevice_dstunreach";
}

$form_str =~ s/\s*<!-- TESTS -->\s*/$tests/;
$form_str =~ s/\s*<!-- CACHES_LIST -->\s*\n/\n$caches_html/;

print "Content-type:text/html\r\n\r\n";

print $form_str;

1;

