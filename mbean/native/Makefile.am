bin_PROGRAMS = libheapstats-mbean.so

libheapstats_mbean_so_SOURCES   = heapstats-mbean.c
libheapstats_mbean_so_CPPFLAGS  = -I@JDK_DIR@/include \
                                  -I@JDK_DIR@/include/linux -Wall \
                                  -Wno-strict-aliasing -fPIC
libheapstats_mbean_so_LDFLAGS   = -shared -ldl

install-binPROGRAMS:
	$(mkdir_p) $(DESTDIR)/$(libdir)
	$(INSTALL_PROGRAM) $(srcdir)/libheapstats-mbean.so $(DESTDIR)/$(libdir)

uninstall-binPROGRAMS:
	$(RM) $(DEST_DIR)/$(libdir)/libheapstats-mbean.so

