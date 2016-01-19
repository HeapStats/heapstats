ACLOCAL_AMFLAGS = -I ./m4
SUBDIRS = src attacher

all: replace_config

replace_config:
	$(SED) -e "s|@IOTRACER@|$(sysconfdir)/iotracer/IoTrace.class|" heapstats.conf > heapstats.conf.tmp
	mv -f heapstats.conf.tmp heapstats.conf

install-data-local:
	$(mkdir_p) $(DESTDIR)/$(sysconfdir)
	$(INSTALL_DATA) $(srcdir)/heapstats.conf $(DESTDIR)/$(sysconfdir)/heapstats.conf

uninstall-local:
	$(RM) $(DESTDIR)/$(sysconfdir)/heapstats.conf

clean-local:
	$(RM) $(srcdir)/src/*.gcno $(srcdir)/src/*.gcda

