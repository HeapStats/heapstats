#!/bin/bash

TARGET_HEAPSTATS=$1

if [ "x$TARGET_HEAPSTATS" = "x" ]; then
  echo "You must set HeapStats agent that you want to check."
  exit 1
fi

if [ "x$JAVA_HOME" = "x" ]; then
  JAVA_HOME=/usr/lib/jvm/java-openjdk
fi


# Check1: Parallel
echo "Check1-1: Parallel"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseParallelGC -XX:-UseParallelOldGC Simple
echo "Check1-2: Parallel (-UseCOOP)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseParallelGC -XX:-UseParallelOldGC -XX:-UseCompressedOops Simple

# Check2: ParallelOld
echo "Check2-1: ParallelOld"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseParallelOldGC Simple
echo "Check2-2: ParallelOld (-UseCOOP)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseParallelOldGC -XX:-UseCompressedOops Simple

# Check3: CMS
echo "Check3-1: CMS"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseConcMarkSweepGC Simple
echo "Check3-2: CMS (-UseCOOP)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseConcMarkSweepGC -XX:-UseCompressedOops Simple
echo "Check3-3: CMS (+ExplicitGC)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseConcMarkSweepGC -XX:+ExplicitGCInvokesConcurrent Simple
echo "Check3-4: CMS (+ExplicitGC -UseCOOP)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseConcMarkSweepGC -XX:+ExplicitGCInvokesConcurrent -XX:-UseCompressedOops Simple

# Check4: G1
echo "Check4-1: G1"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseG1GC Simple
echo "Check4-2: G1 (-UseCOOP)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseG1GC -XX:-UseCompressedOops Simple
echo "Check4-3: G1 (+ExplicitGC)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseG1GC -XX:+ExplicitGCInvokesConcurrent Simple
echo "Check4-4: G1 (+ExplicitGC -UseCOOP)"
$JAVA_HOME/bin/java -agentpath:$TARGET_HEAPSTATS $JAVA_OPTS -XX:+UseG1GC -XX:+ExplicitGCInvokesConcurrent -XX:-UseCompressedOops Simple

