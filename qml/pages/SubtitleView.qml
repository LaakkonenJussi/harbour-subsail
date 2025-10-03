import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import Nemo.KeepAlive 1.2
import Nemo.Notifications 1.0
import Nemo.Configuration 1.0

Page {
    id: subtitleViewPage
    allowedOrientations: Orientation.All

    readonly property string appRootPath: "/apps/harbour-subsail/"
    property string subtitleFilePath: ""

    property bool playing: subSailMain.playing
    property bool showFPSSelector: false

    property int time: 0
    property int oldTime: 0
    property int timeOffset: 0
    property int timeInterval: 20
    property int totalTime: 0
    property int fontSizeIncrement: 10
    property int fontsizeMax: 200
    property int fontsizeMin: 20
    property int loadStatus: -1

    property var lastUpdate
    property int applicationState: Qt.application.state
    property double fps: 0.0
    property double oldFps: 0.0

    property bool hideSliderOnPlay: hideSliderSetting.value
    property bool autoResumePlayback: autoResumePlaybackSetting.value
    property bool useOffset: useTimeOffsetSetting.value
    property int timeOffsetInterval: timeOffsetIncreaseSetting.value
    property int fontsize: subtitleSizeSetting.value
    property string fallbackCodec: fallbackCodecSetting.value

    property bool currentPlaying: false
    property bool currentKeepaliveEnabled: false
    property bool currentDisplayBlankingPrevent: false

    ListModel {
        id: subtitlesModel
        property string modelText: ""
    }

    DisplayBlanking {
        id: displayBlanking
    }

    KeepAlive {
        id: keepAlive
    }

    ConfigurationValue {
        id: hideSliderSetting
        key: appRootPath + "hideSlider"
        defaultValue: true
        onValueChanged: hideSliderOnPlay = value
    }

    ConfigurationValue {
        id: autoResumePlaybackSetting
        key: appRootPath + "autoResumePlayback"
        defaultValue: true
        onValueChanged: autoResumePlayback = value
    }

    ConfigurationValue {
        id: useTimeOffsetSetting
        key: appRootPath + "useTimeOffset"
        defaultValue: false
        onValueChanged: {
            var oldUseOffset = useOffset
            useOffset = value

            if (!oldUseOffset && useOffset) {
                timeOffset = 0
                resetDelayoffsetTimer()
            }
        }
    }

    ConfigurationValue {
        id: timeOffsetIncreaseSetting
        key: appRootPath + "timeOffsetIncrease"
        defaultValue: 100
        onValueChanged: timeOffsetInterval = value
    }

    ConfigurationValue {
        id: fallbackCodecSetting
        key: appRootPath + "fallbackCodec"
        defaultValue: "Windows-1252"
    }

    ConfigurationValue {
        id: subtitleSizeSetting
        key: appRootPath + "subtitleSize"
        defaultValue: Theme.fontSizeHuge
        onValueChanged: fontsize = value
    }

    ConfigurationValue {
        id: pauseWhenMinimizedSetting
        key: appRootPath + "pauseWhenMinimized"
        defaultValue: true
    }

    function padWithZero(value) {
        return value < 10 ? "0" + value : value.toString();
    }

    function updateSubtitle()
    {
        SubtitleEngine.setTime(time)
        subtitlesModel.clear()
        subtitlesModel.append({ modelText: SubtitleEngine.getSubtitle(0) })
    }

    function updateTime(value)
    {
        if (playing && hideSliderOnPlay)
            return

        time = value * 1000
        updateSubtitle()
    }

    function updateOffset(offsetChange)
    {
        if (useOffset) {
            timeOffset += offsetChange
        } else {
            time += offsetChange
            updateSubtitle()
            // for visuals
            timeOffset = offsetChange
        }

        resetDelayoffsetTimer()
    }

    function updateFontsize(sizeChange)
    {
        if (fontsize + sizeChange >= fontsizeMax)
            fontsize = fontsizeMax
        else if (fontsize + sizeChange <= fontsizeMin)
            fontsize = fontsizeMin
        else
            fontsize += sizeChange

        subtitleSizeSetting.value = fontsize.toString()

        resetFontTimer()
    }

    function formatTime(value)
    {
        return padWithZero(Math.floor(value / 3600000)) + ":" +
                padWithZero(Math.floor((value % 3600000) / 60000)) + ":" +
                padWithZero(Math.floor((value % 60000) / 1000))
    }

    function resetDelayoffsetTimer()
    {
        delayOffset.text = (timeOffset > 0 ? "+" : (useOffset ? "-" : "")) + timeOffset + " ms"
        delayOffset.opacity = 1

        if (delayOffsetTimer.running)
            delayOffsetTimer.restart()
    }

    function resetFPSTimer()
    {
        fpsDisplay.text = "FPS " + fps
        fpsDisplay.opacity = 1

        if (fpsDisplayTimer.running)
            fpsDisplayTimer.restart()
    }

    function getFontDisplay()
    {
        var fontText

        if (fontsize <= fontsizeMin)
            fontText = "MIN"
        else if (fontsize >= fontsizeMax)
            fontText = "MAX"
        else
            fontText = fontsize

        return "Font size " + fontText + " pt"

    }

    function resetFontTimer()
    {
        fontDisplay.text = getFontDisplay()
        fontDisplay.opacity = 1

        if (fontDisplayTimer.running)
            fontDisplayTimer.restart()
    }

    function setupSubtitles()
    {
        subSailMain.loaded = true
        totalTime = SubtitleEngine.getTotalTime()
        DisplayBlanking.preventBlanking = true
        KeepAlive.enabled = true
    }

    function clearSubtitles()
    {
        subtitleTimer.stop()
        oldTime = time
        time = 0
        SubtitleEngine.setTime(0)
        totalTime = SubtitleEngine.getTotalTime()
        slider.value = 0
        subtitlesModel.clear()
    }

    function showFPSDialog()
    {
        var playstate = subSailMain.playing

        pageStack.completeAnimation()

        if (subSailMain.playing)
            subSailMain.playing = false

        var dialogObj = pageStack.animatorPush(Qt.resolvedUrl("FPSDialog.qml"),
                {"fps": fps})
        dialogObj.pageCompleted.connect(function(dialog) {
            dialog.onAccepted.connect(function() {
                clearSubtitles()
                setupSubtitles()

                oldFps = fps
                fps = dialog.fps

                if (autoResumePlayback && playstate)
                    subSailMain.playing = playstate
            })
            dialog.onRejected.connect(function() {
                if (fps === 0) {
                    loaded = false
                    subtitleFilePath = ""
                    showFPSSelector = false
                    SubtitleEngine.unloadSubtitle();
                }

                clearSubtitles()
            })
        })
    }

    function showSubtitleSelect()
    {
        currentPlaying = subSailMain.playing
        subSailMain.playing = false
        subtitleFilePath = ""
        fps = 0.0

        var pickerObj = pageStack.animatorPush("Sailfish.Pickers.FilePickerPage", {
            nameFilters: ["*.srt", "*.sub"],
            popOnSelection: true
        })

        pickerObj.pageCompleted.connect(function(picker) {
            picker.selectedContentPropertiesChanged.connect(function() {
                subtitleFilePath = picker.selectedContentProperties['filePath']
            })
        })
    }

    function showPage(pageUrl)
    {
        var playstate = subSailMain.playing
        var oldOrientation = subtitleViewPage.orientation

        subtitleViewPage.orientation = Orientation.Portrait
        pageStack.completeAnimation()

        if (subSailMain.playing)
            subSailMain.playing = false

        var pageObj = pageStack.animatorPush(Qt.resolvedUrl(pageUrl))
        pageObj.Component.destruction.connect(function() {
            if (autoResumePlayback && playstate) {
                console.log("autoresume playback")
                lastUpdate = new Date()
                subSailMain.playing = playstate
            }

            subtitleViewPage.orientation = oldOrientation
        })
    }

    function showSettings()
    {
        showPage("SettingsPage.qml")
    }

    function showAbout()
    {
        showPage("AboutPage.qml")
    }

    function errorNotify(message, summary) {
        pageStack.completeAnimation()
        errorNotification.body = message
        errorNotification.summary = summary
        errorNotification.publish()
        errorNotification.expireTimeout = 5000
        clearSubtitles()
    }

    function errorNotifyLoadFailure(message)
    {
        errorNotify(message, qsTr("Subtitle load failure"))
    }

    function checkSubtitleLoadResult()
    {
        switch (loadStatus) {
        case SubtitleEngine.SUBTITLE_LOAD_STATUS_FAILURE:
        case 6:
            errorNotifyLoadFailure(qsTr("Failed to load file"))
            break
        case SubtitleEngine.SUBTITLE_LOAD_STATUS_PARSE_FAILURE:
        case 5:
            errorNotifyLoadFailure(qsTr("Failed to parse file"))
            break
        case SubtitleEngine.SUBTITLE_LOAD_STATUS_NOT_SUPPORTED:
        case 4:
            errorNotifyLoadFailure(qsTr("Subtitle type not supported"))
            break
        case SubtitleEngine.SUBTITLE_LOAD_STATUS_ACCESS_DENIED:
        case 3:
            errorNotifyLoadFailure(qsTr("File access denied"))
            break
        case SubtitleEngine.SUBTITLE_LOAD_STATUS_FILE_NOT_FOUND:
        case 2:
            errorNotifyLoadFailure(qsTr("File not found"))
            break
        case SubtitleEngine.SUBTITLE_LOAD_STATUS_OK_NEED_FPS:
        case 1:
            clearSubtitles()
            oldTime = 0
            showFPSDialog()
            showFPSSelector = true
            break
        case SubtitleEngine.SUBTITLE_LOAD_STATUS_OK:
        case 0:
            showFPSSelector = false
            clearSubtitles()
            setupSubtitles()
            break
        default:
            console.log("subtitle load status out of bounds:", loadStatus)
            errorNotifyLoadFailure(qsTr("Unknown error"))
            break
        }
    }

    function togglePlayPause()
    {
        if (!loaded)
            return

        if (!subSailMain.playing && !subtitleTimer.running && time >= totalTime) {
            time = 0
            SubtitleEngine.setTime(0)
            slider.value = 0
        }

        subSailMain.playing = !subSailMain.playing
    }

    function timeSliderVisible()
    {
        if (!subSailMain.loaded)
            return false
        else if (subSailMain.playing && hideSliderOnPlay)
            return false

        return true
    }

    function loadSubtitle()
    {
        if (subtitleFilePath === "")
            return

        loadStatus = SubtitleEngine.loadSubtitle(subtitleFilePath)
        pageStack.completeAnimation()
        loadStatusChangedTimer.start()
    }

    onFpsChanged: {
        SubtitleEngine.updateFps(fps)
        resetFPSTimer()
        totalTime = SubtitleEngine.getTotalTime()

        if (oldTime > totalTime) {
            pageStack.completeAnimation()
            clearSubtitles()
        } else {
            if (oldFps > 0 && oldTime > 0) {
                var newTime = Math.round(oldTime / oldFps * fps)
                console.log("new time", newTime, "total time", totalTime)
                if (newTime < totalTime) {
                    time = newTime;
                    updateSubtitle()
                }
            }
        }
    }

    onTimeChanged: {
        slider.value = time ? time / 1000 : 0
        subSailMain.currentTime = time
    }

    onTotalTimeChanged: {
        subSailMain.totalTime = totalTime
    }

    onTimeOffsetChanged: {
        SubtitleEngine.setOffset(timeOffset)
        if (useOffset)
            updateSubtitle()
    }

    onSubtitleFilePathChanged: {
        loadSubtitle()
    }

    onPlayingChanged: {
        subtitleTimer.running = playing
        if (playing)
            lastUpdate = new Date()
    }

    onFallbackCodecChanged: {
        switch (SubtitleEngine.setFallbackCodec(fallbackCodec)) {
        case -2: //-ENOENT
            errorNotify(qsTr("Failed to change fallback codec to") + fallbackCodec, qsTr("Codec empty"))
            break
        case -22: //-EINVAL
            errorNotify(qsTr("Failed to change fallback codec") + fallbackCodec, qsTr("Invalid codec"))
            break
        case -144: //-EALREADY
            break
        case 0:
            console.log("Codec changed to", fallbackCodec)

            if (subSailMain.loaded) {
                console.log("Reload subtitle", subtitleFilePath)
                loadSubtitle()
            }
        }
    }

    onApplicationStateChanged: {
        if (applicationState === Qt.ApplicationActive) {

            if (autoResumePlayback && currentPlaying && !subSailMain.playing)
                togglePlayPause()

            displayBlanking.preventBlanking = currentDisplayBlankingPrevent
            keepAlive.enabled = currentKeepaliveEnabled
        } else {
            currentPlaying = subSailMain.playing
            currentDisplayBlankingPrevent = displayBlanking.preventBlanking
            currentKeepaliveEnabled = keepAlive.enabled

            if (pauseWhenMinimizedSetting.value === true && subSailMain.playing)
                togglePlayPause()

            displayBlanking.preventBlanking = false
            keepAlive.enabled = false

        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: height

        Notification {
            id: errorNotification
            summary: qsTr("Failed to load subtitle")
            body: ""
        }

        BackgroundItem {
            id: background
            width: subtitleViewPage.width
            height: subtitleViewPage.height - controls.height
            enabled: subSailMain.loaded

            onDoubleClicked: togglePlayPause()
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: showAbout()
            }

            MenuItem {
                text: qsTr("Settings")
                onClicked: showSettings()
            }

            MenuItem {
                text: qsTr("Select Subtitle")
                onClicked: showSubtitleSelect()
            }
            MenuItem {
                text: qsTr("Select FPS")
                visible: showFPSSelector
                onClicked: showFPSDialog()
            }
        }

        Timer {
            id: loadStatusChangedTimer
            interval: 1
            repeat: false

            onTriggered: {
                checkSubtitleLoadResult()
                loadStatusChangedTimer.stop()
            }

        }

        Timer {
            id: subtitleTimer
            interval: timeInterval
            repeat: true
            running: subSailMain.playing
            property string prevSub

            onTriggered: {
                var now = new Date()
                var expired = now - lastUpdate
                lastUpdate = now

                var nextSub = SubtitleEngine.getSubtitle(expired)
                if (nextSub != prevSub) {
                    prevSub = nextSub
                    subtitlesModel.clear()
                    subtitlesModel.append({ modelText: nextSub})
                }
                time += expired

                if (time >= totalTime) {
                    subSailMain.playing = false
                    subtitleTimer.stop()
                    subtitleTimer.running = false
                    displayBlanking.preventBlanking = false
                    keepAlive.enabled = false
                } else {
                    displayBlanking.preventBlanking = true
                    keepAlive.enabled = true
                }
            }
        }

        Timer {
            id: delayOffsetTimer
            interval: 50
            repeat: true
            running: delayOffset.opacity > 0
            onTriggered: delayOffset.opacity -= 0.02
        }

        Timer {
            id: fpsDisplayTimer
            interval: 50
            repeat: true
            running: fpsDisplay.opacity > 0
            onTriggered: fpsDisplay.opacity -= 0.02
        }

        Timer {
            id: fontDisplayTimer
            interval: 50
            repeat: true
            running: fontDisplay.opacity > 0
            onTriggered: fontDisplay.opacity -= 0.02
        }

        Item {
            id: mainView
            anchors.fill: parent

            Label {
                id: delayOffset
                text: timeOffset + " ms"
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: Theme.paddingLarge
                anchors.topMargin: Theme.paddingLarge
                horizontalAlignment: Text.AlignLeft
                font.pixelSize: Theme.fontSizeSmall
                visible: opacity > 0
                opacity: 0
            }

            BackgroundItem {
                width: fpsDisplay.width
                height: fpsDisplay.height
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.rightMargin: Theme.paddingLarge
                anchors.topMargin: Theme.paddingLarge

                Label {
                    id: fpsDisplay
                    text: "FPS " + fps
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: Theme.fontSizeSmall
                    visible: opacity > 0
                    opacity: 0
                }

                onDoubleClicked: {
                    if (showFPSSelector)
                        resetFPSTimer()
                }
            }

            BackgroundItem {
                width: fontDisplay.width
                height: fontDisplay.height
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.rightMargin: Theme.paddingLarge
                anchors.topMargin: Theme.paddingLarge

                Label {
                    id: fontDisplay
                    text: getFontDisplay()
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: Theme.fontSizeSmall
                    visible: opacity > 0
                    opacity: 0
                }

                onDoubleClicked: resetFontTimer()
            }

            Column {
                id: controls
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.95
                spacing: Theme.paddingLarge

                Row {
                    id: playtime
                    width: parent.width + Theme.horizontalPageMargin * 2
                    anchors.horizontalCenter: parent.horizontalCenter

                    Slider {
                        id: slider
                        value: time ? time / 1000 : 0
                        minimumValue: 0
                        maximumValue: totalTime ? totalTime / 1000 : 1
                        stepSize: 1
                        width: parent.width
                        label: formatTime(time) + " / " + formatTime(totalTime)
                        onPositionChanged: updateTime(value)
                        onReleased: updateTime(value)
                        onPressed: updateTime(value)
                        enabled: timeSliderVisible()
                        opacity: subSailMain.playing && hideSliderOnPlay ? 0.5 : 1
                    }
                }

                Row {
                    id: iconButtons
                    spacing: Theme.paddingLarge
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottomMargin: Theme.paddingMedium
                    height: play.height + Theme.horizontalPageMargin

                    IconButton {
                        icon.source: "image://theme/icon-m-remove"
                        enabled: subSailMain.loaded
                        onClicked: updateFontsize(-fontSizeIncrement)

                    }
                    IconButton {
                        icon.source: "image://theme/icon-m-previous"
                        enabled: subSailMain.loaded
                        onClicked: updateOffset(-timeOffsetInterval)
                    }
                    IconButton {
                        id: play
                        icon.source: subSailMain.playing ?
                                         "image://theme/icon-l-pause" :
                                         "image://theme/icon-l-play"
                        onClicked: togglePlayPause()
                        enabled: subSailMain.loaded
                    }
                    IconButton {
                        icon.source: "image://theme/icon-m-next"
                        enabled: subSailMain.loaded
                        onClicked: updateOffset(timeOffsetInterval)
                    }
                    IconButton {
                        icon.source: "image://theme/icon-m-add"
                        enabled: subSailMain.loaded
                        onClicked: updateFontsize(fontSizeIncrement)
                    }
                }
            }

            Item {
                id: subview
                anchors {
                    top: parent.top
                    topMargin: Theme.paddingLarge * 3
                    left: parent.left
                    right: parent.right
                    bottom: controls.top
                }

                Repeater {
                    model: subtitlesModel
                    delegate: Label {
                        text: modelText
                        textFormat: Text.RichText
                        wrapMode: Text.Wrap
                        anchors.fill: parent
                        anchors.margins: Theme.paddingLarge
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: palette.highlightColor
                        font.pixelSize: fontsize
                    }
                }
            }
        }
    }


}
