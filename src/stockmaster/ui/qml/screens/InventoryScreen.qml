import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Flickable {
    id: root

    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + 16

    function selectedLotId(combo) {
        if (!combo || !combo.model || combo.currentIndex < 0 || combo.currentIndex >= combo.model.length) {
            return ""
        }

        return combo.model[combo.currentIndex].lotId
    }

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

    component SolidComboBox: ComboBox {
        implicitHeight: 36
        font.pixelSize: 13

        contentItem: Text {
            leftPadding: 10
            rightPadding: 28
            text: parent.displayText
            font: parent.font
            color: parent.enabled ? "#24364D" : "#6F8299"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 8
            color: parent.enabled ? "#ECF3FB" : "#E5EDF6"
            border.width: 1
            border.color: parent.activeFocus ? "#4F83CC" : "#A9C0DA"
        }
    }

    Connections {
        target: inventoryViewModel

        function onDetailChanged() {
            if (!lotAdjustCombo.model || lotAdjustCombo.model.length === 0) {
                lotAdjustCombo.currentIndex = -1
            } else if (lotAdjustCombo.currentIndex < 0 || lotAdjustCombo.currentIndex >= lotAdjustCombo.model.length) {
                lotAdjustCombo.currentIndex = 0
            }
        }
    }

    Component.onCompleted: {
        inventoryViewModel.reload()
        appViewModel.refreshOverview()
    }

    ColumnLayout {
        id: contentColumn

        width: root.width
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Tồn kho"
            subtitle: "Snapshot tồn hiện tại, lịch sử biến động, cảnh báo tồn thấp và điều chỉnh tồn có lý do."
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(740, root.height - 60)
            spacing: 12

            Rectangle {
                Layout.preferredWidth: Math.max(320, Math.min(430, root.width * 0.34))
                Layout.minimumWidth: 300
                Layout.fillHeight: true
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Text {
                        text: "Snapshot tồn hiện tại"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        SolidTextField {
                            id: searchField

                            Layout.fillWidth: true
                            placeholderText: "Tìm theo SKU, tên hoặc đơn vị"
                            text: inventoryViewModel.filterText
                            onTextChanged: inventoryViewModel.filterText = text
                        }

                        ActionButton {
                            text: "Xóa"
                            fillColor: "#607D9C"
                            enabled: searchField.text.length > 0
                            onClicked: searchField.text = ""
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: "#F6FAFF"
                        border.color: "#D7E4F2"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Text {
                                text: "Mặt hàng đang hiển thị: " + inventoryViewModel.productCount
                                color: "#1F4F8E"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }

                            Text {
                                text: "Cảnh báo tồn thấp (<= 20): " + inventoryViewModel.lowStockCount
                                color: inventoryViewModel.lowStockCount > 0 ? "#A04E19" : "#4A5B72"
                                font.pixelSize: 13
                            }
                        }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: inventoryViewModel

                        delegate: Rectangle {
                            required property string productId
                            required property string sku
                            required property string name
                            required property string unit
                            required property int totalOnHand
                            required property int lotCount
                            required property bool isLowStock

                            width: ListView.view.width
                            height: 90
                            radius: 10
                            color: productId === inventoryViewModel.selectedProductId ? "#DBE9FF" : "#F9FBFE"
                            border.width: 1
                            border.color: productId === inventoryViewModel.selectedProductId
                                ? "#8FB2E8"
                                : isLowStock
                                    ? "#E2BA85"
                                    : "#DCE6F2"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    Text {
                                        text: sku + " - " + name
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        color: "#1D3047"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: "Đơn vị: " + unit + " | Số lô: " + lotCount
                                        font.pixelSize: 12
                                        color: "#4D617B"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: "Tồn hiện tại: " + totalOnHand
                                        font.pixelSize: 12
                                        color: isLowStock ? "#A04E19" : "#4D617B"
                                        font.weight: isLowStock ? Font.DemiBold : Font.Normal
                                    }
                                }

                                Rectangle {
                                    visible: isLowStock
                                    radius: 6
                                    color: "#FFF0CB"
                                    border.color: "#D8B266"
                                    Layout.preferredWidth: warningLabel.implicitWidth + 14
                                    Layout.preferredHeight: 26

                                    Text {
                                        id: warningLabel

                                        anchors.centerIn: parent
                                        text: "Tồn thấp"
                                        color: "#7A5713"
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: inventoryViewModel.selectProduct(productId)
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.minimumWidth: 540
                Layout.fillHeight: true
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Text {
                        text: inventoryViewModel.hasSelectedProduct
                            ? inventoryViewModel.selectedProductName
                            : "Chọn sản phẩm để theo dõi tồn kho"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Rectangle {
                        id: inventorySummaryCard
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        Layout.minimumHeight: implicitHeight
                        implicitHeight: inventorySummaryContent.implicitHeight + 20
                        radius: 10
                        color: inventoryViewModel.selectedProductLowStock ? "#FFF7E8" : "#F6FAFF"
                        border.color: inventoryViewModel.selectedProductLowStock ? "#E2BA85" : "#D7E4F2"
                        clip: true

                        RowLayout {
                            id: inventorySummaryContent

                            x: 10
                            y: 10
                            width: Math.max(0, parent.width - 20)
                            spacing: 12

                            Text {
                                text: "Tồn tổng: " + inventoryViewModel.selectedProductTotalOnHand
                                color: inventoryViewModel.selectedProductLowStock ? "#A04E19" : "#1F4F8E"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }

                            Text {
                                text: inventoryViewModel.selectedProductLowStock
                                    ? "Cảnh báo: tồn đang thấp hơn hoặc bằng 20"
                                    : "Trạng thái: tồn ổn định"
                                color: inventoryViewModel.selectedProductLowStock ? "#A04E19" : "#4A5B72"
                                font.pixelSize: 13
                            }
                        }
                    }

                    Rectangle {
                        id: adjustCard
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        Layout.minimumHeight: implicitHeight
                        implicitHeight: adjustCardContent.implicitHeight + 20
                        radius: 10
                        color: "#F9FBFE"
                        border.color: "#DCE6F2"
                        clip: true

                        ColumnLayout {
                            id: adjustCardContent

                            x: 10
                            y: 10
                            width: Math.max(0, parent.width - 20)
                            spacing: 8

                            Text {
                                text: "Điều chỉnh tồn có lý do"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: "#1B2B40"
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SolidComboBox {
                                    id: lotAdjustCombo

                                    Layout.fillWidth: true
                                    enabled: inventoryViewModel.hasSelectedProduct && inventoryViewModel.selectedProductLots.length > 0
                                    model: inventoryViewModel.selectedProductLots
                                    textRole: "label"
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SolidTextField {
                                    id: adjustQtyField

                                    Layout.preferredWidth: 180
                                    Layout.minimumWidth: 150
                                    enabled: inventoryViewModel.hasSelectedProduct
                                    placeholderText: "+/- số lượng"
                                }

                                SolidTextField {
                                    id: adjustReasonField

                                    Layout.fillWidth: true
                                    enabled: inventoryViewModel.hasSelectedProduct
                                    placeholderText: "Lý do điều chỉnh"
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SolidTextField {
                                    id: adjustDateField

                                    Layout.preferredWidth: 180
                                    Layout.minimumWidth: 160
                                    enabled: inventoryViewModel.hasSelectedProduct
                                    placeholderText: "YYYY-MM-DD"
                                    text: Qt.formatDate(new Date(), "yyyy-MM-dd")
                                }

                                ActionButton {
                                    Layout.fillWidth: true
                                    text: "Áp dụng"
                                    fillColor: "#C9861A"
                                    enabled: inventoryViewModel.hasSelectedProduct && inventoryViewModel.selectedProductLots.length > 0
                                    onClicked: {
                                        if (inventoryViewModel.adjustSelectedLot(root.selectedLotId(lotAdjustCombo),
                                                                                adjustQtyField.text,
                                                                                adjustReasonField.text,
                                                                                adjustDateField.text)) {
                                            adjustQtyField.text = ""
                                            adjustReasonField.text = ""
                                            appViewModel.refreshOverview()
                                            productsViewModel.reload()
                                        }
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "Nhập số dương để cộng tồn, số âm để trừ tồn. Bắt buộc phải có lý do; hệ thống không cho điều chỉnh làm tồn âm."
                                color: "#58708F"
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 10
                            color: "#F9FBFE"
                            border.color: "#DCE6F2"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text {
                                    text: "Snapshot theo lô"
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: "#1B2B40"
                                }

                                ListView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    spacing: 6
                                    model: inventoryViewModel.selectedProductLots

                                    delegate: Rectangle {
                                        property var lotData: modelData

                                        width: ListView.view.width
                                        height: 78
                                        radius: 8
                                        color: "#FFFFFF"
                                        border.width: 1
                                        border.color: "#DCE6F2"

                                        Column {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 3

                                            Text {
                                                text: lotData.lotNo + " | Tồn: " + lotData.onHandQty
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                color: "#1D3047"
                                            }

                                            Text {
                                                text: "Ngày nhập: " + lotData.receivedDate + " | HSD: " + lotData.expiryDate
                                                font.pixelSize: 12
                                                color: "#4D617B"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }
                                        }
                                    }
                                }

                                Text {
                                    visible: inventoryViewModel.selectedProductLots.length === 0
                                    text: "Sản phẩm này chưa có lô."
                                    color: "#7286A0"
                                    font.pixelSize: 12
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 10
                            color: "#F9FBFE"
                            border.color: "#DCE6F2"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text {
                                    text: "Lịch sử biến động"
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: "#1B2B40"
                                }

                                ListView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    spacing: 6
                                    model: inventoryViewModel.selectedProductMovements

                                    delegate: Rectangle {
                                        property var movementData: modelData

                                        width: ListView.view.width
                                        height: 84
                                        radius: 8
                                        color: "#FFFFFF"
                                        border.width: 1
                                        border.color: "#DCE6F2"

                                        Column {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 3

                                            Text {
                                                text: movementData.movementDate + " | " + movementData.movementType + " | Lô: " + movementData.lotNo
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                color: "#1D3047"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            Text {
                                                text: (movementData.isPositive ? "+" : "") + movementData.qtyDelta
                                                      + " | Tồn lô sau biến động: " + movementData.lotBalanceAfter
                                                font.pixelSize: 12
                                                color: movementData.isPositive ? "#2E8B57" : "#C05A2A"
                                            }

                                            Text {
                                                text: "Lý do: " + movementData.reason
                                                font.pixelSize: 12
                                                color: "#4D617B"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }
                                        }
                                    }
                                }

                                Text {
                                    visible: inventoryViewModel.selectedProductMovements.length === 0
                                    text: "Chưa có lịch sử biến động cho sản phẩm này."
                                    color: "#7286A0"
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 8
                        visible: inventoryViewModel.lastError.length > 0
                        color: "#FFECEC"
                        border.color: "#F0BABA"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            verticalAlignment: Text.AlignVCenter
                            text: inventoryViewModel.lastError
                            color: "#8F2D2D"
                            font.pixelSize: 13
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
