import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {

    id: settingsPage
    allowedOrientations: Orientation.Portrait
    readonly property string appRootPath: "/apps/harbour-subsail/"

    function itemTextToIndex(text) {
        switch (text) {
        case "Big5":
            return 0
        case "CP949":
            return 1
        case "EUC-JP":
            return 2
        case "GB18030":
            return 3
        case "ISO 8859-1":
            return 4
        case "ISO 8859-15":
            return 5
        case "KOI8-R":
            return 6
        case "Shift-JIS":
            return 7
        case "Windows-1252":
            return 8
        }

        return 0
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height
        contentWidth: width

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                id: pageHeader
                title: qsTr("Settings")
                description: subSailMain.appVersion
                rightMargin: Theme.horizontalPageMargin + height - logo.padding

                Image {
                    id: logo
                    readonly property int padding: Theme.paddingLarge
                    readonly property int size: pageHeader.height - 2 * padding
                    x: pageHeader.width - width - Theme.horizontalPageMargin
                    y: padding
                    height: size
                    width: size
                    sourceSize: Qt.size(size,size)
                    source: "/usr/share/icons/hicolor/172x172/apps/harbour-subsail.png"
                    smooth: true
                    opacity: 1.0
                }
            }

            SectionHeader {
                text: qsTr("Playback options")
            }

            TextSwitch {
                text: qsTr("Slider enabled during playback")
                description: qsTr("Enable slider when playing subtitle")
                automaticCheck: false
                checked: !hideSlider.value
                onClicked: hideSlider.value = checked

                ConfigurationValue {
                    id: hideSlider
                    key: appRootPath + "hideSlider"
                    defaultValue: true
                }
            }

            TextSwitch {
                text: qsTr("Autoresume playback")
                description: qsTr("Resume playback when returning to main view")
                automaticCheck: false
                checked: autoResumePlayback.value
                onClicked: autoResumePlayback.value = !checked

                ConfigurationValue {
                    id: autoResumePlayback
                    key: appRootPath + "autoResumePlayback"
                    defaultValue: false
                }
            }

            TextSwitch {
                text: qsTr("Pause playback when minimized")
                description: qsTr("Does not affect display blanking")
                automaticCheck: false
                checked: pauseWhenMinimized.value
                onClicked: pauseWhenMinimized.value = !checked

                ConfigurationValue {
                    id: pauseWhenMinimized
                    key: appRootPath + "pauseWhenMinimized"
                    defaultValue: true
                }
            }

            SectionHeader {
                text: qsTr("Time options")
            }

            TextSwitch {
                text: qsTr("Alter time directly")
                description: qsTr("Alter time instead of offset with rev/ff")
                automaticCheck: false
                checked: !useTimeOffset.value
                onClicked: useTimeOffset.value = checked

                ConfigurationValue {
                    id: useTimeOffset
                    key: appRootPath + "useTimeOffset"
                    defaultValue: false
                }
            }

            Slider {
                visible: opacity > 0
                opacity: 1.0
                width: parent.width
                minimumValue: 100
                maximumValue: 2000
                value: timeOffsetIncrease.value ? timeOffsetIncrease.value : 100
                stepSize: 100
                label: qsTr("Time/offset increase (ms)")
                valueText: value + "ms"
                onPositionChanged: timeOffsetIncrease.value = value
                onReleased: timeOffsetIncrease.value = value
                onPressed: timeOffsetIncrease.value = value
                FadeAnimation on opacity { }

                ConfigurationValue {
                    id: timeOffsetIncrease
                    key: appRootPath + "timeOffsetIncrease"
                    defaultValue: 100
                }
            }

            SectionHeader {
                text: qsTr("Subtitle options")
            }

            Slider {
                visible: opacity > 0
                opacity: 1.0
                width: parent.width
                minimumValue: 10
                maximumValue: 200
                value: subtitleSize.value ? subtitleSize.value : 100
                stepSize: 20
                label: qsTr("Subtitle text size")
                valueText: value + " pt"
                onPositionChanged: subtitleSize.value = value
                onReleased: subtitleSize.value = value
                onPressed: subtitleSize.value = value
                FadeAnimation on opacity { }

                ConfigurationValue {
                    id: subtitleSize
                    key: appRootPath + "subtitleSize"
                    defaultValue: Theme.fontSizeHuge
                }
            }

            ComboBox {
                width: parent.width
                label: qsTr("Default fallback codec")
                description: qsTr("For files without BOM. Current subtitle is reloaded.")
                value: fallbackCodec.value
                currentIndex: itemTextToIndex(fallbackCodec.value)

                menu: ContextMenu {
                    MenuItem { text: "Big5" }
                    MenuItem { text: "CP949" }
                    MenuItem { text: "EUC-JP" }
                    MenuItem { text: "GB18030" }
                    MenuItem { text: "ISO 8859-1" }
                    MenuItem { text: "ISO 8859-15" }
                    MenuItem { text: "KOI8-R" }
                    MenuItem { text: "Shift-JIS" }
                    MenuItem { text: "Windows-1252" }
                }

                onValueChanged: fallbackCodec.value = value
                onCurrentItemChanged: fallbackCodec.value = currentItem.text

                ConfigurationValue {
                    id: fallbackCodec
                    key: appRootPath + "fallbackCodec"
                    defaultValue: "Windows-1252"
                }
            }
        }
    }
}
