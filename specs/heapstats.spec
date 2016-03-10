%define __check_files %{nil}

Summary: A Java programming language debugging tool.
Name: heapstats
Version: 2.0.trunk
%define DIST_EXT 0%{?dist:%{dist}}%{!?dist:.el5}
Release: %{DIST_EXT}
License: GPLv2
Vendor: NTT OSS Center
Group: Development/Tools
Source: heapstats-%{version}.tar.gz
#Patch0: none
Buildroot: /var/tmp/heapstats

# Requires for running
Requires: pcre >= 6

# Requires for building
BuildRequires: pcre-devel >= 6
BuildRequires: net-snmp-devel >= 5.3
BuildRequires: java-1.8.0-openjdk-devel
BuildRequires: binutils >= 2
BuildRequires: binutils-devel
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: maven

%package cli
Summary:   HeapStats CLI
Group:     Development/Tools
BuildArch: noarch

%description
A lightweight monitoring JVMTI agent for Java HotSpot VM.
Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation.

%description cli
Commandline analysis tool for HeapStats.
Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation.

%prep
%setup -q -n heapstats-2.0

%build
CXXFLAGS="$RPM_OPT_FLAGS" ./configure \
  --build=%{_build} \
  --host=%{_host} \
  --target=%{_target_platform} \
  --prefix=%{_prefix} \
  --libdir=%{_libdir}/heapstats \
  --libexecdir=%{_libexecdir}/heapstats \
  --sysconfdir=%{_sysconfdir}/heapstats \
  --disable-vmstructs \
  %{configure_opts} \
  --enable-debug \
  --enable-optimize \
  --without-gcov \
  --disable-profile 
make agent mbean RPM_OPT_FLAGS="$RPM_OPT_FLAGS"
mvn -am -pl analyzer/cli package

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p ${RPM_BUILD_ROOT}%{_libdir}/heapstats
cd agent
make install DESTDIR=${RPM_BUILD_ROOT}
cd ../mbean
make install DESTDIR=${RPM_BUILD_ROOT}
cd ../
mkdir -p $RPM_BUILD_ROOT/etc/ld.so.conf.d/
echo "%{_libdir}/heapstats" \
  >> $RPM_BUILD_ROOT/etc/ld.so.conf.d/heapstats-agent.conf
mkdir -p $RPM_BUILD_ROOT/usr/share/snmp/mibs/
cp ./agent/mib/HeapStatsMibs.txt $RPM_BUILD_ROOT/usr/share/snmp/mibs/

# We do not privide FX analyzer.
# So we install CLI analyzer manually.
mkdir -p ${RPM_BUILD_ROOT}/%{_libexecdir}/heapstats
cp -fR ./analyzer/cli/target/heapstats-cli-2.0-bin/heapstats-cli-2.0.trunk/* \
                                    ${RPM_BUILD_ROOT}%{_libexecdir}/heapstats/
cp -f ./analyzer/cli/heapstats-cli ${RPM_BUILD_ROOT}%{_bindir}
chmod a+x ${RPM_BUILD_ROOT}%{_bindir}/heapstats-cli

%post
/sbin/ldconfig

%postun
/sbin/ldconfig
rm -fR %{_libdir}/heapstats

%clean
rm -rf $RPM_BUILD_ROOT

%files
%dir %{_libdir}/heapstats/
%{_libdir}/heapstats/libheapstats-*.so*
%{_libdir}/heapstats/heapstats-mbean.jar
%dir %{_libdir}/heapstats/heapstats-engines/
%{_libdir}/heapstats/heapstats-engines/libheapstats-engine*.so
%doc NEWS README COPYING AUTHORS ChangeLog
%dir %{_sysconfdir}/heapstats/
%config(missingok) %{_sysconfdir}/heapstats/heapstats.conf
%dir %{_sysconfdir}/heapstats/iotracer/
%{_sysconfdir}/heapstats/iotracer/IoTrace.class
/usr/bin/heapstats-attacher
/usr/libexec/heapstats/heapstats-attacher.jar
/etc/ld.so.conf.d/heapstats-agent.conf
/usr/share/snmp/mibs/HeapStatsMibs.txt

%files cli
/usr/bin/heapstats-cli
%dir %{_libexecdir}/heapstats/
%{_libexecdir}/heapstats/heapstats-cli.jar
%dir %{_libexecdir}/heapstats/lib/
%{_libexecdir}/heapstats/lib/heapstats-core.jar
%{_libexecdir}/heapstats/lib/heapstats-mbean.jar


%changelog
* Thu Mar 10 2016 Yasumasa Suenaga <yasuenag@gmail.com>
- Remove dependency on net-snmp-libs.
* Tue Feb 09 2016 KUBOTA Yuji <kubota.yuji@lab.ntt.co.jp>
- Set version to 2.0.trunk
* Thu Oct 22 2015 Yasumasa Suenaga <yasuenag@gmail.com>
- Add CLI package.
- Add IoTrace class.
* Thu Oct 22 2015 KUBOTA Yuji <kubota.yuji@lab.ntt.co.jp>
- Bump to 2.0
* Thu Jul 24 2014 Yasumasa Suenaga <yasuenag@gmail.com>
- Sync HeapStats 1.1

