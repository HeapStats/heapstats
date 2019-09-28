HeapStats Attacher
===================

HeapStats Attacher can attach JVMTI agent such as HeapStats Agent to existed process.

# Requirements #

* Oracle JDK / OpenJDK 7 or later

# How to use #

```
$ heapstats-attacher /path/to/libheapstats.so [heapstats.conf]
```

# Build requirements #

* Apache Ant
* JDK 7 or later

You can build HeapStats Attacher with top of `Makefile` with `agent` target.
