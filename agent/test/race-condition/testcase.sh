#!/bin/sh

pushd $(dirname $0) >/dev/null

if [[ $1 == "--clean" ]]; then
  find . \( -name "*.class" -o -name "core*" -o -name "hs_err*log" -o -name "*.gdb" -o -name "*.log" -o -name command.gdb -o -name "heapstats_*" -o -name "test-failed" -o -name "test-succeeded" -o -name "tmp*" -o -name core \) -exec rm -fR {} \;
  rm -fR __pycache__
  exit
fi

declare -A DEFAULT_ULIMITS

function store_ulimits(){
  for item in `ulimit -a | sed -e 's/^.\+\(-.\))\s\+\(.\+\)$/\1,\2/g'`; do
    key=`echo $item | cut -d',' -f 1`
    value=`echo $item | cut -d',' -f 2`
    DEFAULT_ULIMITS[$key]=$value
  done
}

function restore_ulimits(){
  for key in ${!DEFAULT_ULIMITS[@]}; do
    ulimit -S $key ${DEFAULT_ULIMITS[$key]} > /dev/null 2>&1
  done
}


if [ -z "$JAVA_HOME" ]; then
  echo '$JAVA_HOME is not set.'
  exit 1
fi;

if [ -z "$HEAPSTATS_LIB" ]; then
  echo '$HEAPSTATS_LIB is not set.'
  exit 2
fi;

TESTLIST=${1:-$PWD/testlist.txt}

if [ ! -e $TESTLIST ]; then
  echo "$TESTLIST does not exist."
  exit 3
fi

ulimit -c unlimited
store_ulimits

for TEST_ENTRY in `cat $TESTLIST`; do
  for TESTDIR in `ls -d $TEST_ENTRY`; do
    echo "Run $TESTDIR"

    export TEST_TARGET=$PWD/$TESTDIR
    source $TEST_TARGET/buildenv.sh

    AGENTPATH="-agentpath:$HEAPSTATS_LIB"

    if [ -n "$HEAPSTATS_CONF" ]; then
      AGENTPATH="$AGENTPATH=$HEAPSTATS_CONF"
    fi

    cat <<EOF > $TEST_TARGET/command.gdb
set logging file result.log
set logging on
run $AGENTPATH $JAVA_OPTS $MAINCLASS >> result.log 2>&1
EOF
    # If select specified gdb, load files and symbols to exec
    if [ -n "$GDB_PATH" ] ; then
      cat <<EOF > $TEST_TARGET/loadfiles.gdb
set sysroot /
set debug-file-directory /usr/lib/debug
set directories /usr/src/debug
set use-deprecated-index-sections on
EOF
      JAVA_LIB=$(find ${JAVA_HOME%/}/ -name "java" -type f | grep -v jre)
      echo "file ${JAVA_LIB}" >> ${TEST_TARGET}/loadfiles.gdb
      JAVA_LIB=${JAVA_LIB%/bin/java}
      echo "set solib-search-path /lib64/:${JAVA_LIB}:${JAVA_LIB}*/jre/lib/amd64/:${JAVA_LIB}*/jre/lib/amd64/*/:${HEAPSTATS_LIB%/*}" >> ${TEST_TARGET}/loadfiles.gdb

      pushd $TEST_TARGET
      ${GDB_PATH}/bin/gdb -q -x loadfiles.gdb -x test.py -x command.gdb
    else
      pushd $TEST_TARGET
      gdb -q -x test.py -x command.gdb $JAVA_HOME/bin/java
    fi

    if [ $? -ne 0 ]; then
      echo "$TESTDIR failed!"
      touch test-failed
    else
      ls hs_err*.log > /dev/null 2>&1

      if [ $? -eq 0 ]; then
        echo "$TESTDIR failed! (hs_err exists)"
        touch test-failed
      else
        ls core* > /dev/null 2>&1
        if [ $? -eq 0 ]; then
          echo "$TESTDIR failed! (core exists)"
          touch test-failed
        else
          echo "$TESTDIR succeeded"
          touch test-succeeded
        fi
      fi

    fi

    restore_ulimits
    popd
  done
done

echo
echo "Test summary:"

for TESTDIR in `cat $TESTLIST`; do
  echo -n "  $TESTDIR: "

  if [ -e $TESTDIR/test-succeeded ]; then
    echo -e "\e[32msucceeded\e[m"
  elif [ -e $TESTDIR/test-failed ]; then
    echo -e "\e[31mfailed\e[m"
  else
    echo -e "\e[33munknown\e[m"
  fi

done

popd >/dev/null
