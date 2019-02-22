#
# spec file for package libmbus
#
# Copyright (c) 2010-2013, Raditex Control AB
# All rights reserved.
#
# rSCADA 
# http://www.rSCADA.se
# info@rscada.se
#

Summary: 	Open source M-bus (Meter-Bus) library
Name: 		libmbus
Version: 	0.9.0
Release: 	1
Source:	 	https://github.com/rscada/%{name}/archive/%{version}.tar.gz
URL:		https://github.com/rscada/libmbus/
License:	BSD
Vendor:		Raditex Control AB
Packager:	Stefan Wahren <info@lategoodbye.de>
Group: 		Development/Languages/C and C++ 
BuildRoot: 	{_tmppath}/%{name}-%{version}-build
AutoReqProv:	on 

%description
libmbus: M-bus Library from Raditex Control (http://www.rscada.se)

libmbus is an open source library for the M-bus (Meter-Bus) protocol. 
The Meter-Bus is a standard for reading out meter data from electricity meters, 
heat meters, gas meters, etc. The M-bus standard deals with both the electrical
signals on the M-Bus, and the protocol and data format used in transmissions 
on the M-Bus. The role of libmbus is to decode/encode M-bus data, and to handle
the communication with M-Bus devices.

For more information see http://www.rscada.se/libmbus

%package devel
License:        BSD
Summary:        Development libraries and header files for using the M-bus library
Group:          Development/Libraries/C and C++
AutoReqProv:    on
Requires:       %{name} = %{version}

%description devel
This package contains all necessary include files and libraries needed
to compile and link applications which use the M-bus (Meter-Bus) library.

%prep -q
%setup -q
# workaround to get it's build
autoreconf

%build
./configure --prefix=/usr
make

%install
rm -Rf "%buildroot"
mkdir "%buildroot"
make install DESTDIR="%buildroot"

%clean
rm -rf "%buildroot"

%files
%defattr (-,root,root)
%doc COPYING README.md
%{_bindir}/mbus-serial-*
%{_bindir}/mbus-tcp-*
%{_libdir}/libmbus.so*
%{_mandir}/man1/libmbus.1
%{_mandir}/man1/mbus-*

%files devel
%defattr (-,root,root)
%{_includedir}/mbus
%{_libdir}/libmbus.a
%{_libdir}/libmbus.la
%{_libdir}/pkgconfig/libmbus.pc

%changelog
* Fri Feb 22 2019 Stefan Wahren <info@lategoodbye.de> - 0.9.0-1
- switch to github repo
- enable man pages

* Fri Mar 29 2013 Stefan Wahren <info@lategoodbye.de> - 0.8.0-1
- Initial package based on the last official release
