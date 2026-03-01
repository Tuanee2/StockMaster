import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Flickable {
    id: root

    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + 16

    component ActionButton: Button {
        id: control

        property color fillColor: "#2D6CDF"

        implicitHeight: 36
        padding: 12

        background: Rectangle {
            radius: 8
            color: control.enabled ? control.fillColor : "#B8C6D6"
            border.width: 1
            border.color: control.enabled ? Qt.darker(control.fillColor, 1.15) : "#9AAEC1"
        }

        contentItem: Text {
            text: control.text
            color: "#FFFFFF"
            font.pixelSize: 13
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component SolidTextField: TextField {
        implicitHeight: 36
        font.pixelSize: 13
        color: enabled ? "#24364D" : "#6F8299"
        placeholderTextColor: "#7286A0"
        selectionColor: "#9DC0EA"
        selectedTextColor: "#132338"

        background: Rectangle {
            radius: 8
            color: parent.enabled ? "#ECF3FB" : "#E5EDF6"
            border.width: 1
            border.color: parent.activeFocus ? "#4F83CC" : "#A9C0DA"
        }
    }

    Connections {
        target: appViewModel

        function onCurrentSectionChanged() {
            if (appViewModel.currentSection === "Reports") {
                reportsViewModel.reloadAll()
            }
        }
    }

    Connections {
        target: reportsViewModel

        function onSalesChanged() {
            salesFromField.text = reportsViewModel.salesFromDate
            salesToField.text = reportsViewModel.salesToDate
        }

        function onMovementChanged() {
            movementFromField.text = reportsViewModel.movementFromDate
            movementToField.text = reportsViewModel.movementToDate
        }
    }

    Component.onCompleted: reportsViewModel.reloadAll()

    ColumnLayout {
        id: contentColumn

        width: root.width
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Báo cáo"
            subtitle: "Sales summary theo kỳ, debt aging, inventory movement và export CSV/PDF."
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 10
            visible: reportsViewModel.lastExportPath.length > 0
            color: "#EAF7EC"
            border.color: "#BED8C1"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Text {
                    text: "Đã export:"
                    color: "#2E6B36"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                Text {
                    Layout.fillWidth: true
                    text: reportsViewModel.lastExportPath
                    color: "#355E3B"
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            Layout.minimumHeight: implicitHeight
            implicitHeight: salesContent.implicitHeight + 24
            radius: 12
            color: "#FFFFFF"
            border.color: "#D8E2EE"
            clip: true

            ColumnLayout {
                id: salesContent

                x: 12
                y: 12
                width: Math.max(0, parent.width - 24)
                spacing: 10

                Text {
                    text: "Sales Summary Theo Kỳ"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: "#1B2B40"
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    SolidTextField {
                        id: salesFromField

                        width: 150
                        placeholderText: "Từ ngày"
                        text: reportsViewModel.salesFromDate
                    }

                    SolidTextField {
                        id: salesToField

                        width: 150
                        placeholderText: "Đến ngày"
                        text: reportsViewModel.salesToDate
                    }

                    ActionButton {
                        text: "Tải số liệu"
                        fillColor: "#2D6CDF"
                        onClicked: {
                            if (reportsViewModel.loadSalesSummary(salesFromField.text, salesToField.text)) {
                                salesFromField.text = reportsViewModel.salesFromDate
                                salesToField.text = reportsViewModel.salesToDate
                            }
                        }
                    }

                    ActionButton {
                        text: "CSV"
                        fillColor: "#607D9C"
                        onClicked: reportsViewModel.exportSalesSummary("csv")
                    }

                    ActionButton {
                        text: "PDF"
                        fillColor: "#7B6FD0"
                        onClicked: reportsViewModel.exportSalesSummary("pdf")
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: width >= 840 ? 4 : 2
                    rowSpacing: 8
                    columnSpacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#F7FBFF"
                        border.color: "#D7E4F2"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: "Số đơn"; font.pixelSize: 12; color: "#58708F" }
                            Text {
                                text: reportsViewModel.salesOrderCount.toString()
                                font.pixelSize: 22
                                font.weight: Font.DemiBold
                                color: "#20324A"
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#F7FBFF"
                        border.color: "#D7E4F2"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: "Doanh số"; font.pixelSize: 12; color: "#58708F" }
                            Text {
                                text: reportsViewModel.salesTotalVndText
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                color: "#20324A"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#F7FBFF"
                        border.color: "#D7E4F2"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: "Đã thu"; font.pixelSize: 12; color: "#58708F" }
                            Text {
                                text: reportsViewModel.salesCollectedVndText
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                color: "#2E8B57"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#FFF8E8"
                        border.color: "#E6C781"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: "Còn phải thu"; font.pixelSize: 12; color: "#7A5713" }
                            Text {
                                text: reportsViewModel.salesOutstandingVndText
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                color: "#8A5A10"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    radius: 10
                    color: "#F9FBFE"
                    border.color: "#DCE6F2"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Text {
                            text: "Chi tiết theo ngày"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: "#1B2B40"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 6
                            model: reportsViewModel.salesBuckets

                            delegate: Rectangle {
                                property var bucketData: modelData

                                width: ListView.view.width
                                height: 58
                                radius: 8
                                color: "#FFFFFF"
                                border.width: 1
                                border.color: "#DCE6F2"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 8

                                    Text {
                                        Layout.preferredWidth: 88
                                        text: bucketData.bucketDate
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: "#20324A"
                                    }

                                    Text {
                                        Layout.preferredWidth: 72
                                        text: "Đơn: " + bucketData.orderCount
                                        font.pixelSize: 12
                                        color: "#536883"
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: "Doanh số: " + bucketData.salesText
                                        font.pixelSize: 12
                                        color: "#2D6CDF"
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: "Thu: " + bucketData.collectedText
                                        font.pixelSize: 12
                                        color: "#CC7A1A"
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            Layout.minimumHeight: implicitHeight
            implicitHeight: debtContent.implicitHeight + 24
            radius: 12
            color: "#FFFFFF"
            border.color: "#D8E2EE"
            clip: true

            ColumnLayout {
                id: debtContent

                x: 12
                y: 12
                width: Math.max(0, parent.width - 24)
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "Debt Aging Report"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Item { Layout.fillWidth: true }

                    ActionButton {
                        text: "Làm mới"
                        fillColor: "#2D6CDF"
                        onClicked: reportsViewModel.loadDebtAging()
                    }

                    ActionButton {
                        text: "CSV"
                        fillColor: "#607D9C"
                        onClicked: reportsViewModel.exportDebtAging("csv")
                    }

                    ActionButton {
                        text: "PDF"
                        fillColor: "#7B6FD0"
                        onClicked: reportsViewModel.exportDebtAging("pdf")
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: width >= 840 ? 4 : 2
                    rowSpacing: 8
                    columnSpacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#F7FBFF"
                        border.color: "#D7E4F2"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: "0-30 ngày"; font.pixelSize: 12; color: "#58708F" }
                            Text {
                                text: reportsViewModel.debtCurrentVndText
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                color: "#20324A"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#FFF8E8"
                        border.color: "#E6C781"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: "31-60 ngày"; font.pixelSize: 12; color: "#7A5713" }
                            Text {
                                text: reportsViewModel.debt31To60VndText
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                color: "#8A5A10"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#FFF0F0"
                        border.color: "#E6B3B3"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: ">60 ngày"; font.pixelSize: 12; color: "#9C3C3C" }
                            Text {
                                text: reportsViewModel.debt61PlusVndText
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                color: "#9C3C3C"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        radius: 10
                        color: "#F6FAFF"
                        border.color: "#BFD4EB"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text { text: "Tổng công nợ"; font.pixelSize: 12; color: "#305070" }
                            Text {
                                text: reportsViewModel.debtTotalVndText
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                color: "#1F4F8E"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    radius: 10
                    color: "#F9FBFE"
                    border.color: "#DCE6F2"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Text {
                            text: "Đối chiếu theo khách"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: "#1B2B40"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 6
                            model: reportsViewModel.debtRows

                            delegate: Rectangle {
                                property var rowData: modelData

                                width: ListView.view.width
                                height: 62
                                radius: 8
                                color: "#FFFFFF"
                                border.width: 1
                                border.color: "#DCE6F2"

                                Column {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 2

                                    Text {
                                        text: rowData.code + " - " + rowData.name
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: "#20324A"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }

                                    Text {
                                        text: "0-30: " + rowData.currentText
                                              + " | 31-60: " + rowData.bucket31To60Text
                                              + " | >60: " + rowData.bucket61PlusText
                                              + " | Tổng: " + rowData.totalText
                                        font.pixelSize: 12
                                        color: "#536883"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            Layout.minimumHeight: implicitHeight
            implicitHeight: movementContent.implicitHeight + 24
            radius: 12
            color: "#FFFFFF"
            border.color: "#D8E2EE"
            clip: true

            ColumnLayout {
                id: movementContent

                x: 12
                y: 12
                width: Math.max(0, parent.width - 24)
                spacing: 10

                Text {
                    text: "Inventory Movement"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: "#1B2B40"
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    SolidTextField {
                        id: movementKeywordField

                        width: Math.max(220, Math.min(340, parent.width * 0.4))
                        placeholderText: "Lọc theo SKU, sản phẩm, lô hoặc lý do"
                    }

                    SolidTextField {
                        id: movementFromField

                        width: 150
                        placeholderText: "Từ ngày"
                        text: reportsViewModel.movementFromDate
                    }

                    SolidTextField {
                        id: movementToField

                        width: 150
                        placeholderText: "Đến ngày"
                        text: reportsViewModel.movementToDate
                    }

                    ActionButton {
                        text: "Tải số liệu"
                        fillColor: "#2D6CDF"
                        onClicked: {
                            if (reportsViewModel.loadInventoryMovement(movementKeywordField.text,
                                                                       movementFromField.text,
                                                                       movementToField.text)) {
                                movementFromField.text = reportsViewModel.movementFromDate
                                movementToField.text = reportsViewModel.movementToDate
                            }
                        }
                    }

                    ActionButton {
                        text: "CSV"
                        fillColor: "#607D9C"
                        onClicked: reportsViewModel.exportInventoryMovement("csv")
                    }

                    ActionButton {
                        text: "PDF"
                        fillColor: "#7B6FD0"
                        onClicked: reportsViewModel.exportInventoryMovement("pdf")
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 260
                    radius: 10
                    color: "#F9FBFE"
                    border.color: "#DCE6F2"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Text {
                            text: "Lịch sử biến động tồn theo bộ lọc"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: "#1B2B40"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 6
                            model: reportsViewModel.inventoryMovementRows

                            delegate: Rectangle {
                                property var movementData: modelData

                                width: ListView.view.width
                                height: 72
                                radius: 8
                                color: "#FFFFFF"
                                border.width: 1
                                border.color: "#DCE6F2"

                                Column {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 2

                                    Text {
                                        text: movementData.movementDate + " | " + movementData.sku + " - " + movementData.productName
                                              + " | Lô: " + movementData.lotNo
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: "#20324A"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }

                                    Text {
                                        text: movementData.movementType
                                              + " | " + movementData.qtyDeltaText
                                              + " | Tồn sau: " + movementData.lotBalanceAfter
                                        font.pixelSize: 12
                                        color: movementData.qtyDelta >= 0 ? "#2E8B57" : "#C05A2A"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }

                                    Text {
                                        text: "Lý do: " + movementData.reason
                                        font.pixelSize: 12
                                        color: "#536883"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            radius: 8
            visible: reportsViewModel.lastError.length > 0
            color: "#FFECEC"
            border.color: "#F0BABA"

            Text {
                anchors.fill: parent
                anchors.margins: 10
                verticalAlignment: Text.AlignVCenter
                text: reportsViewModel.lastError
                color: "#8F2D2D"
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }
    }
}
