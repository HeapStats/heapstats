ACLOCAL_AMFLAGS = -I ./m4
SUBDIRS = agent mbean
MVN = @MVN@
JAVA_HOME = @JDK_DIR@

ANALYZER_DIR = $(srcdir)/analyzer/fx/target/heapstats-analyzer-2.0-bin/heapstats-analyzer-2.0
CLI_DIR = $(srcdir)/analyzer/cli/target/heapstats-cli-2.0-bin/heapstats-cli-2.0

.PHONY: $(SUBDIRS) analyzer


all: $(SUBDIRS) analyzer

agent:
	$(MAKE) -C $@

analyzer:
	env JAVA_HOME=$(JAVA_HOME) bash -c "$(MVN) package"

mbean:
	$(MAKE) -C $@

clean-local:
	env JAVA_HOME=$(JAVA_HOME) bash -c "$(MVN) clean"

install-exec-local:
	$(mkdir_p) $(DESTDIR)/$(libexecdir)
	$(mkdir_p) $(DESTDIR)/$(libexecdir)/lib
	$(INSTALL_DATA) $(ANALYZER_DIR)/THIRD_PARTY_README $(DESTDIR)/$(libexecdir)
	$(INSTALL_DATA) $(ANALYZER_DIR)/heapstats-analyzer.jar $(DESTDIR)/$(libexecdir)
	$(INSTALL_DATA) $(ANALYZER_DIR)/filterDefine.xsd $(DESTDIR)/$(libexecdir)
	$(INSTALL_DATA) $(ANALYZER_DIR)/heapstats.properties $(DESTDIR)/$(libexecdir)
	$(INSTALL_DATA) $(ANALYZER_DIR)/lib/heapstats-core.jar $(DESTDIR)/$(libexecdir)/lib
	$(INSTALL_DATA) $(ANALYZER_DIR)/lib/heapstats-mbean.jar $(DESTDIR)/$(libexecdir)/lib
	$(INSTALL_DATA) $(ANALYZER_DIR)/lib/jgraphx.jar $(DESTDIR)/$(libexecdir)/lib
	$(INSTALL_DATA) $(CLI_DIR)/heapstats-cli.jar $(DESTDIR)/$(libexecdir)
	$(INSTALL_PROGRAM) $(srcdir)/analyzer/cli/heapstats-cli $(DESTDIR)/$(bindir)

uninstall-local:
	rm -fR $(DESTDIR)/$(libexecdir)/lib
	$(RM) $(DESTDIR)/$(libexecdir)/THIRD_PARTY_README $(DESTDIR)/$(libexecdir)/filterDefine.xsd $(DESTDIR)/$(libexecdir)/heapstats* $(DESTDIR)/$(bindir)/heapstats-cli

