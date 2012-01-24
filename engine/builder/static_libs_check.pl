#!/usr/bin/perl
#
# Test Environment Builder
#
# Script to check/fix list of libraries to be linked: whether static
# libraries obtained via TE_EXT_LIBS are provided with libraries
# they depend on.
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
use Text::ParseWords;
use IPC::Open2;
use Cwd 'realpath';

my @libs = ();
my @lib_names;
my @lib_is_ext;
my @ext_lib_names;
my @ext_lib_so;
my $i;
my $lib_name;
my $te_ext_libs_path = $ENV{"TE_EXT_LIBS_PATH"};
my $cmd_out;
my $cmd_pid;
my $result = "";

# Read list of libraries both from standard input and from
# list of arguments passed to script
@libs = (@libs, parse_line('[\s]+', 1, $_)) while (<STDIN>);
@libs = (@libs, parse_line('[\s]+', 1, $_)) foreach (@ARGV);

if (defined($te_ext_libs_path))
{
    $te_ext_libs_path = realpath($te_ext_libs_path);
}
else
{
    $te_ext_libs_path = "";
}

# Implementation of binary search in array of strings

sub bin_search_aux
{
    my $arr = $_[0];
    my $val = $_[1];
    my $min = $_[2];
    my $max = $_[3];
    my $pos = int((int($min) + int($max)) / 2);
    my $l;
    my $m;

    return $pos if ($val eq @$arr[$pos]);

    if ($val gt @$arr[$pos])
    {
        $l = $pos;
        $m = $max;
    }
    else
    {
        $l = $min;
        $m = $pos;
    }

    return -1 if ($l == $m);

    if ($l + 1 == $m)
    {
        return $l if ($val eq @$arr[$l]);
        return $m if ($val eq @$arr[$m]);

        return -1;
    }

    return bin_search_aux($arr, $val, $l, $m);
}

sub bin_search
{
    my $arr = $_[0];
    my $val = $_[1];

    return bin_search_aux($arr, $val, 0, scalar @$arr);
}

if ($te_ext_libs_path ne "")
{
    # Get list of all the static libraries in folder where
    # external libraries should be saved
    $cmd_pid = open2($cmd_out, 0, "/usr/bin/find", 
                     ($te_ext_libs_path,
                      "-maxdepth", 1, "-name", "lib*.a"));

    # Fill array @ext_lib_names with names of all static
    # libraries found on the previous step
    while (<$cmd_out>)
    {
        $ext_lib_names[@ext_lib_names] = $2
            if ($_ =~ /\/?([^\s\/]*\/)*lib([^\s\/]*)\.a/);
    }

    @ext_lib_names = sort(@ext_lib_names);

    # Get list of all the shared objects in folder where
    # external libraries should be saved
    $cmd_pid = open2($cmd_out, 0, "/usr/bin/find",
                     ($te_ext_libs_path, "-name", "lib*.so",
                      "-maxdepth", 1)) or die("/usr/bin/find failed");

    # Delete from array @ext_lib_names all the names of static libraries
    # for which shared objects with the same name are found in the
    # folder
    while (<$cmd_out>)
    {
        my $pos;
        $lib_name = "";
        $lib_name = $2
            if ($_ =~ /\/?([^\s\/]*\/)*lib([^\s\/]*)\.so/);

        $pos = bin_search(\@ext_lib_names, $lib_name);
        if ($pos != -1)
        {
             splice(@ext_lib_names, $pos, 1);
        }
    }

    # Process array of libraries @lib passed to the script
    # determining whether a library is an external one
    $i = 0;
    foreach my $lib (@libs)
    {
        $lib_is_ext[$i] = 0;
        $lib_names[$i] = "";

        # If library is provided as -llibname,
        # check whether it can be found in array of
        # static libraries names
        unless (!defined($lib) || $lib eq "")
        {
            if ($lib =~ /-l(\S*)/)
            {
                $lib_names[$i] = $1;

                if (bin_search(\@ext_lib_names, $lib_names[$i]) != -1)
                {
                    $lib_is_ext[$i] = 1;
                }
            }

            # If library is provided as path to static library file -
            # check whether it is in the same folder as external static
            # libraries should be
            if ($lib =~ /(\/?([^\s\/]*\/)*)lib(\S*)\.a/)
            {
                $lib_names[$i] = $3;
                $lib_is_ext[$i] = 1 if (realpath($1) eq $te_ext_libs_path);
            }
        }
        $i++;
    }

    for ($i = 0; $i < @libs; $i++)
    {
        if ($lib_is_ext[$i] == 1)
        {
            my $deps_file = $te_ext_libs_path."/".$lib_names[$i]."_deps";
            if (-e $deps_file)
            {
                # If an external library depends on some other libraries,
                # information about these libraries and the order in which
                # they should be in linker's command line should be
                # specified in libname_deps file.

                my $in;

                open($in, "<", $deps_file) or die "Cannot open $deps_file";

                while (<$in>)
                {
                    my @dep_libs = parse_line('[\s]+', 1, $_);
                    my $n = $i;

                    # dep_libs contains list of dependencies in the right
                    # order. We ensure that libraries are presented in
                    # @libs in this order and, if no, insert them.

                    foreach my $dep_lib (@dep_libs)
                    {
                        unless (!defined($dep_lib) || $dep_lib eq "")
                        {
                            my $k;

                            for ($k = $n + 1; $k < @libs; $k++)
                            {
                                last if ($lib_names[$k] eq $dep_lib);
                            }

                            if ($k == @libs)
                            {
                                splice(@libs, $n + 1, 0, ("-l".$dep_lib));
                                splice(@lib_names, $n + 1, 0, ($dep_lib));
                                splice(@lib_is_ext, $n + 1, 0, 0);

                                if (bin_search(\@ext_lib_names,
                                               $lib_names[$n + 1]) != -1)
                                {
                                    $lib_is_ext[$n + 1] = 1;
                                }

                                $n++;
                            }
                        }
                    }
                }
            }
        }
    }
}

# Form linker command line from list of libraries
foreach my $lib (@libs)
{
    unless (!defined($lib) || $lib eq "")
    {
        $result .= $lib." ";
    }
}

print $result;
