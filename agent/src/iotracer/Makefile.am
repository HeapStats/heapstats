all:
	$(ANT) all
 
clean-local:
	$(ANT) clean
 
install-exec-local:
	$(mkdir_p) $(DESTDIR)/$(sysconfdir)/iotracer
	$(INSTALL_DATA) $(srcdir)/build/sun/misc/IoTrace.class $(DESTDIR)/$(sysconfdir)/iotracer/

uninstall-local:
	$(RM) -fR $(DESTDIR)/$(sysconfdir)/iotracer
 

