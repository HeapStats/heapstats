ACLOCAL_AMFLAGS = -I ./m4
SUBDIRS = native
MVN = @MVN@
JAVA_HOME = @JDK_DIR@

all: $(SUBDIRS)
	cd java && env JAVA_HOME=$(JAVA_HOME) bash -c "$(MVN) package"

clean-local:
	cd java && env JAVA_HOME=$(JAVA_HOME) bash -c "$(MVN) clean"

install-exec-local:
	$(mkdir_p) $(DESTDIR)/$(libdir)
	$(INSTALL_DATA) $(srcdir)/java/target/heapstats-mbean.jar $(DESTDIR)/$(libdir)

uninstall-local:
	$(RM) $(DESTDIR)/$(libdir)/heapstats-mbean.jar
