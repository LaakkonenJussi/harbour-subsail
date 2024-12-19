import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {
    property double fps
    allowedOrientations: Orientation.All

    function itemTextToIndex(text) {
        switch (text) {
        case "23.976":
            return 0
        case "24":
            return 1
        case "25":
            return 2
        case "29.97":
            return 3
        case "30":
            return 4
        case "50":
            return 5
        case "59.94":
            return 6
        }

        return 0
    }

    Column {
        width: parent.width

        DialogHeader { }

        ComboBox {
            id: optionBox
            width: parent.width
            label: qsTr("Select subtitle FPS")
            value: fps > 0 ? fps.toString() : "23.976"
            currentIndex: itemTextToIndex(fps.toString())

            menu: ContextMenu {
                id: menu
                MenuItem { text: "23.976" }
                MenuItem { text: "24" }
                MenuItem { text: "25" }
                MenuItem { text: "29.97" }
                MenuItem { text: "30" }
                MenuItem { text: "50" }
                MenuItem { text: "59.94" }
            }

            onCurrentIndexChanged: {
                if (optionBox.currentItem)
                    fps = parseFloat(optionBox.currentItem.text)
            }
        }
    }

    onDone: {
        if (result == DialogResult.Accepted)
            fps = parseFloat(optionBox.currentItem.text)
        else
            fps = 0
    }
}
