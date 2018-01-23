HEAPSTATS_BIN_TARGET = libheapstats-2.0.so.3
bin_PROGRAMS = $(HEAPSTATS_BIN_TARGET)
SUBDIRS = heapstats-engines iotracer

ACLOCAL_AMFLAGS = -I ../m4
libheapstats_2_0_so_3_SOURCES = heapstats.cpp

libheapstats_2_0_so_3_LDFLAGS = -shared
libheapstats_2_0_so_3_LDFLAGS += -Wl,-soname,libheapstats.so -dl

libheapstats_2_0_so_3_CXXFLAGS = -I@JDK_DIR@/include -I@JDK_DIR@/include/linux
libheapstats_2_0_so_3_CXXFLAGS += -Wall -Wno-strict-aliasing -fPIC

if X86
  libheapstats_2_0_so_3_SOURCES += arch/x86/heapstats_md_x86.cpp

if SSE2
    libheapstats_2_0_so_3_CXXFLAGS += -DSSE2
endif

if SSE4
    libheapstats_2_0_so_3_CXXFLAGS += -DSSE4
endif

if AVX
    libheapstats_2_0_so_3_CXXFLAGS += -DAVX
endif

endif


if ARM
  libheapstats_2_0_so_3_SOURCES += arch/arm/heapstats_md_arm.cpp
  libheapstats_2_0_so_3_CXXFLAGS += -mhard-float
endif


all: $(bin_PROGRAMS) libheapstats.so

libheapstats.so:
	$(RM) libheapstats.so

clean-local:
	$(RM) libheapstats.so

install-binPROGRAMS:
	$(mkdir_p) $(DESTDIR)/$(libdir)
	$(INSTALL_PROGRAM) $(srcdir)/$(HEAPSTATS_BIN_TARGET) $(DESTDIR)/$(libdir)

uninstall-binPROGRAMS:
	$(RM) $(DESTDIR)/$(libdir)/*heapstats*.so*

