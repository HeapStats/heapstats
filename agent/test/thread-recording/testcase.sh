#!/bin/sh
set -e

pushd $(dirname $0) >/dev/null

if [[ $1 == "--clean" ]]; then
  rm -rf *class parser/*class heapstats-thread-records.htr test.txt
  exit
fi

${JAVA_HOME:=/usr/lib/jvm/java-openjdk}/bin/javac -cp . *.java parser/*java

${JAVA_HOME}/bin/java -agentpath:${HEAPSTATS_LIB:=/usr/lib64/heapstats/libheapstats.so}=./heapstats.conf Test
${JAVA_HOME}/bin/java -cp parser ThreadRecordParser heapstats-thread-records.htr

${JAVA_HOME}/bin/java -agentpath:${HEAPSTATS_LIB}=./heapstats.conf IOTest testcase.sh
${JAVA_HOME}/bin/java -cp parser ThreadRecordParser heapstats-thread-records.htr

popd > /dev/null
