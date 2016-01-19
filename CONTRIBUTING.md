# Contributing guidelines

We gladly accept new feature to this project. Please read the guidelines for reporting issues and submitting pull requests below.

### Reporting issues

- Please make clear in the subject what the issue is about.
- Include the version and vendor of Java Virtual Machine you are using.
 - HeapStats agent depends on HotSpot VM, e.g., OpenJDK and Oracle JDK. If you uses other JVM, we may not fix the issue because the codes are not opened. But, pull request(s) which support other JVM are very welcome!

***NOTE***: HeapStats project uses [bugzilla](http://icedtea.classpath.org/bugzilla/) mainly. If you have a time, please [create a new bug](http://icedtea.classpath.org/bugzilla/enter_bug.cgi?product=HeapStats) for bugzilla.

### Pull request guidelines

1. [Fork it](http://github.com/HeapStats/heapstats/fork) and clone your new repo
2. Create a branch (`git checkout -b my_good_feature`)
3. Commit your changes (`git add my/good/file.cpp; git commit -m "Added my good feature"`)
4. Push your changes to your fork (`git push origin my_good_feature`)
5. Open a [Pull Request](https://github.com/HeapStats/heapstats/pulls)

The important cautions:

- Your commit message(s) will be modified by maintainer to follow [icedtea format](http://icedtea.classpath.org/hg/heapstats/log).
 - Do NOT worry, we ***NEVER*** remove your contributions. Just add small fixes.
- Do not update the ChangeLog.

