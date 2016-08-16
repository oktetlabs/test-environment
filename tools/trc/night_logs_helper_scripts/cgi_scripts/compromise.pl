#!/usr/bin/perl

use strict;
use warnings;

# Next comment is special, it will be changed to
# correct "use lib" when deploying.

#USE_LIB
use night_logs_cgi_aux::aux_funcs;

use CGI;
use HTML::Entities;

*STDERR = *STDOUT;

print "Content-type:text/html\r\n\r\n";

print <<EOT;
<html>
<head>
</head>
<body>
EOT

my $cgi = CGI->new;

my $cache_path = $cgi->param("cache_path");
my $iter = $cgi->param("iter");
my $result = $cgi->param("result");
my $logs = $cgi->param("logs");
my $compromise = $cgi->param("compromise");
my $rc = 0;

my @logs_arr = grep(/[^\s]+/, split(/,/, $logs));

if ($compromise > 0)
{
    $rc = compromise_iter($cache_path, $iter, $result, \@logs_arr);
}
else
{
    $rc = uncompromise_iter($cache_path, $iter, $result, \@logs_arr);
}

print <<EOT;
  RC = $rc;
</body>
</html>
EOT
