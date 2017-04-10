# Test for HeapStats

Test cases for HeapStats. These test cases could run on containers.

## Usage

### on container

```
docker build --pull -t heapstats/$(basename $PWD) -f ./Dockerfile(.el7) .
```
If run under proxy, run with `--build-arg http_proxy=<proxy> --build-arg https_proxy=<proxy> --build-arg _JAVA_OPTIONS="-Dhttps.proxyHost=<proxy host> -Dhttps.proxyPort=<proxy port>"`

Once you have the image, you can easily run tests as below.

```
docker run --rm  -t heapstats/$(basename $PWD) ./<dirctory>/testcase.sh

# You need security option when you run race-condition/testcase.sh
docker run --rm --security-opt seccomp=unconfined -t heapstats/$(basename $PWD) ./race-condition/testcase.sh
```

Note: You need to get and build gtest when you test ./gtest on a container of el7.

### on local machine

```
bash <directory>/testcase.sh

# You can delete result files
bash <directory>/testcase.sh --clean
```

## Requirements

There are some requirements for running test cases on your own machine.

* Libraries
  * gdb (>= 7.12)
  * gtest
  * maven (>= 3.3)
  * python (>= 2.7)
* Environment varibles
  * JAVA_HOME: Path to JDK installed (default: /usr/lib/jvm/java-openjdk)
  * HEAPSTATS_LIB: Path to libheapstats.so (default: /usr/lib64/heapstats/libheapstats.so)

