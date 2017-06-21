#!/bin/sh

export CLASSPATH=testcase
export MAINCLASS=Test
export JAVA_OPTS="-XX:+UseG1GC -XX:+ExplicitGCInvokesConcurrent"
unset HEAPSTATS_CONF

$JAVA_HOME/bin/javac $TEST_TARGET/testcase/Test.java
