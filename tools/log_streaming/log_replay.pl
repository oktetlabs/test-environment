#!/usr/bin/perl

use strict;
use warnings;

use JSON qw(encode_json decode_json);
use LWP::UserAgent;
use HTTP::Cookies;

# Set up the user agent
my $ua = LWP::UserAgent->new;
$ua->max_redirect(5);
$ua->timeout(12000);
$ua->requests_redirectable(['GET', 'HEAD', 'POST']);
$ua->cookie_jar(HTTP::Cookies->new);

# Common variables
my $trailing_slash = 0;
my $verbose = 0;
my $dry = 0;
my $adjust_ts = 0;
my $init_ts = undef;
my $ts_diff = 0;
my $run_id = '';

sub usage {
    print "$0 [options] URL logfile\n";
}

# Command-line arguments
while ($ARGV[0] =~ /^-/) {
    if ($ARGV[0] eq '--help') {
        usage;
        exit;
    } elsif ($ARGV[0] eq '--verbose') {
        $verbose = 1;
        shift;
    } elsif ($ARGV[0] eq '--dry') {
        $dry = 1;
        shift;
    } elsif ($ARGV[0] eq '--trailing-slash') {
        $trailing_slash = 1;
        shift;
    } elsif ($ARGV[0] eq '--update-timestamps') {
        $adjust_ts = 1;
        shift;
    } elsif ($ARGV[0] =~ /^--start-timestamp=(\d+)$/) {
        $adjust_ts = 1;
        $init_ts = $1;
        shift;
    } else {
        print STDERR "unexpected command-line argument: $ARGV[0]\n";
        exit;
    }
}

my $base_url = shift;
if ($base_url !~ /\/$/) {
    $base_url .= '/';
}

my $in_filename = shift;

unless (defined($base_url) and defined($in_filename)) {
    usage
}

# Additional preprocessing of event data
my %request_handler = (
    init => sub {
        if ($adjust_ts) {
            my ($body) = @_;
            $ts_diff = ($init_ts // time) - $body->{ts};
            $body->{ts} += $ts_diff;
        }
    },
    feed => sub {
        if ($adjust_ts) {
            my ($body) = @_;
            for my $event (@$body) {
                $event->{ts} += $ts_diff;
            }
        }
    },
    finish => sub {
        my ($body) = @_;
        $body->{ts} += $ts_diff;
    },
);

# How to handle server responses
my %response_handler = (
    init => sub {
        my ($status, $body) = @_;
    },
    feed => sub {},
    finish => sub {},
);

open my $file, "<", $in_filename or die "Failed to open log $in_filename";

my $type = undef;
my $current_json = '';

sub process_req {
    my ($type, $body) = @_;
    my $url = $base_url . $type;

    if ($trailing_slash) {
        $url .= '/';
    }

    print "processing $type\n";
    $request_handler{$type}->($body);
    unless ($type eq 'init') {
        $url .= "?run=$run_id";
    }

    if ($verbose) {
        print "$type\n";
        print encode_json($body), "\n";
    }

    unless ($dry) {
        my $start = time;
        my $res = $ua->post($url, 'Content-Type' => 'application/json',
                                   Content => encode_json $body);

        print $res->decoded_content, "\n";
        die $res->message if $res->is_error;

        if ($type eq 'init') {
            my $body = decode_json $res->decoded_content;
            $run_id = $body->{runid};
        }
        print "done in ", time - $start, " seconds\n";
    }
}

while (my $line = <$file>) {
    chomp $line;

    # Empty line signifies the end of an event description
    unless($line) {
        my $json = $current_json ? decode_json $current_json : undef;
        process_req(lc($type), $json);
        $type = undef;
        $current_json = '';
        next;
    }

    if(defined $type) {
        $current_json .= $line;
    } else {
        $type = $line unless defined $type;
    }
}
