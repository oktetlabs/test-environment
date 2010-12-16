
ACLOCAL_AMFLAGS = -I @TE_M4@

external_scripts = @TE_TRC_EXT_SCRIPTS@

dist_bin_SCRIPTS = te-trc-key te-trc-ol

install_extra_scripts: $(external_scripts)
	for p in $(external_scripts); do \
	    echo $(INSTALL) $$p "$(DESTDIR)$(bindir)/"; \
	    $(INSTALL) $$p "$(DESTDIR)$(bindir)/"; \
	done

install-data-hook: install_extra_scripts
