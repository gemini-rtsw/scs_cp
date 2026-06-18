%define _prefix /gem_base/epics/ioc
%define name scs_cp
%define repository gemdev
%define debug_package %{nil}
%define arch %(uname -m)
%define checkout %(git log --pretty=format:'%h' -n 1) 

#These global defines are added to prevent stripping
# symbols on vxWorks cross-compiled code
# Getting 'strip' to work is probably only needed for
# building a related debug sub-package
#
# But this prevents all the strip warnings
# mrippa 20120202
%global _enable_debug_package 0
%global debug_package %{nil}
%global __os_install_post /usr/lib/rpm/brp-compress %{nil}

Summary: %{name} Package, a module for EPICS base
Name: %{name}
Version: 2.9
Release: 10%{?dist}
License: EPICS Open License
Group: Applications/Engineering
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: %{arch}
Prefix: %{_prefix}
## You may specify dependencies here
## Versions are pinned exactly. To upgrade a dependency, edit the version
## below explicitly — do not relax the pins.
BuildRequires: re2c
BuildRequires: tdct
BuildRequires: epics-base-devel    = 7.0.7-0.git.f9e3717%{?dist}
BuildRequires: sequencer-devel     = 2.2.9.e5e3615-4.git.7b11f25%{?dist}
BuildRequires: autosave-devel      = 5.10.2-0.git.57f0212%{?dist}
BuildRequires: bancomm-devel       = 1.6.13-4.git.6f4ba7b%{?dist}
BuildRequires: geminiRec-devel     = 4.1.13-3.git.6f58677%{?dist}
BuildRequires: timelib-devel       = 2.1.4-3.git.a504360%{?dist}
BuildRequires: slalib-devel        = 1.9.7-6.git.54d124d%{?dist}
BuildRequires: xycom-devel         = 2.1.12-2.git.e8d8b00%{?dist}
BuildRequires: gemUtil-devel       = 1.6.13-2.git.9268b49%{?dist}
BuildRequires: timeProbe-devel     = 1.1.16-3.git.1bb32b7%{?dist}
BuildRequires: tcslib-devel        = 1.1.1-9.git.b18a9b9%{?dist}
BuildRequires: pvload-devel        = 1.2.1-7.git.6df46ce%{?dist}
BuildRequires: symb-devel          = 1.6.13-4.git.d097ef5%{?dist}
BuildRequires: vmi5588-devel       = 1.3-1.git.6fe16cd%{?dist}
## No runtime Requires on the main package.
## The artifacts shipped here are cross-compiled for the VME target
## (CROSS_COMPILER_TARGET_ARCHS = RTEMS-mvme2700) and are network-booted
## by the board; nothing in this package executes on the el8 install host.
## The support modules above are needed only to LINK the cross-build, hence
## they are BuildRequires only. Carrying them as exactly-pinned runtime
## Requires would make this package conflict with any co-installed RPM that
## pins a different version of the same support module, for no benefit.
## (The -devel subpackage below intentionally keeps its pins — its purpose
## is to reproduce the exact build environment.)
## Switch dependency checking off
AutoReqProv: no

%description
This is the module %{name}.

%package devel
Summary: %{name}-devel Package
Group: Development/Gemini
Requires: %{name} = %{version}-%{release}
Requires: tdct
Requires: re2c
Requires: epics-base-devel = 7.0.7-0.git.f9e3717%{?dist}
Requires: sequencer-devel  = 2.2.9.e5e3615-4.git.7b11f25%{?dist}
Requires: autosave-devel   = 5.10.2-0.git.57f0212%{?dist}
Requires: bancomm-devel    = 1.6.13-4.git.6f4ba7b%{?dist}
Requires: geminiRec-devel  = 4.1.13-3.git.6f58677%{?dist}
Requires: timelib-devel    = 2.1.4-3.git.a504360%{?dist}
Requires: slalib-devel     = 1.9.7-6.git.54d124d%{?dist}
Requires: xycom-devel      = 2.1.12-2.git.e8d8b00%{?dist}
Requires: gemUtil-devel    = 1.6.13-2.git.9268b49%{?dist}
Requires: timeProbe-devel  = 1.1.16-3.git.1bb32b7%{?dist}
Requires: tcslib-devel     = 1.1.1-9.git.b18a9b9%{?dist}
Requires: pvload-devel     = 1.2.1-7.git.6df46ce%{?dist}
Requires: symb-devel       = 1.6.13-4.git.d097ef5%{?dist}
Requires: vmi5588-devel    = 1.3-1.git.6fe16cd%{?dist}
%description devel
This is the module %{name}.

%prep
%setup -q 

%build
#update environment from former rpm installations due to BuildRequires
source /gem_base/etc/profile
#start virtual framebuffer to have graphics for java
#Xvfb :1  -ac -nolisten tcp -nolisten unix &

make distclean uninstall
#DISPLAY=:1 make
make
#killall Xvfb

%install
export DONT_STRIP=1
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{_prefix}/%{name}
cp -r dbd $RPM_BUILD_ROOT/%{_prefix}/%{name}
cp -r db $RPM_BUILD_ROOT/%{_prefix}/%{name}
cp -r bin $RPM_BUILD_ROOT/%{_prefix}/%{name}
cp -r configure $RPM_BUILD_ROOT/%{_prefix}/%{name}

%postun
if [ "$1" = "0" ]; then
	rm -rf %{_prefix}/%{name}
fi


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
   /%{_prefix}/%{name}/bin
   /%{_prefix}/%{name}/db
   /%{_prefix}/%{name}/dbd
   /%{_prefix}/%{name}/configure

%files devel
%defattr(-,root,root)
   # No files needed for devel package, just dependencies

%changelog
* Wed Mar 26 2024 Pedro Gigoux <pedro.gigoux@noirlab.edu> 2.9
- Commented out devel entries






