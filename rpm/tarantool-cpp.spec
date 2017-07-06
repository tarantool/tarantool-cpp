Name: tarantool-cpp
Version: 1.0.1
Release: 1%{?dist}
Summary: Tarantool C++ connector
Group: Development/Languages
License: BSD
# URL: https://github.com/tarantool/tarantool-cpp
BuildRequires: cmake
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
# Vendor: tarantool.org
Group: Applications/Databases

%description
C++ connector for Tarantool. Uses boost.asio for processing tarantool requests.

%package devel
Summary: Development files for tarantool-cpp
Requires: tarantool-cpp%{?_isa} = %{version}-%{release}
Source0: tarantool-cpp-%{version}.tar.gz

%description devel
The tarantool-cpp-devel package contains library and header files for developing application that use C++ connector for Tarantool.

##################################################################

%prep
%setup -q -n %{name}-%{version}

%build
cmake . -DCMAKE_INSTALL_LIBDIR='%{_libdir}' -DCMAKE_INSTALL_INCLUDEDIR='%{_includedir}' -DCMAKE_BUILD_TYPE='RelWithDebInfo'
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install

%files
"%{_libdir}/libtarantool-cpp.a"
"%{_libdir}/libtarantool-cpp.so"

%files devel
%dir "%{_includedir}/tarantool-cpp"
"%{_includedir}/tarantool-cpp/*.h"

%changelog
* Mon May 15 2017 Anton Kilchik <a.kilchik@corp.mail.ru> 1.0.1-1
- Couple of fixes for tarantool-c library

