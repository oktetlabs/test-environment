use strict;
package night_logs_cgi_aux::aux_funcs;
use base 'Exporter';
our @EXPORT = qw(load_caches_list get_cli_path);

sub get_cli_path
{
    my $cli_path = CLI_PATH;

    return $cli_path;
}

sub load_caches_list
{
    my $caches_dir = get_cli_path();
    my @caches_list = `cat $caches_dir/caches_list`;
    my @caches = ();

    foreach my $cache (@caches_list)
    {
        if ($cache =~ /([^[:space:]]+)\s*([^[:space:]]+)\s*([^[:space:]]+)/)
        {
            push(@caches, { id => $1, logs_source => $2, cache_path => $3});
        }
    }

    return @caches;
}

