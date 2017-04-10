# Test for HeapStats

Test cases for HeapStats. These test cases could run on containers.

## Usage

### on container

```
docker build --pull -t heapstatas/test -f ./Dockerfile .
```
If run under proxy, run with `--build-arg http_proxy=<proxy> --build-arg https_proxy=<proxy> --build-arg _JAVA_OPTIONS="-Dhttps.proxyHost=<proxy host> -Dhttps.proxyPort=<proxy port>"`

Once you have the image, you can easily run tests as below.

```
docker run --rm  -t heapstats/test ./<dirctory>/testcase.sh

# You need security option when you run race-condition/testcase.sh
docker run --rm --security-opt seccomp=unconfined -t heapstats/test ./race-condition/testcase.sh
```

These commands are just example, you can use container as you like.

#### Container for RHEL 7

You can create an image of RHEL 7 by using Dockerfile.el7. This requires Red Hat subscription(s), read [Red Hat's documentations](https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/7/html/7.0_Release_Notes/sect-Red_Hat_Enterprise_Linux-7.0_Release_Notes-Linux_Containers_with_Docker_Format-Using_Docker.html) for details.

Also, you need to get and build gtest when you test by ./gtest .

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

