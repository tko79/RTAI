bin_PROGRAMS = genconf_io

genconf_io_SOURCES = \
	genconf_io.c

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) $(srcdir)/gen_io $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) $(srcdir)/gen_dev $(DESTDIR)$(bindir)

EXTRA_DIST = example_config gen_dev gen_io scilab-howto.pdf
