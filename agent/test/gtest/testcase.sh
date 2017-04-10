#!/bin/sh
set -e

pushd $(dirname $0) >/dev/null

if [[ $1 == "--clean" ]]; then
  make clean
  exit
fi

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${JAVA_HOME:-/usr/lib/jvm/java-openjdk}/jre/lib/amd64/server"

if [[ ! -d test-bin ]]; then
  make
fi

pushd test-bin
./jvmti-load-test
./heapstats-test
popd

popd >/dev/null
