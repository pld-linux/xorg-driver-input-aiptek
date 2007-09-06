Summary:	X.org input driver for Aiptek HyperPen USB-based tablet devices
Summary(pl.UTF-8):	Sterownik wejściowy X.org dla tabletów Aiptek HyperPen na USB
Name:		xorg-driver-input-aiptek
Version:	1.0.1
Release:	0.2
License:	MIT
Group:		X11/Applications
Source0:	http://xorg.freedesktop.org/releases/individual/driver/xf86-input-aiptek-%{version}.tar.bz2
# Source0-md5:	951b2b1a270f67d28e2e89fd2b9f15ae
# from http://dl.sourceforge.net/aiptektablet/xorg-x11-drv-aiptek-1.0.1-2RvP.src.rpm
Source1:	xf86Aiptek.c
Source2:	xf86Aiptek.h
URL:		http://aiptektablet.sourceforge.net/
BuildRequires:	autoconf >= 2.57
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	pkgconfig >= 1:0.19
BuildRequires:	xorg-proto-inputproto-devel
BuildRequires:	xorg-proto-randrproto-devel
BuildRequires:	xorg-util-util-macros >= 0.99.2
BuildRequires:	xorg-xserver-server-devel >= 1.0.99.901
BuildRequires:  rpmbuild(macros) >= 1.389
%requires_xorg_xserver_xinput
Requires:	xorg-xserver-server >= 1.0.99.901
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%description
X.org input driver for Aiptek HyperPen USB-based tablet devices.

%description -l pl.UTF-8
Sterownik wejściowy X.org dla tabletów Aiptek HyperPen na USB.

%prep
%setup -q -n xf86-input-aiptek-%{version}

cp -f %{SOURCE1} %{SOURCE2} src

%build
%{__libtoolize}
%{__aclocal}
%{__autoconf}
%{__autoheader}
%{__automake}
%configure \
	--disable-static

%{__make}

%install
rm -rf $RPM_BUILD_ROOT

%{__make} install \
	DESTDIR=$RPM_BUILD_ROOT

rm -f $RPM_BUILD_ROOT%{_libdir}/xorg/modules/*/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc COPYING ChangeLog
%attr(755,root,root) %{_libdir}/xorg/modules/input/aiptek_drv.so
%{_mandir}/man4/aiptek.4*
