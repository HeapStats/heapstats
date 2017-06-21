#!/bin/sh

export CLASSPATH=testcase
export MAINCLASS=Test
export JAVA_OPTS="-XX:-UseParallelOldGC"
unset HEAPSTATS_CONF

$JAVA_HOME/bin/javac $TEST_TARGET/testcase/Test.java
