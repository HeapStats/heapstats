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

# Check OpenJFX supported platform
%if %{?fedora:%{fedora}}%{!?fedora:0} >= 26
%define WITH_ANALYZER 1
%endif

# Requires for running
Requires: pcre >= 6
Requires: tbb

# Requires for building
BuildRequires: pcre-devel >= 6
BuildRequires: net-snmp-devel >= 5.3
BuildRequires: java-1.8.0-openjdk-devel
BuildRequires: binutils >= 2
BuildRequires: binutils-devel
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: tbb-devel

%if 0%{?WITH_ANALYZER:1}
BuildRequires: java-1.8.0-openjdk-openjfx-devel
%endif

%package cli
Summary:   HeapStats CLI
Group:     Development/Tools
BuildArch: noarch

%if 0%{?WITH_ANALYZER:1}
%package analyzer
Summary:   HeapStats Analyzer
Group:     Development/Tools
BuildArch: noarch
Requires: java-1.8.0-openjdk-openjfx
Requires: heapstats-cli
%endif

%description
A lightweight monitoring JVMTI agent for Java HotSpot VM.
Copyright (C) 2011-2017 Nippon Telegraph and Telephone Corporation.

%description cli
Commandline analysis tool for HeapStats.
Copyright (C) 2011-2017 Nippon Telegraph and Telephone Corporation.

%if 0%{?WITH_ANALYZER:1}
%description analyzer
HeapStats GUI Analyzer
Copyright (C) 2011-2017 Nippon Telegraph and Telephone Corporation.
%endif

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

%if 0%{?WITH_ANALYZER:1}
  make
%else
  make agent mbean RPM_OPT_FLAGS="$RPM_OPT_FLAGS"
  mvn -am -pl analyzer/cli package
%endif

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/ld.so.conf.d/
echo "%{_libdir}/heapstats" \
  >> $RPM_BUILD_ROOT/etc/ld.so.conf.d/heapstats-agent.conf
mkdir -p $RPM_BUILD_ROOT/usr/share/snmp/mibs/
cp ./agent/mib/HeapStatsMibs.txt $RPM_BUILD_ROOT/usr/share/snmp/mibs/

%if 0%{?WITH_ANALYZER:1}
  make install DESTDIR=${RPM_BUILD_ROOT}
%else
  mkdir -p ${RPM_BUILD_ROOT}%{_libdir}/heapstats
  cd agent
  make install DESTDIR=${RPM_BUILD_ROOT}
  cd ../mbean
  make install DESTDIR=${RPM_BUILD_ROOT}
  cd ../

  # We do not privide FX analyzer.
  # So we install CLI analyzer manually.
  mkdir -p ${RPM_BUILD_ROOT}/%{_libexecdir}/heapstats
  cp -fR ./analyzer/cli/target/heapstats-cli-*-bin/heapstats-cli-*/* \
                                    ${RPM_BUILD_ROOT}%{_libexecdir}/heapstats/
  cp -f ./analyzer/cli/heapstats-cli ${RPM_BUILD_ROOT}%{_bindir}
  chmod a+x ${RPM_BUILD_ROOT}%{_bindir}/heapstats-cli
%endif

%post
/sbin/ldconfig

%preun
if [ $1 -eq 0 ]; then  # uninstall
  # Remove symlink which is generated by ldconfig
  rm -f %{_libdir}/heapstats/libheapstats.so
fi

%postun
/sbin/ldconfig

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
%{_libexecdir}/heapstats/lib/heapstats-jmx-helper.jar

%if 0%{?WITH_ANALYZER:1}
%files analyzer
/usr/bin/heapstats-analyzer
%{_libexecdir}/heapstats/heapstats-analyzer.jar
%{_libexecdir}/heapstats/THIRD_PARTY_README
%{_libexecdir}/heapstats/filterDefine.xsd
%{_libexecdir}/heapstats/heapstats.properties
%{_libexecdir}/heapstats/lib/jgraphx.jar
%{_libexecdir}/heapstats/lib/heapstats-plugin-api.jar
%endif

%changelog
* Tue Jan 23 2018 KUBOTA Yuji <kubota.yuji@lab.ntt.co.jp>
- Remove SSE 3 optimized binary.
* Thu Jan 18 2018 Yasumasa Suenaga <yasuenag@gmail.com>
- Add dependencies to TBB
* Tue Jul 11 2017 Yasumasa Suenaga <yasuenag@gmail.com>
- Add Analyzer package.
* Tue Feb 09 2016 KUBOTA Yuji <kubota.yuji@lab.ntt.co.jp>
- Set version to 2.0.trunk
* Thu Oct 22 2015 Yasumasa Suenaga <yasuenag@gmail.com>
- Add CLI package.
- Add IoTrace class.
* Thu Oct 22 2015 KUBOTA Yuji <kubota.yuji@lab.ntt.co.jp>
- Bump to 2.0
* Thu Jul 24 2014 Yasumasa Suenaga <yasuenag@gmail.com>
- Sync HeapStats 1.1

