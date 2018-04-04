FROM fedora:latest

MAINTAINER KUBOTA Yuji <kubota.yuji@gmail.com>

ENV HEAPSTATS_PATH /heapstats
ARG _JAVA_OPTIONS=""

# Update without using cache
## https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/#run
RUN dnf update -y \
 && dnf install -y \
    git

# Install dependencies for HeapStats
RUN dnf install -y \
    ant \
    binutils-devel \
    gcc-c++ \
    tbb-devel \
# If you want to test with old jdk, use below
#    java-1.6.0-openjdk-devel java-1.7.0-openjdk-devel \
    java-1.8.0-openjdk-devel \
    maven \
    net-snmp-devel net-snmp-libs \
    pcre-devel \
 && dnf install -y \
    --enablerepo="*debug*" --disablerepo="*test*" \
# If you want to test with old jdk, use below
#    java-1.6.0-openjdk-debuginfo java-1.7.0-openjdk-debuginfo \
    java-1.8.0-openjdk-debuginfo

ENV JAVA_HOME /usr/lib/jvm/java-1.8.0-openjdk
ENV PATH ${PATH}:${JAVA_HOME}/lib/

# Build HeapStats
RUN git clone https://github.com/HeapStats/heapstats.git ${HEAPSTATS_PATH}

WORKDIR ${HEAPSTATS_PATH}
RUN bash configure --with-jdk=${JAVA_HOME} \
 && make agent mbean

ENV HEAPSTATS_LIB ${HEAPSTATS_PATH}/agent/src/libheapstats-2.2.so.3

# Prepare to test HeapStats
WORKDIR ${HEAPSTATS_PATH}/agent/test/gtest
RUN dnf install -y \
    gtest-devel \
    gdb
#RUN make

WORKDIR ${HEAPSTATS_PATH}/agent/test
RUN git clone https://github.com/HeapStats/race-condition.git
RUN useradd testuser \
 && chown -R testuser.testuser ./*
USER testuser
