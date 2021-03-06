%define name	gnome-alsamixer
%define ver	0.9.7
%define RELEASE	paw.1
%define rel	%{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix /usr

Summary:   An ALSA mixer for GNOME written for ALSA 0.9.x.
Name:      %{name}
Version:   %{ver}
Release:   %{rel}
Packager:  Dennis J Houy <djhouy@paw.za.org>
Vendor:    PAW Digital Dynamics
Copyright: GPL
Group:     Applications/Multimedia
Source:    ftp://ftp.paw.za.org/pub/PAW/sources/gnome-alsamixer-%{ver}.tar.gz
URL:       http://www.paw.za.org/projects/gnome-alsamixer
BuildRoot: /var/tmp/gnome-alsamixer-%{PACKAGE_VERSION}-root

Requires: gtk2 >= 2.4.0
Requires: GConf2 >= 2.8.0
Requires: alsa-lib >= 0.9.0

BuildRequires: gtk2-devel >= 2.0.0
BuildRequires: GConf2-devel >= 2.8.0
BuildRequires: alsa-lib >= 0.9.0
BuildRequires: desktop-file-utils

%description
A sound mixer for GNOME which is written for the Advanced Linux Sound
Architecture (ALSA), which supports ALSA 0.9.x.

%prep
%setup -q -n %{name}-%{version}

%build
%configure
make

%install
rm -rf $RPM_BUILD_ROOT
export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
make DESTDIR=$RPM_BUILD_ROOT install-strip
unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL

cat << EOF > gnome-alsamixer.desktop
[Desktop Entry]
Encoding=UTF-8
Name=ALSA Volume Control
Comment=Adjust the volume level
Exec=gnome-alsamixer
Icon=gnome-alsamixer/gnome-alsamixer-icon.png
Terminal=false
Type=Application
Categories=GNOME;Application;AudioVideo;X-Red-Hat-Base;
StartupNotify=true
X-Desktop-File-Install-Version=0.3
EOF

mkdir ${RPM_BUILD_ROOT}%{_datadir}/applications

desktop-file-install --vendor "PAW Digital Dynamics" --delete-original \
  --dir ${RPM_BUILD_ROOT}%{_datadir}/applications                      \
  --add-category GNOME                                                 \
  --add-category Application                                           \
  --add-category AudioVideo                                            \
  --add-category X-Red-Hat-Base                                        \
  gnome-alsamixer.desktop

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/%{name}.schemas &> /dev/null || :
/usr/bin/killall -HUP gconfd-2

%postun
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-uninstall-rule %{_sysconfdir}/gconf/schemas/%{name}.schemas &> /dev/null || :
/usr/bin/killall -HUP gconfd-2

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc README COPYING ChangeLog NEWS AUTHORS INSTALL TODO
%{_bindir}/%{name}
%{_sysconfdir}/gconf/schemas/%{name}.schemas
%{_datadir}/applications/*%{name}.desktop
%{_datadir}/pixmaps/%{name}
%{_datadir}/locale/*

%changelog
* Thu Jul  7 2005 Derrick J Houy <djhouy@paw.za.org>
- Added GConf stuff, and gnome-alsamixer.schemas to files section.

* Wed Sep 10 2003 Dennis J Houy <djhouy@paw.za.org>
- Added locale directory.

* Mon Sep  8 2003 Dennis J Houy <djhouy@paw.za.org>
- Added creation of Red Hat menu entry to spec file.

* Mon Nov 11 2002 Dennis J Houy <djhouy@paw.za.org>
- New build for GNOME2.

* Sun Sep 22 2002 Dennis J Houy <djhouy@paw.za.org> - 0.1.2b
- Initial release version.
