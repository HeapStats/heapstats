#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([HeapStats], [2.2.trunk], [heapstats@icedtea.classpath.org])
m4_include([m4/compiler-opto])
AC_DEFINE([HEAPSTATS_MAJOR_VERSION], ["2.2"])
AC_CONFIG_SRCDIR([./agent/src])
AC_CONFIG_AUX_DIR([./m4])
AC_CONFIG_MACRO_DIR([./m4])
AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE
AC_CACHE_SAVE
ifdef([AC_DISABLE_OPTION_CHECKING], [AC_DISABLE_OPTION_CHECKING])
AC_CANONICAL_HOST

LDFLAGS+=" -Wl,-z,noexecstack"

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXXCPP
AM_PROG_AS
AC_PROG_AWK
AC_PROG_EGREP
AC_PROG_INSTALL

AC_PATH_PROG([ECHO], [echo], [echo], [$PATH])

AC_PATH_PROG([SED], [sed], [], [$PATH])
if test -z "$SED"; then
  AC_MSG_ERROR([sed not found.])
fi

AC_PATH_PROG([NM], [nm], [], [$PATH])
if test -z "$NM"; then
  AC_MSG_ERROR([nm not found.])
fi

AC_PATH_PROG([ANT], [ant], [], [$PATH])
if test -z "$ANT"; then
  AC_MSG_ERROR([ant not found.])
fi

AC_PATH_PROG([MVN], [mvn], [], [$PATH])
if test -z "$MVN"; then
  AC_MSG_ERROR([mvn not found.])
fi

AC_CACHE_SAVE

# Checks for libraries.
AC_CHECK_LIB([iberty], [main], [], [])
AC_CHECK_LIB([bfd], [main], [],
  [AC_MSG_ERROR([BFD library was not found.])])
AC_CHECK_LIB([pthread], [main], [],
  [AC_MSG_ERROR([Posix thread library was not found.])])
AC_CACHE_SAVE

# Checks for common header files.
AC_HEADER_STDC     # stdlib.h stdarg.h string.h float.h
AC_HEADER_DIRENT   # dirent.h
AC_HEADER_STAT     # sys/stat.h
AC_HEADER_SYS_WAIT # sys/wait.h
AC_HEADER_TIME     # time.h sys/time.h
AC_CHECK_HEADERS([ \
  unistd.h sys/socket.h sys/types.h sys/un.h \
  sys/utsname.h sys/wait.h sys/syscall.h sys/mman.h \
  stddef.h stdint.h stdio.h fcntl.h link.h \
  limits.h errno.h signal.h execinfo.h \
  gnu/libc-version.h pthread.h \
  ], [], [AC_MSG_ERROR([Not found common header files.])])
AC_CHECK_HEADERS([sys/auxv.h], [], [])  # Supplemental headers
AC_CACHE_SAVE

# Check C++11 support
AC_LANG_PUSH([C++])
CXXFLAGS+=" -std=c++11"
AC_MSG_CHECKING([C++11 support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], [], [ AC_MSG_ERROR([Compiler does not support C++11.]) ])
AC_LANG_POP([C++])

AC_CHECK_HEADERS([sys/auxv.h], [], [])  # Supplemental headers

# Checks for library header files.
AC_CHECK_HEADERS([net-snmp/net-snmp-config.h],
  [], [AC_MSG_ERROR([Header files of Net-SNMP were not found.])])
AC_CHECK_HEADERS([net-snmp/net-snmp-includes.h],
  [], [AC_MSG_ERROR([Header files of Net-SNMP were not found.])],
  [#include <net-snmp/net-snmp-config.h>])
AC_CHECK_HEADERS([bfd.h],
  [], [AC_MSG_ERROR([Header files of binutils were not found.])])
AC_CACHE_SAVE

# Checks for compiler characteristics.
AC_C_BIGENDIAN([], [],
  [AC_MSG_ERROR([Unknown byteorder.])])
AC_CACHE_SAVE

# Checks for typedefs, structures.
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_CHECK_TYPES([ptrdiff_t])
AC_CACHE_SAVE

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_FORK
AC_FUNC_STAT
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([ syscall dl_iterate_phdr \
  localtime_r strdup realpath unlink truncate getline \
  madvise posix_fadvise munmap \
  gnu_get_libc_version gnu_get_libc_release ], [], \
  [AC_MSG_ERROR([Function was not found that need to test feature.])])
AC_CACHE_SAVE

# set test macro state
AC_LANG([C++])
AC_REQUIRE_CPP

# set general file path
DEVNULL="/dev/null"

# check processor type ---------------------------------------------------------
PROCESSOR_TYPE=""
ARCH=""

case "$host_cpu" in
  *86*)
    if test x"$host_cpu" = "xi686"; then
      ARCH="$host_cpu"
    fi
    AC_DEFINE(PROCESSOR_ARCH, X86)
    AC_DEFINE(X86, 1)
    PROCESSOR_TYPE=x86
    ;;
  *arm*)
    AC_DEFINE(PROCESSOR_ARCH, ARM)
    AC_DEFINE(ARM, 2)
    PROCESSOR_TYPE=arm
    ;;
  *)
    AC_MSG_ERROR(HeapStats agent does not support $host_cpu processor.)
    ;;
esac

AM_CONDITIONAL(X86, test x"$PROCESSOR_TYPE" = "xx86")
AM_CONDITIONAL(ARM, test x"$PROCESSOR_TYPE" = "xarm")

AC_ARG_WITH(arch,
  [AC_HELP_STRING([--with-arch@<:@=ARG@:>@],
  [Specify an architecture of the system.])],
  [ARCH="$withval"],)

if test -n "$ARCH"; then
  CXXFLAGS+=" -march=$ARCH";
  CCASFLAGS+=" -march=$ARCH"
elif test x"$PROCESSOR_TYPE" = "xarm"; then  # Raspberry Pi 2 support
  CXXFLAGS+=" -march=armv7-a"
  CCASFLAGS+=" -march=armv7-a"
fi

# check TBB --------------------------------------------------------------------
AC_CHECK_HEADERS([ \
  tbb/concurrent_hash_map.h tbb/concurrent_queue.h \
  ], [], [AC_MSG_ERROR([Not found common header files.])])
AC_CHECK_LIB([tbb], [main], [], [AC_MSG_ERROR([TBB library was not found.])])

# end processor type -----------------------------------------------------------

# get JDK path -----------------------------------------------------------------

AC_ARG_WITH([jdk], AC_HELP_STRING([--with-jdk@<:@=PATH@:>@],
  [Path to JDK home directory. (default: follow symbolic links from /etc/alternatives/java)]))

AC_MSG_CHECKING([for JAVA_HOME using configure arguments and environment (\$JAVA_HOME)])
if test -n "$with_jdk"; then
  JDK_DIR="$with_jdk"
  AC_MSG_RESULT($JDK_DIR)
elif test -n "$JAVA_HOME"; then
  JDK_DIR="$JAVA_HOME"
  AC_MSG_RESULT($JDK_DIR)
else
  AC_MSG_RESULT(none)
  AC_PATH_PROG([JAVA_PATH], [java], [], [/etc/alternatives])
  if test -n "$JAVA_PATH"; then
    # Follow a chain of symbolic link by readlink to get the real JDK path
    AC_PATH_PROG([READLINK], [readlink], [], [$PATH])
    if test -z "$READLINK"; then
      AC_MSG_ERROR([readlink not found.])
    else
      AC_MSG_CHECKING([for JAVA_HOME using java in /etc/alternatives])
      # We have to get JDK directory, not JRE directory
      JDK_DIR=`$READLINK -f "$JAVA_PATH" 2>$DEVNULL | $SED -e 's:/\(jre/\|\)bin/java::g' 2>$DEVNULL`
      if test $? -ne 0 ; then
        JDK_DIR=""
        AC_MSG_RESULT([not found])
      else
        AC_MSG_RESULT([$JDK_DIR])
      fi
    fi
  else
    # Get JDK path from openjdk default directory
    AC_MSG_CHECKING([for JAVA_HOME using well-known locations (/usr/lib/jvm/java-openjdk)])
    DEFAULT_JDK_PATH="/usr/lib/jvm/java-openjdk"
    if test -d "$DEFAULT_JDK_PATH" && test -r "$DEFAULT_JDK_PATH" ; then
      JDK_DIR="$DEFAULT_JDK_PATH"
      AC_MSG_RESULT([exist])
    fi
  fi
fi

AC_MSG_CHECKING([for JDK library using found JAVA_HOME])
if test -z "$JDK_DIR" || test ! -d $JDK_DIR || test ! -r $JDK_DIR \
  || test ! -d $JDK_DIR/bin/ || test ! -x $JDK_DIR/bin/javac ; then

  AC_MSG_RESULT([not found])
  AC_MSG_NOTICE([
    [Do you put java at non default directory ?]
    [Please expect path to "with-jdk" option, if you put java to such path.]])
  AC_MSG_ERROR([JDK directory (JAVA_HOME) was not found.])
fi

AC_MSG_RESULT([found at $JDK_DIR])
AC_SUBST([JDK_DIR])

# end JDK path -----------------------------------------------------------------

# VMStructs --------------------------------------------------------------------

AC_ARG_ENABLE([vmstructs], AC_HELP_STRING([--enable-vmstructs@<:@=yes/no@:>@],
  [Use symbol of VMStructs in libjvm.so . @<:@default=no@:>@]))

VMSTRUCTS_CXX_FLAGS=""
VMSTRUCTS_LIB_FLAGS=""
JVM_LIB_DIR=""
VMSTRUCTS_SYMBOL="gHotSpotVMStructs"
if test "$enable_vmstructs" = "yes" ; then

  AC_MSG_CHECKING([for of VMStructs in jvm shared-library])

  for archPath in i386 amd64
  do
    if test -d $JDK_DIR/jre/lib/$archPath/ ; then
      JVM_LIB_DIR=`$ECHO $JDK_DIR/jre/lib/$archPath 2>$DEVNULL`
      break
    elif test -d $JDK_DIR/lib/$archPath/ ; then
      # JDK9 does not have jre directory
      JVM_LIB_DIR=`$ECHO $JDK_DIR/lib/$archPath 2>$DEVNULL`
      break
    fi
  done
  
  if test -z "$JVM_LIB_DIR"; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([JDK shared-library arch directory was not found.])
  fi

  if test -d $JVM_LIB_DIR/server/ ; then
    JVM_LIB_DIR+="/server"
  elif test -d $JVM_LIB_DIR/client/ ; then
    JVM_LIB_DIR+="/client"
  else
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([JDK shared-library directory was not found.])
  fi

  RESULT_SYMBOL=`$NM -D $JVM_LIB_DIR/libjvm.so 2>$DEVNULL | $EGREP $VMSTRUCTS_SYMBOL 2>$DEVNULL`
  if test $? -ne 0 || test -z "$RESULT_SYMBOL"; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Symbol of VMStructs was not found in libjvm.so .])
  fi

  VMSTRUCTS_CXX_FLAGS="-DUSE_VMSTRUCTS"
  AC_MSG_RESULT([yes])
fi

AC_SUBST([VMSTRUCTS_CXX_FLAGS])

# end VMStructs ----------------------------------------------------------------

# get default NET-SNMP client library path  ------------------------------------
AC_PATH_PROGS([NET_SNMP_CFG_PATH], [net-snmp-config], [/bin:/sbin/:/usr/bin])
LIBNETSNMP_PATH=`$NET_SNMP_CFG_PATH --netsnmp-libs | sed -e 's|^-L\(.\+\) -l\(.\+\)$|\1/lib\2.so|'`

if test $? -ne 0 ; then
  AC_MSG_ERROR([Could not get path to libnetsnmp.so .])
fi

AC_SUBST([LIBNETSNMP_PATH])
AC_DEFINE_UNQUOTED([LIBNETSNMP_PATH], "$LIBNETSNMP_PATH")
# end default NET-SNMP client library path  ------------------------------------

# CPU optimization (x86 only) --------------------------------------------------
if test x"$PROCESSOR_TYPE" = "xx86"; then
  CHECK_COMPILER_OPTO([SSE2], [sse2])
  AM_CONDITIONAL(SSE2, test $enable_sse2 = yes)
  CHECK_COMPILER_OPTO([SSE4], [sse4])
  AM_CONDITIONAL(SSE4, test $enable_sse4 = yes)
  CHECK_COMPILER_OPTO([AVX], [avx])
  AM_CONDITIONAL(AVX, test $enable_avx = yes)
else
  AM_CONDITIONAL(SSE2, false)
  AM_CONDITIONAL(SSE4, false)
  AM_CONDITIONAL(AVX, false)
fi
# end CPU optimization ---------------------------------------------------------


# set debug --------------------------------------------------------------------

AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug@<:@=yes/no@:>@],
  [Enable debug compile flags. @<:@default=yes@:>@]),
  [], [enable_debug="yes"])

COMPILER_DEBUG_FLAG="-g"
if test "$enable_debug" = "yes" ; then
  CXXFLAGS+=" $COMPILER_DEBUG_FLAG"
else
  CXXFLAGS=`eval "$ECHO \"$CXXFLAGS\" | $SED -e 's:$COMPILER_DEBUG_FLAG::g'" 2>$DEVNULL`
fi

# end debug --------------------------------------------------------------------

# set optimize ------------------------------------------------------------------

AC_ARG_ENABLE([optimize], AC_HELP_STRING([--enable-optimize@<:@=yes/no@:>@],
  [Enable optimize compile flags. @<:@default=yes@:>@]),
  [], [enable_optimize="yes"])

if test "$enable_optimize" = "yes" ; then
  CXXFLAGS+=" -O2 -DRELEASE"
else
  CXXFLAGS+=" -O0 -DDEBUG"
fi

# end optimize ------------------------------------------------------------------

# check gcov and profiling ------------------------------------------------------

AC_ARG_WITH([gcov], AC_HELP_STRING([--with-gcov@<:@=yes/no@:>@],
  [Link with gcov library. @<:@default=no@:>@]))
AC_ARG_ENABLE([profiling],
  AC_HELP_STRING([--enable-profile@<:@=useprofile/sampling/no@:>@],
  [Enable gcc profile and optimize. @<:@default=no@:>@]),
  [], [enable_profile="no"])

if test "$with_gcov" = "yes" ; then
  CXXFLAGS+=" -fprofile-arcs -ftest-coverage"
  LDFLAGS+=" -lgcov"
elif test "$enable_profile" = "useprofile" ; then
  CXXFLAGS+=" -fprofile-use"
  LDFLAGS+=" -fprofile-use"
elif test "$enable_profile" = "sampling" ; then
  CXXFLAGS+=" -fprofile-genereate"
  LDFLAGS+=" -fprofile-genereate"
  AC_MSG_NOTICE([GCC optimize using profile need agent library running.
    And if you wish optimization with using profile,
    then don't remove "*.gcda/*.gcno" files
    So don't use "make clean" command which delete such files.
    You should use "make mostly-clean" command.])
fi

# end gcov and profiling  -------------------------------------------------------

# end of configure attacher  ---------------------------------------------------

AC_CONFIG_FILES([Makefile agent/Makefile agent/src/Makefile agent/src/heapstats-engines/Makefile agent/attacher/Makefile agent/attacher/heapstats-attacher agent/src/iotracer/Makefile mbean/Makefile mbean/native/Makefile])
AC_CONFIG_FILES([agent/heapstats.conf])

AC_OUTPUT
