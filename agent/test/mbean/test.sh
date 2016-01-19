#!/bin/sh

#: ${JAVA_HOME?"Need to set JAVA_HOME"}
if [[ -z "$JAVA_HOME" ]]; then
  JAVA_HOME=/usr/lib/jvm/java
fi

CURRENT_DIR=`pwd`
AGENT_HOME=$CURRENT_DIR/../../
CLASSPATH=""

if [[ -x "$JAVA_HOME/bin/java" ]] && [[ -e "$JAVA_HOME/lib/tools.jar" ]]; then
  # This testcase supports JDK8 or later
  version=$(export LANG=C;"$JAVA_HOME/bin/java" -version 2>&1 | awk -F '"' '/version/ {print $2}')
  if [[ "$version" > "1.8" ]]; then
    CLASSPATH=$JAVA_HOME/lib/tools.jar:$AGENT_HOME/../mbean/java/target/heapstats-mbean.jar:.
  fi
fi
if [[ -z "$CLASSPATH" ]]; then
  echo "Install JDK8 or later, and set JAVA_HOME."
  exit -1
fi

# Compile testcase
$JAVA_HOME/bin/javac -classpath $CLASSPATH HeapStatsJMXClient.java JMXTest.java

# Run JMX server
$JAVA_HOME/bin/java -classpath $CLASSPATH \
                    -Djava.library.path=$AGENT_HOME/../mbean/native \
                    -agentpath:$AGENT_HOME/src/libheapstats-2.0.so.3 \
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

