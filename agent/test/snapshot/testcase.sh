#!/bin/bash
set -e
pushd $(dirname $0) >/dev/null

### Usage
###   - Without Valgrind
###       ./testcase.sh /path/to/heapstats
###
###   - With Valgrind
###       VALGRIND_OPTION="--leak-check=yes --suppressions=/path/to/suppress" \
###             JAVA_OPTS="<Java options (-X, -XX, ...)" \
###                    ./testcase.sh /path/to/heapstats

if [[ $1 == "--clean" ]]; then
  rm -rf *class heapstats_log.csv heapstats_snapshot.dat heapstats_analyze*zip
  exit
fi

TARGET_HEAPSTATS=$1

if [ ! -z "$TARGET_HEAPSTATS" ]; then
  HEAPSTATS_OPT="-agentpath:${TARGET_HEAPSTATS}"
else
  HEAPSTATS_OPT="-agentpath:${HEAPSTATS_LIB:-/usr/lib64/heapstats/libheapstats.so}"
fi

${JAVA_HOME:=/usr/lib/jvm/java-openjdk/}/bin/javac Simple.java

EXEC_COMMAND="$JAVA_HOME/bin/java $JAVA_OPTS"

if [ -n "$VALGRIND_OPTION" ]; then
  EXEC_COMMAND="valgrind $VALGRIND_OPTION $EXEC_COMMAND"
fi

EXEC_COMMAND="$EXEC_COMMAND ${HEAPSTATS_OPT}"

# This testcase calls errors by OOME
set +e

# Check1: Parallel
echo "Check1-1: Parallel"
$EXEC_COMMAND -XX:+UseParallelGC -XX:-UseParallelOldGC Simple
echo "Check1-2: Parallel (-UseCOOP)"
$EXEC_COMMAND -XX:+UseParallelGC -XX:-UseParallelOldGC -XX:-UseCompressedOops Simple

# Check2: ParallelOld
echo "Check2-1: ParallelOld"
$EXEC_COMMAND -XX:+UseParallelOldGC Simple
echo "Check2-2: ParallelOld (-UseCOOP)"
$EXEC_COMMAND -XX:+UseParallelOldGC -XX:-UseCompressedOops Simple

# Check3: CMS
echo "Check3-1: CMS"
$EXEC_COMMAND -XX:+UseConcMarkSweepGC Simple
echo "Check3-2: CMS (-UseCOOP)"
$EXEC_COMMAND -XX:+UseConcMarkSweepGC -XX:-UseCompressedOops Simple
echo "Check3-3: CMS (+ExplicitGC)"
$EXEC_COMMAND -XX:+UseConcMarkSweepGC -XX:+ExplicitGCInvokesConcurrent Simple
echo "Check3-4: CMS (+ExplicitGC -UseCOOP)"
$EXEC_COMMAND -XX:+UseConcMarkSweepGC -XX:+ExplicitGCInvokesConcurrent -XX:-UseCompressedOops Simple

# Check4: G1
echo "Check4-1: G1"
$EXEC_COMMAND -XX:+UseG1GC Simple
echo "Check4-2: G1 (-UseCOOP)"
$EXEC_COMMAND -XX:+UseG1GC -XX:-UseCompressedOops Simple
echo "Check4-3: G1 (+ExplicitGC)"
$EXEC_COMMAND -XX:+UseG1GC -XX:+ExplicitGCInvokesConcurrent Simple
echo "Check4-4: G1 (+ExplicitGC -UseCOOP)"
$EXEC_COMMAND -XX:+UseG1GC -XX:+ExplicitGCInvokesConcurrent -XX:-UseCompressedOops Simple

popd >/dev/null
