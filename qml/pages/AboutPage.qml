import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: settingsPage
    allowedOrientations: Orientation.Portrait
    readonly property string appRootPath: "/apps/harbour-subsail/"

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
                title: qsTr("About")
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
                text: "SubSail"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - x*2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall

                text: qsTr("SubSail is a simple subtitle viewer for Sailfish OS. Pulley menu can be used to load a subtitle file and the slider and/or forward/backward buttons can be used to manually synchronize a subtitle.")
            }

            SectionHeader {
                text: qsTr("Supported files and formats")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - x*2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall

                text: qsTr("The files that are supported are .srt and .sub subtitle files.\n\nThe most common encodings are supported, and if the encoding cannot be detected from the subtitle file the fallback codec is used. The fallback codec can be changed in the settings.\n\nFor .sub files FPS may be detected automatically but otherwise the FPS is prompted to be selected.\n\nAny other than italic, bold or underline text decorations are ignored.")
            }

            SectionHeader {
                text: qsTr("Features")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - x*2
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall

                text: qsTr("Time change is supported as a direct time forward/backward adjustment or using an offset with adjustable increases.\n\nSubtitle font can be changed both in main view and settings.\n\nThe slider can be enabled in settings to be used while playing a subtitle.\n\nAutomatic resume of playback after returning to main view can be changed in settings as well as can be the pause when application is minimized.")
            }

            SectionHeader {
                text: qsTr("Sources")
            }

            BackgroundItem {
                id: clickItem
                width: parent.width
                height: Theme.itemSizeMedium

                Row {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2*x
                    height: parent.height
                    spacing:Theme.paddingMedium

                    Label {
                        width: parent.width - parent.height - parent.spacing
                        wrapMode: Text.WordWrap
                        font.pixelSize: Theme.fontSizeSmall

                        text: "https://github.com/LaakkonenJussi/harbour-subsail"
                        color: clickItem.pressed ? Theme.highlightColor : Theme.primaryColor
                    }
                }

                onClicked: Qt.openUrlExternally("https://github.com/LaakkonenJussi/harbour-subsail")
            }
        }
    }
}
