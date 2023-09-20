# Setup Web server for logs storage

Pretty often testing logs should be shared amoung many developers.
However, human-readable representation takes a lot of disk space
especially when all formats (HTML, JSON, text) are stored for all
available test runs.

Web server (e.g. apache2) plus CGI scripts allow to store compressed
raw logs only and generate everything else upon request.

``te-logs-index.sh`` adds fake entries in addition to regular files
for files which could be generated from availabe raw logs.

``te-logs-error404.sh`` generates requested log file if it is missing.

Just copy these scripts to your CGI directory, take and look and
fix all ``FIXME`` in ``te-logs-error404.sh`` and add the following
fragment to your Apache2 configuration file (in assumption that logs
should be available under ``/logs`` location).

    ScriptAlias /cgi-bin/ /opt/te-logs/cgi-bin/
    <Directory /opt/te-logs/cgi-bin>
        AllowOverride None
        Options ExecCGI
    </Directory>

    Alias /logs /srv/logs
    <Directory /srv/logs>
        ErrorDocument 404 /cgi-bin/te-logs-error404.sh
        DirectoryIndex index.html /cgi-bin/te-logs-index.sh
    </Directory>

Note that logs should be readable to the web server and directories
with compressed raw logs should be writeable to the web server since
generated files will put nearby.

Generated files may be removed periodically (e.g. in a few days after
creation) to save disk space.


## Publishing logs

Typically logs are published on a dedicated server and structured in
some way. User should not be aware of the structure. User should
just provide meta data and raw logs in the most compact format.

Script ``publish-logs-unpack.sh`` provides a way to enfornce
recommended structure. It could be used in two ways:
 - check incoming directory for available log files anb publish it;
 - publish specified log files.

It should be wrappeed on your server in a helper script which
enforces options. The wrapper could be added to sudo rules
to be able to run it on behalf of appropriate user.

In incoming directory mode, the script removes processed files
from incoming directory. Just delete or move bad files to
specified directory to be able to process it later.

Input file is always a GNU tar archieve with two mandatory files
(``meta_data.json`` and ``raw_log_bundle.tpxz``) and few other
allowed files. See script for details.
