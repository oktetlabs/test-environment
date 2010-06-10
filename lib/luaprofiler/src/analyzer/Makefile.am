dist_noinst_SCRIPTS = summary.lua

install-data-hook:
	@INSTALL@ -d $(bindir)
	(echo "#!/usr/bin/env lua"; cat $(srcdir)/summary.lua) \
	   	> $(bindir)/luaprofiler-summary
	chmod 0755 $(bindir)/luaprofiler-summary
