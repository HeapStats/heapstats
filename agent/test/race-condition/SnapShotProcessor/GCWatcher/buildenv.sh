#!/bin/sh

export CLASSPATH=testcase
export MAINCLASS=Test
unset JAVA_OPTS
unset HEAPSTATS_CONF

$JAVA_HOME/bin/javac $TEST_TARGET/testcase/Test.java
