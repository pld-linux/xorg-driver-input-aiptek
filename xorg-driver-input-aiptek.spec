Summary:	X.org input driver for Aiptek HyperPen USB-based tablet devices
Summary(pl.UTF-8):	Sterownik wejściowy X.org dla tabletów Aiptek HyperPen na USB
Name:		xorg-driver-input-aiptek
Version:	1.4.1
Release:	8
License:	MIT
Group:		X11/Applications
Source0:	http://xorg.freedesktop.org/releases/individual/driver/xf86-input-aiptek-%{version}.tar.bz2
# Source0-md5:	8231f6ce1c477eac653c9deb527fa3cb
URL:		http://aiptektablet.sourceforge.net/
BuildRequires:	autoconf >= 2.60
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	pkgconfig >= 1:0.19
BuildRequires:	xorg-proto-inputproto-devel
BuildRequires:	xorg-proto-randrproto-devel
BuildRequires:	xorg-util-util-macros >= 1.8
BuildRequires:	xorg-xserver-server-devel >= 1.9.0
BuildRequires:  rpmbuild(macros) >= 1.389
%{?requires_xorg_xserver_xinput}
Requires:	xorg-xserver-server >= 1.9.0
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%description
X.org input driver for Aiptek HyperPen USB-based tablet devices.

%description -l pl.UTF-8
Sterownik wejściowy X.org dla tabletów Aiptek HyperPen na USB.

%prep
%setup -q -n xf86-input-aiptek-%{version}

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

%{__rm} $RPM_BUILD_ROOT%{_libdir}/xorg/modules/*/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc COPYING ChangeLog README
%attr(755,root,root) %{_libdir}/xorg/modules/input/aiptek_drv.so
%{_mandir}/man4/aiptek.4*
