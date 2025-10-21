Name:       harbour-subsail

Summary:    SubSail subtitle viewer
Version:    0.6
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
Simple subtitle viewer for SailfishOS. Supports SubRip (.srt), MicroDVD (.sub)
and SubViewer 1.0/2.0 (.sub) files. Time can be adjusted directly or via
offset. FPS can be changed for the .sub files whn the file does not supply the
FPS information. Subtitle size is adjustable up to 200pt. The default coded
used when BOM detection fails on a subtitle file can be changed in the
settings.

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
