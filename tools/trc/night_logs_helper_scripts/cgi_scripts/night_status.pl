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
  </head>
  <body onload="setAutoReload()">
    <b>${last_session} ${cur_time}</b>
    <table border="1">
      <tr><th>Session</th><th>Status</th><th>Statistics</th></tr>
EOT

foreach my $log (@last_logs)
{
    my $log_name;
    my $log_txt;
    my $stat_txt = "";
    my $total_iters = 0;
    my $exp_iters = 0;
    my $unexp_iters = 0;
    my $color = "rgb(100,255,100)";
    my $status = "OK";

    $log =~ s/\s*$//;

    if (-r "${log}/trc-stats.txt")
    {
        my @trc_stats;
        my $link_path = "";

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

        $stat_txt = "Run ${total_iters}, unexpected ${unexp_iters}";
        $link_path = find_link($log);
        if (length($link_path) > 0)
        {
            $link_path =~ s/\/home\/tester-l5\/private_html\///;
            $stat_txt = "<a href=\"https://oktetlabs.ru/~tester-l5/".
                        "$link_path\">".$stat_txt."</a>";
        }
    }

    $log_txt = `cat ${log}/progress.log`;
    $log_txt =~ s/\n/; /g;

    if ($log_txt =~ /Socket tester: ([^;]*)/)
    {
        $status = $1;
    }

    if (($status eq "OK" &&
         $unexp_iters > $total_iters * 0.2) ||
        $status eq "STOPPED")
    {
        $color = "yellow";
    }
    
    if (($status eq "OK" && $unexp_iters > $total_iters * 0.8) ||
        ($status ne "OK" && $status ne "RUNNING" && $status ne "STOPPED") ||
         $log_txt =~ /Driver unload failure/)
    {
        $color = "rgb(255, 100, 100)";
    }

    $log_name = $log;
    $log_name =~ s/^.*session_[^\/]*\///;

    print "<tr style=\"background-color:$color\"><td>${log_name}</td>".
          "<td>${log_txt}</td><td>${stat_txt}</td></tr>\n";
}

my $autoreload_checked = "";
my $cgi = CGI->new;

if ($cgi->param('autoreload_enabled'))
{
    $autoreload_checked = "checked";
}

print <<EOT;
    </table>
    <input type="checkbox" id="autoreload_cb" onclick="setAutoReload()"
           ${autoreload_checked}>
      Refresh page every 30 seconds
  </body>
</html>
EOT
