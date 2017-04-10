# Test cases for testing race condition

## Usage

1. Add directories to `testlist.txt`
2. Run `testcase.sh`

## How to add new tase cases for testing race condition.

1. Create a directory.
2. Create a `buildenv.sh` to set environment for testing.
3. Write `test.py` and testcase such like as existing test codes.

### `buildenv.sh`

`buildenv.sh` requires the following.

* `CLASSPATH`
    * Classpath to build test code.
    * Should write an absolute path.
* `MAINCLASS`
    * A main class of test code.
* `JAVA_OPTS`
    * Options for launching java process.
* `HEAPSTATS_CONF`
    * Path to `heapstats.conf` for testing
* Command line to build test code.

### `test.py`

* Import `common.py` on parent directory
* Use `common.initialize()` method with passing break point names and break condition as arguments

### Result

* `test.py` will touch `test-succeeded` when the test passed correctly. Otherwise, will touch `test-failed`.
* `testcase.sh` will show a summary of all test cases's result at last as below.

```
Test summary:
  Testcase1: succeeded
  Testcase2: succeeded
  Testcase3: failed
  :
```

