
ACLOCAL_AMFLAGS = -I @TE_M4@

external_scripts = @TE_TRC_EXT_SCRIPTS@

dist_bin_SCRIPTS = te-trc-key te-trc-ol

install_extra_scripts: $(external_scripts)
	for p in $(external_scripts); do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  if test -f $$d$$p; then \
	    f=`echo "$$p" | sed 's|^.*/||;$(transform)'`; \
	    echo " $(dist_binSCRIPT_INSTALL) '$$d$$p' '$(DESTDIR)$(bindir)/$$f'"; \
	    $(dist_binSCRIPT_INSTALL) "$$d$$p" "$(DESTDIR)$(bindir)/$$f"; \
	  else :; fi; \
	done

install-data-hook: install_extra_scripts
