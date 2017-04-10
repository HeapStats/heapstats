#!/bin/sh

export CLASSPATH=testcase
export MAINCLASS=Test
unset JAVA_OPTS
export HEAPSTATS_CONF=heapstats-threadrecorder.conf

$JAVA_HOME/bin/javac $TEST_TARGET/testcase/Test.java
$JAVA_HOME/bin/javac $TEST_TARGET/testcase/dynload/DynLoad.java

