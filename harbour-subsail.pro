# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = harbour-subsail

CONFIG += sailfishapp

SOURCES += \
    src/main.cpp \
    src/parser.cpp \
    src/parserenginefactory.cpp \
    src/srtparserqt.cpp \
    src/subparserqt.cpp \
    src/subtitleengine.cpp

DISTFILES += \
    harbour-subsail.desktop \
    icons/108x108/harbour-subsail.png \
    icons/128x128/harbour-subsail.png \
    icons/172x172/harbour-subsail.png \
    icons/86x86/harbour-subsail.png \
    qml/MainPage.qml \
    qml/cover/CoverPage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/SubtitleView.qml \
    rpm/SubSail.changes.in \
    rpm/SubSail.spec \
    rpm/ViewSRT.changes.in \
    rpm/harbour-subsail.spec \
    translations/*.ts

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# to disable building translations every time, comment out the
# following CONFIG line
CONFIG += sailfishapp_i18n

# German translation is enabled as an example. If you aren't
# planning to localize your app, remember to comment out the
# following TRANSLATIONS line. And also do not forget to
# modify the localized app name in the the .desktop file.
#TRANSLATIONS += translations/

HEADERS += \
    src/parser.h \
    src/parserenginefactory.h \
    src/srtparserqt.h \
    src/subparserqt.h \
    src/subtitleengine.h \
    src/types.h
