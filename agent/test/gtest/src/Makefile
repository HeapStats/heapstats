.PHONY: clean

TARGETS = heapstats-test jvmti-load-test
ifdef HEAPSTATS_PATH
HEAPSTATS_SRC = $(HEAPSTATS_PATH)/agent/src
else
HEAPSTATS_SRC = ../../../src
endif
JAVA_HOME = /usr/lib/jvm/java-openjdk

OBJS = test-main.o run-libjvm.o heapstats-md-test.o symbolFinder-test.o \
       jvmSockCmd-test.o fsUtil-test.o jvmInfo-test.o heapStatsEnvironment.o
DUMMYLOAD_OBJS = dummyload-main.o heapstats-test.o \
                       $(HEAPSTATS_SRC)/libheapstats_2_0_so_3-heapstats.o
HEAPSTATS_OBJS = \
      $(HEAPSTATS_SRC)/heapstats-engines/libheapstats_engine_none*.o \
      $(HEAPSTATS_SRC)/heapstats-engines/arch/x86/libheapstats_engine_none*.o \
      $(HEAPSTATS_SRC)/arch/x86/libheapstats_2_0_so_3-heapstats_md_x86.o

CXXFLAGS = -g -I$(shell gtest-config --includedir) -I$(HEAPSTATS_SRC) \
           -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

LDFLAGS = $(shell gtest-config --ldflags --libs) \
          $(shell net-snmp-config --agent-libs) \
          -ldl -lbfd -liberty \
          -L/usr/lib/jvm/java-openjdk/jre/lib/amd64/server -ljvm


all: $(TARGETS)
	$(MAKE) -C stub

heapstats-test: $(OBJS)
	g++ -o $@ $^ $(HEAPSTATS_OBJS) $(LDFLAGS) 

jvmti-load-test: $(DUMMYLOAD_OBJS)
	g++ -o $@ $^ $(LDFLAGS)

.cpp.o:
	g++ $(CXXFLAGS) -c $<

clean:
	$(MAKE) -C stub $@
	$(RM) $(TARGETS) *.o

