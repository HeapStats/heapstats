#!/bin/sh
set -e

pushd $(dirname $0) > /dev/null

if [[ $1 == "--clean" ]]; then
  rm -rf *class heapstats_analyze*zip *log.csv *.dat
  exit
fi

JAVA_HOME=${JAVA_HOME:-/usr/lib/jvm/java-openjdk}
CLASSPATH=""

if [[ -z "$HEAPSTATS_LIB" ]]; then
  HEAPSTATS_OPT="-agentpath:/usr/lib64/heapstats/libheapstats.so"
  MBEAN_JAR=/usr/lib64/heapstats/heapstats-mbean.jar
  MBEAN_LIB=/usr/lib64/heapstats
else
  HEAPSTATS_OPT="-agentpath:${HEAPSTATS_LIB}"
  MBEAN_JAR=${HEAPSTATS_LIB%/agent*}/mbean/java/target/heapstats-mbean.jar
  MBEAN_LIB=${HEAPSTATS_LIB%/agent*}/mbean/native
fi

if [ ! -e $MBEAN_LIB ]; then
  echo "Build/Install heapstats-mbean first."
  exit -1
fi

if [[ -x "${JAVA_HOME}/bin/java" ]] && [[ -e "$JAVA_HOME/lib/tools.jar" ]]; then
  # This testcase supports JDK8 or later
  version=$(export LANG=C;"$JAVA_HOME/bin/java" -version 2>&1 | awk -F '"' '/version/ {print $2}')
  if [[ "$version" > "1.8" ]]; then
    CLASSPATH=$JAVA_HOME/lib/tools.jar:$MBEAN_JAR:.
  else
    echo "Install JDK8 or later, and set JAVA_HOME correctly."
    exit -1
  fi
fi

# Compile testcase
$JAVA_HOME/bin/javac -classpath $CLASSPATH HeapStatsJMXClient.java JMXTest.java

# Run JMX server
$JAVA_HOME/bin/java -classpath $CLASSPATH \
                    -Djava.library.path=$MBEAN_LIB \
                    ${HEAPSTATS_OPT} \
                    -Dcom.sun.management.jmxremote=true JMXTest &
TARGET_PID=$!

echo 'Sleep 5sec...'
sleep 5

# Run JMX client
$JAVA_HOME/bin/java -classpath $CLASSPATH HeapStatsJMXClient $TARGET_PID

# Kill JMX server
kill $TARGET_PID

# Check output
md5sum heapstats_log.csv received-log.csv
md5sum test.dat received-snapshot.dat

popd > /dev/null
