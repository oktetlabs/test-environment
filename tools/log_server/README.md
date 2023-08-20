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
