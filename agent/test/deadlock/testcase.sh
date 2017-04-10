#!/bin/sh
set -e

pushd $(dirname $0) >/dev/null

if [[ $1 == "--clean" ]]; then
  rm -rf *class heapstats_log.csv heapstats_snapshot.dat result.log
  exit
fi

THREAD_NUM=${1:-11}
SLEEP_TIME=${2:-10}
LOGFILE=result.log

${JAVA_HOME:=/usr/lib/jvm/java-openjdk}/bin/javac DLSample.java

${JAVA_HOME}/bin/java -agentpath:${HEAPSTATS_LIB:-/usr/lib64/heapstats/libheapstats.so}=./heapstats.conf \
  DLSample $THREAD_NUM 1>$LOGFILE 2>&1 &
TARGET_PID=$!

sleep $SLEEP_TIME && kill -9 $TARGET_PID

if [[ "cat $LOGFILE | grep 'heapstats CRIT: ALERT(DEADLOCK): occurred deadlock. threadCount: $THREAD_NUM'" ]]; then
  echo "Detected deadlock correctly."
  exit 0
else
  echo "Could not detect deadlock or deadlock did not occure."
  exit 1
fi

popd > /dev/null
