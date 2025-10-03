import QtQuick 2.2
import Sailfish.Silica 1.0

import "pages"

ApplicationWindow {
    id: subSailMain
    property bool playing: false
    property bool loaded: false
    readonly property string appVersion: qsTr("SubSail Subtitle Viewer 0.4-1")
    property int currentTime
    property int totalTime
    initialPage: Component { SubtitleView { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations   
}
