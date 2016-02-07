#!/usr/bin/perl

use strict;
use warnings;

# Next comment is special, it will be changed to
# correct "use lib" when deploying.

#USE_LIB
use night_logs_cgi_aux::aux_funcs;

use CGI;
use Time::localtime;
use File::Temp qw/ tempfile /;

my $cgi = CGI->new;

sub fix_date
{
    my $date = $_[0];
    my $tm = localtime;

    if (!defined($date))
    {
        return $date;
    }

    if ($date =~ /([0-9]{2})[.]([0-9]{2})\s*$/)
    {
        return ($tm->year + 1900).".".$2.".".$1;
    }
    elsif ($date =~ /([0-9]{2})[.]([0-9]{2})[.]([0-9]{4})\s*$/)
    {
        return $3.".".$2.".".$1;
    }

    return $date;
}

sub split_str
{
    my $str = $_[0];

    if (!defined($str))
    {
        return ();
    }

    return grep(/[^\s]+/, split(/(\r\n|\n|\r)/, $str));
}

my $cli_path = get_cli_path();

my $start_date = $cgi->param('start_date');
my $end_date = $cgi->param('end_date');
my $tests = $cgi->param('tests');
my $cache = $cgi->param('cache');
my $tag_expr = $cgi->param('tag_expr');
my $log_sources = $cgi->param('log_sources');

my @tests_list = split_str($tests);
my @logs_list = split_str($log_sources);

my $command_str = "$cli_path/te_get_nlogs_results";

my $report_fn;

$start_date = fix_date($start_date);
$end_date = fix_date($end_date);

if (defined($start_date))
{
    $command_str .= " -f$start_date";
}
if (defined($end_date))
{
    $command_str .= " -t$end_date";
}

foreach my $test (@tests_list)
{
    $command_str .= " --test=$test";
}

foreach my $log (@logs_list)
{
    $command_str .= " --log-path=$log";
}

if (defined($tag_expr) && length($tag_expr) > 0)
{
    $command_str .= " --tag-expr=\"$tag_expr\"";
}

if (defined($cache) && length($cache) > 0)
{
    my @caches_list = load_caches_list();

    foreach my $cache_rec (@caches_list)
    {
        if ($cache eq $cache_rec->{"id"})
        {
            $command_str .= " --cached-logs=".$cache_rec->{"cache_path"};

            if (scalar(@logs_list) == 0)
            {
                $command_str .=
                    " --log-path=".$cache_rec->{"logs_source"};
            }
        }
    }
}

if ($cgi->param('include_compromised'))
{
    $command_str .= " --include-compromised";
}

(undef, $report_fn) = tempfile("/tmp/report-XXXX");

if (defined($report_fn) && length($report_fn) > 0)
{
    my $result;

    $command_str .= " --out-file=$report_fn";
    `pushd /tmp/ >/dev/null; $command_str 2>/dev/null 1>/dev/null ; popd >/dev/null`;

    $result = join('', `cat $report_fn`);

    `rm $report_fn`;

    print $cgi->header("text/html");
    print $result;
}

1;
