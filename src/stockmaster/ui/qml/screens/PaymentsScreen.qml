import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Flickable {
    id: root

    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + 16

    function selectedId(combo) {
        if (!combo || !combo.model || combo.currentIndex < 0 || combo.currentIndex >= combo.model.length) {
            return ""
        }

        return combo.model[combo.currentIndex].id
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
        target: paymentsViewModel

        function onDetailChanged() {
            if (!payableOrderCombo.model) {
                payableOrderCombo.currentIndex = -1
                return
            }

            if (payableOrderCombo.model.length === 0) {
                payableOrderCombo.currentIndex = -1
            } else if (payableOrderCombo.currentIndex < 0 || payableOrderCombo.currentIndex >= payableOrderCombo.model.length) {
                payableOrderCombo.currentIndex = 0
            }
        }

        function onSelectedCustomerChanged() {
            amountField.text = ""
        }
    }

    Component.onCompleted: {
        paymentsViewModel.reload()
        appViewModel.refreshOverview()
    }

    ColumnLayout {
        id: contentColumn

        width: root.width
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Thanh toán & Công nợ"
            subtitle: "Lập phiếu thu, chặn over-payment, cập nhật Paid/PartiallyPaid và đối soát ledger theo khách hàng."
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(720, root.height - 60)
            spacing: 12

            Rectangle {
                Layout.preferredWidth: Math.max(300, Math.min(380, root.width * 0.32))
                Layout.minimumWidth: 280
                Layout.fillHeight: true
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Text {
                        text: "Khách có công nợ"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        SolidTextField {
                            id: searchField

                            Layout.fillWidth: true
                            placeholderText: "Tìm theo mã hoặc tên khách"
                            text: paymentsViewModel.filterText
                            onTextChanged: paymentsViewModel.filterText = text
                        }

                        ActionButton {
                            text: "Xóa"
                            fillColor: "#607D9C"
                            enabled: searchField.text.length > 0
                            onClicked: searchField.text = ""
                        }
                    }

                    Text {
                        text: "Kết quả: " + paymentsViewModel.customerCount
                        font.pixelSize: 13
                        color: "#4A5B72"
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: paymentsViewModel

                        delegate: Rectangle {
                            required property string customerId
                            required property string code
                            required property string name
                            required property string receivableText
                            required property int openOrderCount
                            required property int paymentCount

                            width: ListView.view.width
                            height: 88
                            radius: 10
                            color: customerId === paymentsViewModel.selectedCustomerId ? "#DBE9FF" : "#F9FBFE"
                            border.width: 1
                            border.color: customerId === paymentsViewModel.selectedCustomerId ? "#8FB2E8" : "#DCE6F2"

                            Column {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 3

                                Text {
                                    text: code + " - " + name
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                    color: "#1D3047"
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Text {
                                    text: "Công nợ còn lại: " + receivableText
                                    font.pixelSize: 12
                                    color: "#4D617B"
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Text {
                                    text: "Đơn chờ thu: " + openOrderCount + " | Phiếu thu: " + paymentCount
                                    font.pixelSize: 12
                                    color: "#4D617B"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: paymentsViewModel.selectCustomer(customerId)
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.minimumWidth: 520
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
                        text: paymentsViewModel.hasSelectedCustomer
                            ? paymentsViewModel.selectedCustomerName
                            : "Chọn khách hàng để lập phiếu thu"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Rectangle {
                        id: summaryCard
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        Layout.minimumHeight: implicitHeight
                        implicitHeight: summaryContent.implicitHeight + 20
                        radius: 10
                        color: "#F6FAFF"
                        border.color: "#D7E4F2"
                        clip: true

                        RowLayout {
                            id: summaryContent

                            x: 10
                            y: 10
                            width: Math.max(0, parent.width - 20)
                            spacing: 12

                            Text {
                                text: "Công nợ hiện tại: " + paymentsViewModel.selectedReceivableText
                                color: "#1F4F8E"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }

                            Text {
                                text: "Đơn chờ thu: " + paymentsViewModel.selectedOpenOrderCount
                                color: "#4A5B72"
                                font.pixelSize: 13
                            }
                        }
                    }

                    Rectangle {
                        id: receiptCard
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        Layout.minimumHeight: implicitHeight
                        implicitHeight: receiptContent.implicitHeight + 20
                        radius: 10
                        color: "#F9FBFE"
                        border.color: "#DCE6F2"
                        clip: true

                        ColumnLayout {
                            id: receiptContent

                            x: 10
                            y: 10
                            width: Math.max(0, parent.width - 20)
                            spacing: 8

                            Text {
                                text: "Lập phiếu thu"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: "#1B2B40"
                            }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 8

                                SolidComboBox {
                                    id: payableOrderCombo

                                    width: Math.max(260, Math.min(420, parent.width * 0.48))
                                    enabled: paymentsViewModel.hasSelectedCustomer && paymentsViewModel.payableOrders.length > 0
                                    model: paymentsViewModel.payableOrders
                                    textRole: "label"
                                }

                                SolidTextField {
                                    id: amountField

                                    width: Math.max(140, Math.min(180, parent.width * 0.18))
                                    enabled: paymentsViewModel.hasSelectedCustomer
                                    placeholderText: "Số tiền thu"
                                    inputMethodHints: Qt.ImhDigitsOnly
                                }

                                SolidTextField {
                                    id: methodField

                                    width: Math.max(140, Math.min(180, parent.width * 0.18))
                                    enabled: paymentsViewModel.hasSelectedCustomer
                                    placeholderText: "Phương thức"
                                    text: "Tiền mặt"
                                }

                                SolidTextField {
                                    id: paidAtField

                                    width: Math.max(140, Math.min(170, parent.width * 0.2))
                                    enabled: paymentsViewModel.hasSelectedCustomer
                                    placeholderText: "YYYY-MM-DD"
                                    text: Qt.formatDate(new Date(), "yyyy-MM-dd")
                                }

                                ActionButton {
                                    text: "Lập phiếu thu"
                                    fillColor: "#2E8B57"
                                    enabled: paymentsViewModel.hasSelectedCustomer && paymentsViewModel.payableOrders.length > 0
                                    onClicked: {
                                        if (paymentsViewModel.createReceipt(root.selectedId(payableOrderCombo),
                                                                            amountField.text,
                                                                            methodField.text,
                                                                            paidAtField.text)) {
                                            amountField.text = ""
                                            appViewModel.refreshOverview()
                                        }
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: paymentsViewModel.payableOrders.length > 0
                                    ? "Phiếu thu sẽ tự chặn over-payment và tự cập nhật trạng thái đơn sang PartiallyPaid/Paid."
                                    : "Khách này hiện không có đơn Confirmed/PartiallyPaid nào còn công nợ."
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
                                    text: "Lịch sử phiếu thu"
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: "#1B2B40"
                                }

                                ListView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    spacing: 6
                                    model: paymentsViewModel.paymentHistory

                                    delegate: Rectangle {
                                        property var paymentData: modelData

                                        width: ListView.view.width
                                        height: 72
                                        radius: 8
                                        color: "#FFFFFF"
                                        border.width: 1
                                        border.color: "#DCE6F2"

                                        Column {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 3

                                            Text {
                                                text: paymentData.paymentNo + " | Đơn: " + paymentData.orderNo
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                color: "#1D3047"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            Text {
                                                text: paymentData.paidAt + " | " + paymentData.method
                                                font.pixelSize: 12
                                                color: "#4D617B"
                                            }

                                            Text {
                                                text: "Số tiền: " + paymentData.amountText
                                                font.pixelSize: 12
                                                color: "#2E8B57"
                                            }
                                        }
                                    }
                                }

                                Text {
                                    visible: paymentsViewModel.paymentHistory.length === 0
                                    text: "Chưa có phiếu thu cho khách này."
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
                                    text: "Ledger đối soát"
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: "#1B2B40"
                                }

                                ListView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    spacing: 6
                                    model: paymentsViewModel.customerLedger

                                    delegate: Rectangle {
                                        property var entryData: modelData

                                        width: ListView.view.width
                                        height: 72
                                        radius: 8
                                        color: "#FFFFFF"
                                        border.width: 1
                                        border.color: "#DCE6F2"

                                        Column {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 3

                                            Text {
                                                text: entryData.createdAt + " | " + entryData.refType + ": " + entryData.refId
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                color: "#1D3047"
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            Text {
                                                text: (entryData.isDebit ? "+ " : "- ") + entryData.deltaText.replace("-", "")
                                                font.pixelSize: 12
                                                color: entryData.isDebit ? "#A04E19" : "#2E8B57"
                                            }

                                            Text {
                                                text: "Dư nợ sau bút toán: " + entryData.balanceAfterText
                                                font.pixelSize: 12
                                                color: "#4D617B"
                                            }
                                        }
                                    }
                                }

                                Text {
                                    visible: paymentsViewModel.customerLedger.length === 0
                                    text: "Chưa có bút toán công nợ cho khách này."
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
                        visible: paymentsViewModel.lastError.length > 0
                        color: "#FFECEC"
                        border.color: "#F0BABA"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            verticalAlignment: Text.AlignVCenter
                            text: paymentsViewModel.lastError
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
