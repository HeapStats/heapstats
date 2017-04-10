#!/bin/sh

export CLASSPATH="$JAVA_HOME/lib/tools.jar:testcase"
export MAINCLASS=Test
unset JAVA_OPTS
unset HEAPSTATS_CONF

$JAVA_HOME/bin/javac -cp $CLASSPATH $TEST_TARGET/testcase/Test.java
$JAVA_HOME/bin/javac -cp $CLASSPATH $TEST_TARGET/testcase/dynload/DynLoad.java

