use strict;
use warnings;
use Fcntl qw/:flock/;
use Storable qw/nstore_fd fd_retrieve/;
use Data::Dumper;

package night_logs_cgi_aux::aux_funcs;
use base 'Exporter';
our @EXPORT = qw(load_caches_list get_cli_path lock_caches unlock_caches
                 get_caches_list_path
                 get_compromised compromise_iter uncompromise_iter);

sub get_cli_path
{
    my $cli_path = CLI_PATH;

    return $cli_path;
}

sub get_caches_list_path
{
    return CLI_PATH."/caches_list";
}

sub load_caches_list
{
    my $caches_list_fn = get_caches_list_path();
    my @caches_list = `cat ${caches_list_fn}`;
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

sub lock_caches
{
    my $caches_list_fn = get_caches_list_path();

    open my $fh, '<', "${caches_list_fn}" or
                      die "Failed to open ${caches_list_fn}";
    flock($fh, Fcntl::LOCK_EX);

    return $fh;
}

sub unlock_caches
{
    my $fh = $_[0];

    flock($fh, Fcntl::LOCK_UN);
}

sub get_cache_by_id
{
    my $cache_id = $_[0];
    my @caches = load_caches_list();
    my $cache;

    foreach my $c (@caches)
    {
        if ($c->{"id"} eq $cache_id)
        {
            $cache = $c;
            last;
        }
    }

    return $cache;
}

sub get_compromised
{
    my $cache_path = $_[0];
    my $compromised = { "logs" => {}, "iters" => {} };

    if (!(-e $cache_path."/compromised"))
    {
        return $compromised;
    }

    open (my $fh, '<', $cache_path."/compromised") or
      (print "Cannot open $cache_path/compromised: $!" and return undef);
    flock($fh, Fcntl::LOCK_SH);
    $compromised = Storable::fd_retrieve($fh);
    flock($fh, Fcntl::LOCK_UN);
    close($fh);

    return $compromised;
}

sub proc_iter
{
    my $cache_path = $_[0];
    my $iter = $_[1];
    my $result = $_[2];
    my $logs = $_[3];
    my $compromise = $_[4];
    my $compromised;
    my $iters;

    $compromised = get_compromised($cache_path) or return -1;
    $iters = $compromised->{"iters"};

    if ($compromise > 0)
    {
        if (!defined($iters->{$iter}))
        {
            $iters->{$iter} = {};
        }
        if (!defined($iters->{$iter}->{$result}))
        {
            $iters->{$iter}->{$result} = {};
        }

        foreach my $log (@{$logs})
        {
            $iters->{$iter}->{$result}->{$log} = 1;
        }
    }
    else
    {
        if (defined($iters->{$iter}) && defined($iters->{$iter}->{$result}))
        {
            foreach my $log (@{$logs})
            {
                delete $iters->{$iter}->{$result}->{$log};
            }

            if (scalar(keys %{$iters->{$iter}->{$result}}) == 0)
            {
                delete $iters->{$iter}->{$result};
            }
            if (scalar(keys %{$iters->{$iter}}) == 0)
            {
                delete $iters->{$iter};
            }
        }
    }

    if (!(-e $cache_path."/compromised"))
    {
        system("touch ".$cache_path."/compromised");
    }

    open(my $fh, '+<', $cache_path."/compromised") or
        (print "Cannot open $cache_path/compromised: $!\n" and
         return -1);
    flock($fh, Fcntl::LOCK_EX);
    Storable::nstore($compromised, $cache_path."/compromised_tmp");
    system("mv ".$cache_path."/compromised_tmp ".
           $cache_path."/compromised");
    flock($fh, Fcntl::LOCK_UN);
    close($fh);

    return 0;
}

sub compromise_iter
{
    my $cache_path = $_[0];
    my $iter = $_[1];
    my $result = $_[2];
    my $logs = $_[3];

    return proc_iter($cache_path, $iter, $result, $logs, 1);
}

sub uncompromise_iter
{
    my $cache_path = $_[0];
    my $iter = $_[1];
    my $result = $_[2];
    my $logs = $_[3];

    return proc_iter($cache_path, $iter, $result, $logs, 0);
}
