.PHONY: test clean agent

ifndef HEAPSTATS_PATH
HEAPSTATS_PATH = ../../..
endif

all: agent

	$(MAKE) -C src/

	if [ ! -d test-bin/stub ]; then \
		mkdir -p test-bin/stub; \
	fi

	if [ ! -d test-bin/results ]; then \
		mkdir test-bin/results; \
	fi

	cp -f src/heapstats-test src/jvmti-load-test test-bin/
	cp -f src/stub/*.so test-bin/stub/
	cp -f src/stub/*.class test-bin/stub/

agent:

	$(MAKE) -C $(HEAPSTATS_PATH)/agent

	if [ ! -d test-bin/heapstats-engines ]; then \
		mkdir -p test-bin/heapstats-engines; \
	fi

	cp -f $(HEAPSTATS_PATH)/agent/src/heapstats-engines/libheapstats-*.so \
	                                    test-bin/heapstats-engines/

test: all
	./testcase.sh

clean:
	$(MAKE) -C src clean
	$(RM) -r heapstats-engines test-bin

