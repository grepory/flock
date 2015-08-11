Summary: A program to manage locks from shell scripts
Name: flock
Version: 2.0
Release: 1
License: MIT
Group: Applications/System
Source0: ftp://ftp.kernel.org/pub/software/utils/script/flock/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
A small utility to manage locks from shell scripts.  This is very
useful in keeping automated tasks from stepping on each other.

%prep
%setup -q

%build
make prefix=%{_prefix} BINDIR=%{_bindir}  MANDIR=%{_mandir}

%install
rm -rf $RPM_BUILD_ROOT
make install prefix=%{_prefix} BINDIR=%{_bindir}  MANDIR=%{_mandir}/man1 \
	INSTALLROOT=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc %{_mandir}/man1/*
%{_bindir}/flock

%doc


%changelog
* Sat Jul  9 2005 H. Peter Anvin <hpa@zytor.com>
- Release 2.0, add the ability to spawn a process.

* Fri May  2 2003 H. Peter Anvin <hpa@zytor.com>
- Release 1.0.1, make -h work correctly.

* Mon Mar 17 2003 H. Peter Anvin <hpa@zytor.com>
- Initial build.
