FROM registry.access.redhat.com/rhel7:latest

MAINTAINER KUBOTA Yuji <kubota.yuji@gmail.com>

ENV HEAPSTATS_PATH /heapstats
ARG _JAVA_OPTIONS=""

# Install common requirements
RUN yum update -y \
 && yum install -y \
    git \
    wget

# Install dependencies for HeapStats
RUN yum install -y \
    ant \
    binutils-devel \
    gcc-c++ \
    tbb-devel \
# If you want to test with old jdk, use below
#    java-1.6.0-openjdk-devel java-1.7.0-openjdk-devel \
    java-1.8.0-openjdk-devel \
    make \
    net-snmp-devel net-snmp-libs \
    pcre-devel \
# When RHEL provides maven 3.3 or later, use below.
# && yum install -y --enablerepo="rhel-7-server-optional-rpms" \
#    maven \
 && yum install -y --enablerepo="rhel-server-rhscl-7-beta-rpms" \
    rh-maven33 \
 && yum install -y --enablerepo="*debug*" --disablerepo="*beta*,*test*" \
# If you want to test with old jdk, use below.
#    java-1.6.0-openjdk-debuginfo java-1.7.0-openjdk-debuginfo \
    java-1.8.0-openjdk-debuginfo

ENV JAVA_HOME /usr/lib/jvm/java-1.8.0-openjdk
ENV PATH "/opt/rh/rh-maven33/root/usr/bin:${PATH:-/bin:/usr/bin}"

# Build  HeapStats
RUN git clone https://github.com/HeapStats/heapstats.git ${HEAPSTATS_PATH}

WORKDIR ${HEAPSTATS_PATH}
RUN bash configure --with-jdk=${JAVA_HOME} \
 && make agent mbean

ENV HEAPSTATS_LIB ${HEAPSTATS_PATH}/agent/src/libheapstats-2.2.so.3

# Prepare to test HeapStats
WORKDIR ${HEAPSTATS_PATH}/agent/test
RUN git clone https://github.com/HeapStats/race-condition.git
RUN yum install -y \
    python-devel \
    texinfo \
 && wget -q http://ftp.gnu.org/gnu/gdb/gdb-7.12.tar.gz \
 && tar zxf gdb-7.12.tar.gz \
 && cd gdb-7.12 \
 && bash configure --prefix=/opt/gdb --with-python \
 && make \
 && make install \
 && cd ../ \
 && rm -rf gdb-7.12*

ENV GDB_PATH /opt/gdb

