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
use File::Temp qw/ tempfile /;
use Cwd;

my @tmp_files = ();

sub escape_str
{
    my $str = $_[0];
    $str =~ s/([\\!"\$])/\\$1/g;
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

    if ($initial_name =~ m/^(.*)[.]bz2$/)
    {
        $initial_name = $1;
        (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");
        system("bzcat ".escape_str($file_name)." > ".
               $tmp_files[$#tmp_files]);
        $file_name = $tmp_files[$#tmp_files];
    }

    if ($initial_name =~ m/^(.*)[.]raw/ ||
        !($initial_name =~ m/^(.*)[.]xml/))
    {
        (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");
        system("te-trc-log ".escape_str($file_name).
               " > ".$tmp_files[$#tmp_files]);
    }
}

my $opts = "";
my $test_fake_run = "";
my $test_specified = 0;
my $rc = 0;

foreach (@ARGV)
{
    if ($_ =~ m/^--help/)
    {
        print "\n".
          "      --conf-tester=STRING    Use this if something other \n".
          "                              than tester.conf should be \n".
          "                              used.\n";
        
        $rc = system("te-trc-update --help");
        exit $rc;
    }
    elsif ($_ =~ m/^--conf-tester=(.*)$/)
    {
        $test_fake_run = $test_fake_run." --conf-tester=\"".
                         escape_str($1)."\"";
    }
    elsif ($_ =~ m/^--test-name=(.*)$/)
    {
        $test_fake_run = $test_fake_run." --tester-fake=\"".
                         escape_str($1)."\"";
        $test_fake_run = $test_fake_run." --tester-run=\"".
                         escape_str($1)."\"";
        $opts = $opts." --test-name=\"".escape_str($1)."\"";
        $test_specified = 1;
    }
    elsif ($_ =~ m/^--log=(.*)$/)
    {
        my $log_file = $1;
        my $proto;

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
        $opts = $opts." \"".escape_str($_)."\"";
    }
}

if ($test_specified > 0)
{
    (undef, $tmp_files[$#tmp_files + 1]) = tempfile("log-XXXX");
    my $fake_raw_log = getcwd()."/".$tmp_files[$#tmp_files];
    system("TE_LOG_RAW=\"".escape_str($fake_raw_log)."\" ".
           "./run.sh $test_fake_run --no-builder --tester-no-build ".
           "--no-cs --tester-no-cs 1>/dev/null --log-txt=/dev/null");
    download_prepare_log($fake_raw_log);
    $opts = $opts." --fake-log=".$tmp_files[$#tmp_files];
}

$rc = system("te-trc-update ".$opts);

if ($rc == 0)
{
    $rc = system("rm -f ".join(" ", @tmp_files));
}
elsif ($rc & 127)
{
    printf("te-trc-update died with signal %d.%s\n", ($rc & 127),
           ($rc & 128) ? " Core dumped." : "");
}
else
{
    print("te-trc-update failed\n");
}

exit $rc;
