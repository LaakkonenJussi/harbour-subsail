import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    property int currentTime: subSailMain.currentTime
    property int totalTime: subSailMain.totalTime

    function padWithZero(value) {
        return value < 10 ? "0" + value : value.toString();
    }

    function formatTime(value)
    {
        return padWithZero(Math.floor(value / 3600000)) + ":" +
                padWithZero(Math.floor((value % 3600000) / 60000)) + ":" +
                padWithZero(Math.floor((value % 60000) / 1000))
    }

    Column {
        anchors.centerIn: parent
        Label {
            id: label
            font.pixelSize: Theme.fontSizeExtraLarge
            text: qsTr("SubSail")
        }

        Label {
            id: time
            text: formatTime(currentTime) + "/" + formatTime(totalTime)
        }
    }

    CoverActionList {
        id: coverAction

        CoverAction {
            iconSource: subSailMain.playing && subSailMain.loaded ?
                            "image://theme/icon-cover-pause" :
                            (subSailMain.loaded ?
                                "image://theme/icon-cover-play" :
                                "image://theme/icon-cover-stop")
            onTriggered: {
                if (subSailMain.loaded)
                    subSailMain.playing = !subSailMain.playing
            }
        }
    }
}
