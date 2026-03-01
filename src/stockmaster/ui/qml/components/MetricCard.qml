import QtQuick

Rectangle {
    id: root

    property string title: ""
    property string value: ""
    property color accent: "#2D6CDF"

    radius: 12
    border.width: 1
    border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.25)

    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.20)
        }
        GradientStop {
            position: 1.0
            color: "#FFFFFF"
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.top: parent.top
        anchors.topMargin: 12
        text: root.title
        font.pixelSize: 13
        color: "#29425F"
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        text: root.value
        font.pixelSize: 26
        font.weight: Font.DemiBold
        color: "#142033"
    }
}
