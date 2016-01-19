bin_PROGRAMS = libheapstats-engine-none-2.0.so

BASE_SOURCE     = libmain.cpp util.cpp symbolFinder.cpp vmStructScanner.cpp   \
                  oopUtil.cpp bitMapMarker.cpp agentThread.cpp jvmInfo.cpp    \
                  timer.cpp snapShotMain.cpp snapShotProcessor.cpp            \
                  snapShotContainer.cpp classContainer.cpp gcWatcher.cpp      \
                  logMain.cpp jvmSockCmd.cpp signalManager.cpp logManager.cpp \
                  archiveMaker.cpp cmdArchiver.cpp fsUtil.cpp                 \
                  jniZipArchiver.cpp deadlockFinder.cpp vmVariables.cpp       \
                  vmFunctions.cpp configuration.cpp overrider.cpp             \
                  threadRecorder.cpp heapstatsMBean.cpp overrideFunc.S

if USE_PCRE
  BASE_SOURCE += pcreRegex.cpp
endif

BASE_CXX_FLAGS  = -I@JDK_DIR@/include -I@JDK_DIR@/include/linux -Wall        \
                  -Wno-strict-aliasing -fPIC @VMSTRUCTS_CXX_FLAGS@           \
                  @VMSTRUCTS_CXX_FLAGS@ -DDEFAULT_CONF_DIR=\"$(sysconfdir)\"

BASE_LD_FLAGS   = -shared @NET_SNMP_LIB_FLAGS@

BASE_CCAS_FLAGS = @CCASFLAGS@

ACLOCAL_AMFLAGS = -I ../m4


if X86
  # Normal binary for x86
  libheapstats_engine_none_2_0_so_SOURCES   = $(BASE_SOURCE) \
                                              arch/x86/x86BitMapMarker.cpp
  libheapstats_engine_none_2_0_so_LDFLAGS   = $(BASE_LD_FLAGS)
  libheapstats_engine_none_2_0_so_CXXFLAGS  = $(BASE_CXX_FLAGS)
  libheapstats_engine_none_2_0_so_CCASFLAGS = $(BASE_CCAS_FLAGS)

if SSE2
    bin_PROGRAMS += libheapstats-engine-sse2-2.0.so

    libheapstats_engine_sse2_2_0_so_SOURCES   = $(BASE_SOURCE) \
                                                arch/x86/x86BitMapMarker.cpp \
                                                arch/x86/sse2/sse2BitMapMarker.cpp
    libheapstats_engine_sse2_2_0_so_LDFLAGS   = $(BASE_LD_FLAGS)
    libheapstats_engine_sse2_2_0_so_CXXFLAGS  = $(BASE_CXX_FLAGS) -msse2 -DSSE2
    libheapstats_engine_sse2_2_0_so_CCASFLAGS = $(BASE_CCAS_FLAGS) -DSSE2
endif

if SSE3
    bin_PROGRAMS += libheapstats-engine-sse3-2.0.so

    libheapstats_engine_sse3_2_0_so_SOURCES   = $(BASE_SOURCE) \
                                                arch/x86/x86BitMapMarker.cpp \
                                                arch/x86/sse2/sse2BitMapMarker.cpp
    libheapstats_engine_sse3_2_0_so_LDFLAGS   = $(BASE_LD_FLAGS)
    libheapstats_engine_sse3_2_0_so_CXXFLAGS  = $(BASE_CXX_FLAGS) -msse3 -DSSE3
    libheapstats_engine_sse3_2_0_so_CCASFLAGS = $(BASE_CCAS_FLAGS) -DSSE3
endif

if SSE4
    bin_PROGRAMS += libheapstats-engine-sse4-2.0.so

    libheapstats_engine_sse4_2_0_so_SOURCES   = $(BASE_SOURCE) \
                                                arch/x86/x86BitMapMarker.cpp \
                                                arch/x86/sse2/sse2BitMapMarker.cpp
    libheapstats_engine_sse4_2_0_so_LDFLAGS   = $(BASE_LD_FLAGS)
    libheapstats_engine_sse4_2_0_so_CXXFLAGS  = $(BASE_CXX_FLAGS) -msse4 -DSSE4
    libheapstats_engine_sse4_2_0_so_CCASFLAGS = $(BASE_CCAS_FLAGS) -DSSE4
endif

if AVX
    bin_PROGRAMS += libheapstats-engine-avx-2.0.so

    libheapstats_engine_avx_2_0_so_SOURCES   = $(BASE_SOURCE) \
                                               arch/x86/x86BitMapMarker.cpp \
                                               arch/x86/sse2/sse2BitMapMarker.cpp \
                                               arch/x86/avx/avxBitMapMarker.cpp
    libheapstats_engine_avx_2_0_so_LDFLAGS   = $(BASE_LD_FLAGS)
    libheapstats_engine_avx_2_0_so_CXXFLAGS  = $(BASE_CXX_FLAGS) -mavx -DAVX
    libheapstats_engine_avx_2_0_so_CCASFLAGS = $(BASE_CCAS_FLAGS) -DAVX
endif

endif



if ARM
  # Normal binary for ARM (target to Raspberry Pi)
  libheapstats_engine_none_2_0_so_SOURCES   = $(BASE_SOURCE) \
                                              arch/arm/armBitMapMarker.cpp
  libheapstats_engine_none_2_0_so_LDFLAGS   = $(BASE_LD_FLAGS)
  libheapstats_engine_none_2_0_so_CXXFLAGS  = $(BASE_CXX_FLAGS) \
                                              -mhard-float -mtune=arm1176jzf-s
  libheapstats_engine_none_2_0_so_CCASFLAGS = $(BASE_CCAS_FLAGS) \
                                              -mhard-float -mtune=arm1176jzf-s

  # NEON-optimized binary (target to Raspberryu Pi 2)
  bin_PROGRAMS += libheapstats-engine-neon-2.0.so
  libheapstats_engine_neon_2_0_so_SOURCES   = $(BASE_SOURCE) \
                                              arch/arm/armBitMapMarker.cpp  \
                                              arch/arm/neon/neonBitMapMarker.cpp
  libheapstats_engine_neon_2_0_so_LDFLAGS   = $(BASE_LD_FLAGS)
  libheapstats_engine_neon_2_0_so_CXXFLAGS  = $(BASE_CXX_FLAGS) -DNEON \
                                                  -mhard-float -mtune=arm7 \
                                                  -mfpu=neon
  libheapstats_engine_neon_2_0_so_CCASFLAGS = $(BASE_CCAS_FLAGS) -DNEON \
                                                  -mhard-float -mtune=arm7 \
                                                  -mfpu=neon
endif


install-binPROGRAMS:
	$(mkdir_p) $(DESTDIR)/$(libdir)/heapstats-engines
	$(INSTALL_PROGRAM) $(srcdir)/*heapstats*.so $(DESTDIR)/$(libdir)/heapstats-engines

uninstall-binPROGRAMS:
	rm -fR $(DESTDIR)/$(libdir)/heapstats-engines

