JDK_HOME = @JDK_DIR@
ANT = @ANT@
TARGET_JAR = heapstats-attacher.jar


all:
	env JAVA_HOME=$(JDK_HOME) bash -c $(ANT)

clean-local:
	$(ANT) clean

install-exec-local:
	$(mkdir_p) $(DESTDIR)/$(bindir)
	$(mkdir_p) $(DESTDIR)/$(libexecdir)
	$(INSTALL_DATA) $(srcdir)/dist/$(TARGET_JAR) $(DESTDIR)/$(libexecdir)
	$(INSTALL_PROGRAM) $(srcdir)/heapstats-attacher $(DESTDIR)/$(bindir)

uninstall-local:
	$(RM) $(DESTDIR)/$(libexecdir)/$(TARGET_JAR)
	$(RM) $(DESTDIR)/$(bindir)/heapstats-attacher

