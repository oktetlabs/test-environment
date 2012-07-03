#!/usr/bin/perl
#
# Test Environment: Testing Results Comparator
#
# Script to download/unzip/process raw logs and pass obtained XML logs
# to TRC Update Tool.
#
# Copyright (C) 2004-2011 Test Environment authors (see file AUTHORS
# in the root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as 
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
# 
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
# MA  02111-1307  USA
#
# Author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
#
# $Id$
#

use strict;
use warnings;
use IPC::Open2;
use File::Temp qw/ tempfile /;
use Cwd;

my @tmp_files = ();
my $log_save = "";

sub escape_str
{
    my $str = $_[0];
    $str =~ s/([\\"\$])/\\$1/g;
    return $str;
}

sub escape_file
{
    my $str = $_[0];
    $str =~ s/[\\"\$]/\\$1/g;
    $str =~ s/[&]/_and_/g;
    $str =~ s/[|]/_or_/g;
    $str =~ s/[(]/_ob_/g;
    $str =~ s/[)]/_cb_/g;
    return $str;
}

sub download_log
{
    my $file_to_load = $_[0];
    my $file_to_save = $_[1];

    if ($file_to_load =~ m/https:\/\//)
    {
        return system("curl -s -u : --negotiate ".
                      $file_to_load." -f".
                      " -o ".$file_to_save);
    }
    else
    {
        return system("scp -q ".$file_to_load.
                      " ".$file_to_save);
    }
}

sub download_prepare_log
{
    my $file_to_load = $_[0];
    (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");

    print "Download/Prepare $file_to_load\n";

    my $proto;
    my $initial_name = $file_to_load;
    my $file_name = "";

    if ($file_to_load =~ m/^(.*):/)
    {
        $proto = $1;
    }

    if (defined($proto) && length($proto) > 0)
    {
        my $rname;
        my $rc = 0;
        if ($file_to_load =~ m/^.*\/(.*)/)
        {
            $rname = $1;
        }

        if (defined($rname) && length($rname) > 0)
        {
            $rc = download_log($file_to_load, $tmp_files[$#tmp_files]);
        }
        else
        {
            $rc = download_log($file_to_load."log.xml.bz2",
                               $tmp_files[$#tmp_files]);

            if ($rc != 0)
            {
                print "Failed to fetch XML log. Downloading RAW log.\n";
                $rc = download_log($file_to_load."log.raw.bz2",
                                   $tmp_files[$#tmp_files]);

                if ($rc != 0)
                {
                    die "Failed to fetch RAW log";
                }
                else
                {
                    $initial_name = "log.raw.bz2";
                }
            }
            else
            {
                $initial_name = "log.xml.bz2";
            }
        }

        $file_name = $tmp_files[$#tmp_files];
    }
    else
    {
        $file_name = $file_to_load;
    }

    (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");
    system("cp ".escape_str($file_name)." ".$tmp_files[$#tmp_files]);
    $file_name = $tmp_files[$#tmp_files];

    if ($initial_name =~ m/^(.*)[.]bz2$/)
    {
        $initial_name = $1;
        (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");
        system("bzcat ".$file_name." > ".
               $tmp_files[$#tmp_files]);
        $file_name = $tmp_files[$#tmp_files];
    }

    if (defined($log_save) && length($log_save) > 0)
    {
        system("cp ".$file_name." ".escape_file($log_save));
    }

    if ($initial_name =~ m/^(.*)[.]raw/ ||
        !($initial_name =~ m/^(.*)[.]xml/))
    {
        (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");
        system("te-trc-log ".$file_name.
               " > ".$tmp_files[$#tmp_files]);
    }
}

my $opts = "";
my $conf_tester = "";
my $test_fake_run = "";
my $test_fake_run_aux = "";
my $last_tags = "";
my $log_save_by_tags = 0;
my $test_specified = 0;
my $log_specified = 0;
my $no_extract_paths = 0;
my $tags_by_logs = 0;
my $rc = 0;

foreach (@ARGV)
{
    if ($_ =~ /--tags-by-logs/)
    {
        $tags_by_logs = 1;
    }
}

foreach (@ARGV)
{
    if ($_ =~ m/^--help/)
    {
        print "\n".
          "      --conf-tester=STRING    Use this if something other \n".
          "                              than tester.conf should be \n".
          "                              used.\n";
        print "\n".
          "      --log-save=STRING       Use this before --log= to save \n".
          "                              a copy of (downloaded and \n".
          "                              unpacked) log\n";
        print "\n".
          "      --log-save-by-tags      Use this option to save \n".
          "                              a copy of each (downloaded and \n".
          "                              unpacked) log named by its tag \n";
        print "\n".
          "      --tags-by-logs          If --tags is unspecified, use \n".
          "                              log file name as tag name\n\n";
        #print "\n".
        #  "      --tester-run=STRING     Test path for Tester fake run\n".
        #  "                              (if specified, --test-name \n".
        #  "                               has no effect on Tester)\n\n";
       
        $rc = system("te-trc-update --help");
        exit ($rc >> 8);
    }
    elsif ($_ =~ m/^--tags-by-logs$/)
    {
        #do nothing
    }
    elsif ($_ =~ m/^--conf-tester=(.*)$/)
    {
        $conf_tester = " --conf-tester=\"".escape_str($1)."\"";
    }
    elsif ($_ =~ m/^--test-name=(.*)$/)
    {
        $test_fake_run_aux = $test_fake_run_aux." --tester-fake=\"".
                             escape_str($1)."\"";
        $test_fake_run_aux = $test_fake_run_aux." --tester-run=\"".
                             escape_str($1)."\"";
        $opts = $opts." --test-name=\"".escape_str($1)."\"";
        $test_specified = 1;
    }
    elsif ($_ =~ m/^--tester-run=(.*)$/)
    {
        $test_fake_run = $test_fake_run." --tester-fake=\"".
                         escape_str($1)."\"";
        $test_fake_run = $test_fake_run." --tester-run=\"".
                         escape_str($1)."\"";
        $test_specified = 1;
    }
    elsif ($_ =~ m/^--log-save=(.*)$/)
    {
        $log_save = $1;
    }
    elsif ($_ =~ m/^--log-save-by-tags$/)
    {
        $log_save_by_tags = 1;
    }
    elsif ($_ =~ m/^--tags=(.*)$/)
    {
        $last_tags = $1;
        $opts = $opts." \"".escape_str($_)."\"";
    }
    elsif ($_ =~ m/^--log=(.*)$/)
    {
        my $log_file = $1;
        my $proto;

        if ((!defined($last_tags) || length($last_tags) == 0) &&
            $tags_by_logs == 1)
        {
            $log_file =~ /([^\/]*)$/;
            $opts .= " --tags=".escape_file($1);
        }

        if ($log_save_by_tags == 1 &&
            (!defined($log_save) || length($log_save) == 0))
        {
            $log_save = $last_tags;
        }

        if ($log_file =~ m/^(.*?):/)
        {
            $proto = $1;
        }

        if (defined($proto) && length($proto) > 0 ||
            !($log_file =~ m/[.]xml$/))
        {
            download_prepare_log($log_file);
            $opts = $opts." --log=".$tmp_files[$#tmp_files];
        }
        else
        {
            $opts = $opts." --log=\"".escape_str($log_file)."\"";
        }

        $log_specified = 1;
        $log_save = "";
    }
    elsif ($_ =~ m/^--show-args=(.*)$/)
    {
        print $1."\n";
    }
    elsif ($_ =~ m/^(.*?)=(.*)$/)
    {
        $opts = $opts." ".$1."=\"".escape_str($2)."\"";    
    }
    else
    {
        my $opt_str = $_;
        if ($opt_str =~ m/^--log-wilds$/ ||
            $opt_str =~ m/^--print-paths$/)
        {
            $no_extract_paths = 1;
        }

        $opts = $opts." \"".escape_str($opt_str)."\"";
    }
}

if (!defined($test_fake_run) || length($test_fake_run) == 0)
{
    $test_fake_run = $test_fake_run_aux;
}

if ($test_specified == 0 && $no_extract_paths == 0 && $log_specified > 0)
{
    my $pid = open2(*RDR, undef, "te-trc-update ".$opts." --print-paths");
    my $tst_name;
    while (<RDR>)
    {
        $tst_name = $_;
        $tst_name =~ s/[\r\n]//g;
        $test_fake_run = $test_fake_run." --tester-fake=\"".
                         escape_str($tst_name)."\"";
        $test_fake_run = $test_fake_run." --tester-run=\"".
                         escape_str($tst_name)."\"";

        $opts = $opts." --test-name=\"".escape_str($tst_name)."\"";
        $test_specified = 1;
    }
}

if ($test_specified > 0)
{
    (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");
    my $fake_raw_log = getcwd()."/".$tmp_files[$#tmp_files];
    print("Fake tester run...\n");
    system("if test x\${TE_BASE} != \"x\" -a ".
           "        x\${CONFDIR} != \"x\" ; then\n".
           "TE_LOG_RAW=\"".escape_str($fake_raw_log)."\" ".
           "\${TE_BASE}/dispatcher.sh --conf-dir=\${CONFDIR} ".
           "${conf_tester} ${test_fake_run} --no-builder ".
           "--tester-no-build --no-cs --tester-no-cs --no-rcf ".
           "--tester-no-reqs 1>/dev/null --log-txt=/dev/null\n".
           "else\n".
           "TE_LOG_RAW=\"".escape_str($fake_raw_log)."\" ".
           "./run.sh ${conf_tester} ${test_fake_run} --no-builder ".
           "--tester-no-build --no-cs --tester-no-cs --no-rcf ".
           "--tester-no-reqs 1>/dev/null --log-txt=/dev/null\n".
           "fi");
    download_prepare_log($fake_raw_log);
    $opts = $opts." --fake-log=".$tmp_files[$#tmp_files];
}

$rc = system("te-trc-update ".$opts);
#(undef, $tmp_files[$#tmp_files + 1]) = tempfile("opt-XXXX");
#open(MYFILE, "> ".$tmp_files[$#tmp_files]);
#print MYFILE "te-trc-update $opts";
#close(MYFILE);
#$rc = system("te-trc-update --opts-file=".$tmp_files[$#tmp_files]);

if ($rc & 127)
{
    printf("te-trc-update died with signal %d.%s\n", ($rc & 127),
           ($rc & 128) ? " Core dumped." : "");
}
elsif ($rc != 0)
{
    print("te-trc-update failed, exit status is $rc\n");
}

my $rc_aux = system("rm -f ".join(" ", @tmp_files));

if ($rc != 0)
{
    exit ($rc >> 8);
}
else
{
    exit ($rc_aux >> 8);
}
