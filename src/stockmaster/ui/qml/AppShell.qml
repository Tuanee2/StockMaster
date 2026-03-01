import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Item {
    id: root

    readonly property var sectionLabels: ({
        "Dashboard": "Tổng quan",
        "Customers": "Khách hàng",
        "Products": "Sản phẩm",
        "Orders": "Đơn hàng",
        "Inventory": "Tồn kho",
        "Payments": "Thanh toán",
        "Reports": "Báo cáo",
        "Settings": "Cài đặt"
    })

    function sectionLabel(sectionKey) {
        return sectionLabels[sectionKey] || sectionKey
    }

    Dialog {
        id: exitConfirmDialog

        parent: Overlay.overlay
        modal: true
        focus: true
        width: 372
        padding: 18
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)

        Overlay.modal: Rectangle {
            color: "#13233866"
        }

        background: Rectangle {
            radius: 14
            color: "#FFFFFF"
            border.width: 1
            border.color: "#D8E2EE"
        }

        contentItem: Column {
            spacing: 14
            width: exitConfirmDialog.availableWidth

            Rectangle {
                width: parent.width
                height: 6
                radius: 3
                color: "#F2CACA"
            }

            Text {
                width: parent.width
                text: "Xác nhận thoát"
                font.pixelSize: 17
                font.weight: Font.DemiBold
                color: "#1B2B40"
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "Bạn có chắc muốn thoát chương trình không?"
                font.pixelSize: 14
                font.weight: Font.Medium
                color: "#20324A"
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "Bạn có thể bấm Hủy, nhấn Esc hoặc click ra ngoài để tiếp tục làm việc."
                font.pixelSize: 12
                color: "#5D718A"
            }

            Row {
                width: parent.width
                spacing: 10

                Button {
                    id: stayButton

                    width: (parent.width - parent.spacing) / 2
                    implicitHeight: 40
                    text: "Hủy"
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 10
                        color: stayButton.down
                            ? "#D7E2EE"
                            : (stayButton.hovered ? "#F2F6FB" : "#E8EFF7")
                        border.width: 1
                        border.color: "#B7C9DC"
                    }

                    contentItem: Text {
                        text: stayButton.text
                        color: "#24405F"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    onClicked: exitConfirmDialog.close()
                }

                Button {
                    id: confirmExitButton

                    width: (parent.width - parent.spacing) / 2
                    implicitHeight: 40
                    text: "Thoát"
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 10
                        color: confirmExitButton.down
                            ? "#B64040"
                            : (confirmExitButton.hovered ? "#E06262" : "#D95353")
                        border.width: 1
                        border.color: "#A53B3B"
                    }

                    contentItem: Text {
                        text: confirmExitButton.text
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    onClicked: {
                        exitConfirmDialog.close()
                        Qt.quit()
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#ECF2F8" }
            GradientStop { position: 1.0; color: "#DCE8F4" }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 248
            color: "#F6F9FD"
            border.color: "#D8E2EE"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 88
                    radius: 12
                    color: "#FFFFFF"
                    border.color: "#D8E2EE"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 4

                        Text {
                            text: "StockMaster"
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                            color: "#142033"
                        }

                        Text {
                            text: appViewModel.databaseReady
                                ? "DB: " + appViewModel.databaseBackend
                                : "DB chưa sẵn sàng"
                            font.pixelSize: 13
                            color: appViewModel.databaseReady ? "#2B5F2C" : "#A04343"
                        }
                    }
                }

                ListView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    model: appViewModel.sections
                    spacing: 6
                    clip: true

                    delegate: Rectangle {
                        required property string modelData

                        width: ListView.view.width
                        height: 42
                        radius: 10
                        color: modelData === appViewModel.currentSection ? "#DCEBFF" : "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            text: root.sectionLabel(modelData)
                            font.pixelSize: 14
                            font.weight: modelData === appViewModel.currentSection
                                ? Font.DemiBold
                                : Font.Normal
                            color: "#243348"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: appViewModel.currentSection = modelData
                        }
                    }
                }

                Button {
                    id: exitButton

                    Layout.fillWidth: true
                    implicitHeight: 40
                    text: "Thoát chương trình"
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 10
                        color: exitButton.down
                            ? "#B64040"
                            : (exitButton.hovered ? "#E06262" : "#D95353")
                        border.width: 1
                        border.color: "#A53B3B"
                    }

                    contentItem: Text {
                        text: exitButton.text
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    onClicked: exitConfirmDialog.open()
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "transparent"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    radius: 12
                    color: "#FFFFFF"
                    border.color: "#D8E2EE"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        Text {
                            text: root.sectionLabel(appViewModel.currentSection)
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                            color: "#142033"
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: "Làm mới"
                            onClicked: {
                                appViewModel.refreshOverview()
                                if (appViewModel.currentSection === "Reports") {
                                    reportsViewModel.reloadAll()
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 14
                    color: "#F9FBFE"
                    border.color: "#D8E2EE"

                    StackLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        currentIndex: appViewModel.sections.indexOf(appViewModel.currentSection)

                        DashboardScreen {}
                        CustomersScreen {}
                        ProductsScreen {}
                        OrdersScreen {}
                        InventoryScreen {}
                        PaymentsScreen {}
                        ReportsScreen {}
                        SettingsScreen {}
                    }
                }
            }
        }
    }
}
