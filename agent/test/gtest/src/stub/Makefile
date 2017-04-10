.PHONY: clean

JAVA_HOME = /usr/lib/jvm/java-openjdk
CXXFLAGS = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

TARGETS = libdummyJVMTI.so LongSleep.class


all: $(TARGETS)

libdummyJVMTI.so: dummyJVMTIEntry.o
	g++ -shared -o $@ $^

LongSleep.class:
	$(JAVA_HOME)/bin/javac LongSleep.java

.cpp.o:
	g++ -fPIC $(CXXFLAGS) -c $<

clean:
	$(RM) $(TARGETS) *.o

