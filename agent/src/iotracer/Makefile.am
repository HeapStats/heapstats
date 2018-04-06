JDK_HOME = @JDK_DIR@

all:
	env JAVA_HOME=$(JDK_HOME) bash -c "$(ANT) all"
 
clean-local:
	env JAVA_HOME=$(JDK_HOME) bash -c "$(ANT) clean"
 
install-exec-local:
	$(mkdir_p) $(DESTDIR)/$(sysconfdir)/iotracer
	$(INSTALL_DATA) $(srcdir)/build/sun/misc/IoTrace.class $(DESTDIR)/$(sysconfdir)/iotracer/

uninstall-local:
	$(RM) -fR $(DESTDIR)/$(sysconfdir)/iotracer
 

