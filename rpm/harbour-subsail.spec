Name:       harbour-subsail

Summary:    SubSail subtitle viewer for .srt and .sub
Version:    0.4
Release:    1
License:    GPLv3
URL:        https://github.com/LaakkonenJussi/harbour-subsail
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  desktop-file-utils

%description
Simple subtitle viewer for SailfishOS. Supports SRT and SUB files.
FPS can be changed for the loaded SUB files only. Subtitle size is adjustable
via main UI and offset/time change is supported. The default codec used when
BOM detection fails can be changed in settings.

%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 

%make_build


%install
%qmake5_install


desktop-file-install --delete-original         --dir %{buildroot}%{_datadir}/applications                %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
