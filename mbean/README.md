HeapStats MBean
===================

HeapStats MBean adds the feature to access HeapStats Agent which is attached to its process via JMX.  
It is composed to two modules:

* `heapstats-mbean.jar`
    * Providing MBean and related configuration file
    * Including `jboss-service.xml` for adding this ability to JBoss and/or WildFly
* `libheapstats-mbean.so`
    * Native part of HeapStats MBean
    * Required by `heapstats-mbean.jar`

# Requirements #

* HeapStats Agent
* Linux x64 / x86_64 / AArch32
* Oracle JDK / OpenJDK

# How to use #

1. Deploy `heapstats-mbean.jar` and `libheapstats-mbean.so` to your application
2. Enable JMX on your application
3. Access to `heapstats:type=HeapStats`

You can access to HeapStats MBean via JVMLive on HeapStats Analyzer or HeapStats CLI.

Implementation examples are available on [test of Agent](../agent/test/mbean).

# Build requirements #

* GNU make
* Apache Maven
* GCC
* JDK 7 or later

You can build HeapStats MBean with top of `Makefile` with `mbean` target.
