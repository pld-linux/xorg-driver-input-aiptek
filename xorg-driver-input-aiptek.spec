Summary:	X.org input driver for Aiptek HyperPen USB-based tablet devices
Summary(pl):	Sterownik wej¶ciowy X.org dla tabletów Aiptek HyperPen na USB
Name:		xorg-driver-input-aiptek
Version:	1.0.0.2
Release:	0.1
License:	MIT
Group:		X11/Applications
Source0:	http://xorg.freedesktop.org/releases/X11R7.0-RC2/driver/xf86-input-aiptek-%{version}.tar.bz2
# Source0-md5:	dd900ec97b25f233ac4b53787a607754
URL:		http://xorg.freedesktop.org/
BuildRequires:	autoconf >= 2.57
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	pkgconfig >= 1:0.19
BuildRequires:	xorg-proto-inputproto-devel
BuildRequires:	xorg-proto-randrproto-devel
BuildRequires:	xorg-util-util-macros >= 0.99.1
BuildRequires:	xorg-xserver-server-devel >= 0.99.3
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%description
X.org input driver for Aiptek HyperPen USB-based tablet devices.

%description -l pl
Sterownik wej¶ciowy X.org dla tabletów Aiptek HyperPen na USB.

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
	DESTDIR=$RPM_BUILD_ROOT \
	drivermandir=%{_mandir}/man4

rm -f $RPM_BUILD_ROOT%{_libdir}/xorg/modules/*/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc ChangeLog
%attr(755,root,root) %{_libdir}/xorg/modules/input/aiptek_drv.so
%{_mandir}/man4/aiptek.4x*
