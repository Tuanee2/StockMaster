import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string title: ""
    property string subtitle: ""

    implicitHeight: column.implicitHeight

    ColumnLayout {
        id: column

        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 4

        Text {
            text: root.title
            font.pixelSize: 28
            font.weight: Font.DemiBold
            color: "#142033"
        }

        Text {
            text: root.subtitle
            font.pixelSize: 14
            color: "#4A5B72"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
