#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $logs_path = "/home/tester-l5/private_html/night-testing/logs/";
my $links_path = "/home/tester-l5/private_html/night-testing/";
my $last_session;
my @last_logs;
my $cmd;
my $cur_time;

sub normalize_path
{
    my $path = $_[0];

    $path =~ s/\s*$//;
    $path =~ s/^\s*//;
    $path = "/$path/";
    $path =~ s/\/+/\//g;

    return $path;
}

my $cur_dir_ls;
my $cur_dir_name;

sub find_link_in_dir
{
    my $dir_ls = $_[0];
    my $path = $_[1];
    my $dir_name = $_[2];

    foreach my $line (@{$dir_ls})
    {
        if ($line =~ /([^[:space:]]+)\s*->\s*([^[:space:]]+)/)
        {
            my $link_name = $1;
            my $link_ref = $2;

            $link_ref = normalize_path($link_ref);
            if ($link_ref eq $path)
            {
                return $dir_name."/".$link_name;
            }
        }
    }

    return "";
}

sub find_link
{
    my $path = $_[0];
    my $session_dir;
    my @dir_names;
    my @ls_output;

    $path = normalize_path($path);

    if (defined($cur_dir_name))
    {
        return find_link_in_dir($cur_dir_ls, $path, $cur_dir_name);
    }

    if ($path =~ /.*(session_[^\/]*).*/)
    {
        $session_dir = $1;
    }
    else
    {
        return "";
    }

    @dir_names = `find "${links_path}" -mindepth 3 -maxdepth 3 -name "${session_dir}"`;

    foreach my $dir_name (@dir_names)
    {
        my $result;

        $dir_name = normalize_path($dir_name);
        @ls_output = `ls -l "${dir_name}"`;

        $result = find_link_in_dir(\@ls_output, $path, $dir_name);
        if (length($result) > 0)
        {
            $cur_dir_ls = \@ls_output;
            $cur_dir_name = $dir_name;
            return $result;
        }
    }

    return "";
}

sub get_ts_name {
    my ($file) = @_;
    open my $fh, '<', "$file";
    while (<$fh>) {
        return "ZF"   if /Zetaferno DIRECT API/;
        return "zf shim" if /Zetaferno Socket Shim/;
    }
    return "onload";
}

$cmd = "ls -lt ${logs_path} ".
       "| grep '\\ssession_[0-9]*[.][0-9]*[.][0-9]*\$' | head -n1";

$last_session = `$cmd`;
if ($? != 0)
{
    exit(1);
}

$last_session =~ s/^.*\ssession_/session_/;
$last_session =~ s/\s*$//;

$cur_time = localtime();

@last_logs = `find ${logs_path}/${last_session} -mindepth 2 -maxdepth 2`;

print "Content-type:text/html\r\n\r\n";
print <<EOT;
<html>
  <head>
    <title>Night testing sessions status</title>
    <script type="text/javascript">
      var timeout = false;

      function refreshPage()
      {
          if (timeout)
          {
              clearTimeout(timeout);
              timeout = false;
          }
          if (document.getElementById("autoreload_cb").checked)
          {
              var cur_location = document.location.href;

              cur_location = cur_location.replace(new RegExp(/[?].*/), "");
              cur_location = cur_location + "?autoreload_enabled=1";
              document.location.href = cur_location;
          }
      }

      function setAutoReload()
      {
          if (timeout)
          {
              clearTimeout(timeout);
              timeout = false;
          }

          if (document.getElementById("autoreload_cb").checked)
          {
              timeout = setTimeout("refreshPage()", 30000);
          }
      }
    </script>
    <style>
      td { text-align: center; padding: 1px }
      th { background-color:#DDDDDD; }
      span.stat { font-family: monospace; white-space: pre; }
    </style>
  </head>
  <body onload="setAutoReload()">
    <b>${last_session} ${cur_time}</b>
    <table>
      <tr><th>Tags</th><th>Session</th><th>Driver load</th><th>Status</th>.
            <th>Driver unload</th><th>Run</th><th>Unexpected</th></tr>
EOT

foreach my $log (@last_logs)
{
    my $log_name;
    my $log_txt;
    my $driver_load;
    my $driver_unload;
    my $run_stat_txt = "";
    my $unex_stat_txt = "";
    my $total_iters = 0;
    my $exp_iters = 0;
    my $unexp_iters = 0;
    # Backgroud colour for status
    my $color = "#aaddaa";
    # Foreground colour for suite
    my $color_suite = "#000000";
    my $status;
    my $link_path = "";
    my $tag_name;
    my $suite_name;

    $log =~ s/\s*$//;
    $tag_name = get_ts_name("$log/progress.log");
    if ($tag_name ne "onload")
    {
        $color_suite = "#0000ff";
    }

    if (-r "${log}/trc-stats.txt")
    {
        my @trc_stats;

        @trc_stats = `cat "${log}/trc-stats.txt"`;

        foreach my $line (@trc_stats)
        {
            if ($line =~ /^\s*Run \(total\)\s*([0-9]+)\s*$/)
            {
                $total_iters = 0 + $1;
            }
            elsif ($line =~ /^\s*Passed, as expected\s*([0-9]+)\s*$/ ||
                   $line =~ /^\s*Failed, as expected\s*([0-9]+)\s*$/)
            {
                $exp_iters += 0 + $1;
            }
        }

        $unexp_iters = $total_iters - $exp_iters;

        $run_stat_txt = sprintf "<span class=\"stat\">%5d</span>",
            $total_iters;
        $unex_stat_txt = sprintf "<span class=\"stat\">%4d</span>",
            $unexp_iters;
    }

    $log_txt = `cat ${log}/progress.log`;
    $log_txt =~ s/\n/;/g;

    ($suite_name, $driver_load, $status, $driver_unload) = split /;/,
    $log_txt;
    if ($driver_load =~ /Driver loaded/ )
    {
        $driver_load = "OK";
    }
    elsif ($driver_load =~ /Driver reused/ )
    {
        $driver_load = "REUSE";
    }
    else
    {
        $driver_load = "--";
        $status = "NOT STARTED";
    }

    if ($driver_unload =~ /Driver unload failer/ )
    {
        $driver_unload = "FAILURE";
    }
    elsif ($driver_unload =~ /Driver unload success/ )
    {
        $driver_unload = "OK";
    }
    else
    {
        $driver_unload = "--";
    }

    if (($status eq "OK" &&
         $unexp_iters > $total_iters * 0.2) ||
        $status eq "RUNNING" ||
        $status eq "STOPPED")
    {
        $color = "#ddddaa";
    }

    if (($status eq "OK" && $unexp_iters > $total_iters * 0.8) ||
        ($status ne "OK" && $status ne "RUNNING" && $status ne "STOPPED") ||
         $log_txt =~ /Driver unload failure/)
    {
        $color = "#f08080";
    }

    $log_name = $log;
    $log_name =~ s/^.*session_[^\/]*\///;

    $link_path = $log;
    $link_path =~ s/\/home\/tester-l5\/private_html\///;
    $log_name = "<a href=\"https://oktetlabs.ru/~tester-l5/".
                "$link_path\">".$log_name."</a>";

    print "<tr style=\"background-color:$color\">".
          "<td style=\"background-color:#FFFFFF;".
          "color:$color_suite\">${tag_name}</td>".
          "<td>${log_name}</td><td>${driver_load}</td>".
          "<td>${status}</td><td>${driver_unload}</td>".
          "<td>${run_stat_txt}</td>".
          "<td>${unex_stat_txt}</td></tr>\n";
}

my $autoreload_checked = "checked";
my $cgi = CGI->new;

if (defined($cgi->param('autoreload_enabled')))
{
    if ($cgi->param('autoreload_enabled'))
    {
        $autoreload_checked = "checked";
    }
    else
    {
        $autoreload_checked = "";
    }
}

print <<EOT;
    </table>
    <input type="checkbox" id="autoreload_cb" onclick="setAutoReload()"
           ${autoreload_checked}>
      Refresh page every 30 seconds
  </body>
</html>
EOT
