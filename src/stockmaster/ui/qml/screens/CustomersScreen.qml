import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Item {
    id: root

    property string editingCustomerId: ""
    property string currentCustomerCode: "Tự sinh khi lưu"

    function clearForm() {
        editingCustomerId = ""
        currentCustomerCode = "Tự sinh khi lưu"
        nameField.text = ""
        phoneField.text = ""
        addressField.text = ""
        creditLimitField.text = ""
        customerList.currentIndex = -1
    }

    function loadFormFromRow(row) {
        const customer = customersViewModel.getCustomer(row)
        if (!customer.customerId) {
            return
        }

        editingCustomerId = customer.customerId
        currentCustomerCode = customer.code
        nameField.text = customer.name
        phoneField.text = customer.phone
        addressField.text = customer.address
        creditLimitField.text = customer.creditLimitText
    }

    Component.onCompleted: {
        customersViewModel.reload()
        appViewModel.refreshOverview()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Khách hàng"
            subtitle: "Tạo, sửa, xóa khách hàng trong bộ dữ liệu hiện tại. Hạn mức công nợ nhập dạng số nguyên VND."
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 460
                Layout.fillHeight: true
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        TextField {
                            id: searchField

                            Layout.fillWidth: true
                            placeholderText: "Tìm theo mã, tên hoặc số điện thoại"
                            onTextChanged: customersViewModel.filterText = text
                        }

                        Button {
                            text: "Xóa lọc"
                            enabled: searchField.text.length > 0
                            onClicked: searchField.clear()
                        }
                    }

                    Text {
                        text: "Kết quả: " + customersViewModel.customerCount
                        font.pixelSize: 13
                        color: "#4A5B72"
                    }

                    ListView {
                        id: customerList

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: customersViewModel

                        delegate: Rectangle {
                            required property int index
                            required property string customerId
                            required property string code
                            required property string name
                            required property string phone
                            required property string creditLimitText

                            width: ListView.view.width
                            height: 72
                            radius: 10
                            color: ListView.isCurrentItem ? "#DBE9FF" : "#F9FBFE"
                            border.width: 1
                            border.color: ListView.isCurrentItem ? "#8FB2E8" : "#DCE6F2"

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
                                    text: phone.length > 0 ? phone : "Chưa có số điện thoại"
                                    font.pixelSize: 12
                                    color: "#4D617B"
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Text {
                                    text: "Hạn mức: " + creditLimitText
                                    font.pixelSize: 12
                                    color: "#4D617B"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    customerList.currentIndex = index
                                    root.loadFormFromRow(index)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Text {
                        text: editingCustomerId.length > 0 ? "Sửa khách hàng" : "Thêm khách hàng"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 8
                        columnSpacing: 10

                        Label {
                            text: "Mã khách"
                            color: "#30465F"
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.currentCustomerCode
                            color: "#1D3047"
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                        }

                        Label {
                            text: "Tên khách *"
                            color: "#30465F"
                        }

                        TextField {
                            id: nameField

                            Layout.fillWidth: true
                            placeholderText: "Nhập tên khách hàng"
                        }

                        Label {
                            text: "Điện thoại"
                            color: "#30465F"
                        }

                        TextField {
                            id: phoneField

                            Layout.fillWidth: true
                            placeholderText: "Nhập số điện thoại"
                        }

                        Label {
                            text: "Địa chỉ"
                            color: "#30465F"
                        }

                        TextField {
                            id: addressField

                            Layout.fillWidth: true
                            placeholderText: "Nhập địa chỉ"
                        }

                        Label {
                            text: "Hạn mức công nợ (VND)"
                            color: "#30465F"
                        }

                        TextField {
                            id: creditLimitField

                            Layout.fillWidth: true
                            placeholderText: "VD: 50000000"
                            inputMethodHints: Qt.ImhDigitsOnly
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 8
                        visible: customersViewModel.lastError.length > 0
                        color: "#FFECEC"
                        border.color: "#F0BABA"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            verticalAlignment: Text.AlignVCenter
                            text: customersViewModel.lastError
                            color: "#8F2D2D"
                            font.pixelSize: 13
                            elide: Text.ElideRight
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Button {
                            text: "Tạo mới"
                            onClicked: root.clearForm()
                        }

                        Button {
                            text: editingCustomerId.length > 0 ? "Lưu thay đổi" : "Thêm khách hàng"
                            highlighted: true
                            onClicked: {
                                let success = false
                                if (editingCustomerId.length > 0) {
                                    success = customersViewModel.updateCustomer(
                                                editingCustomerId,
                                                nameField.text,
                                                phoneField.text,
                                                addressField.text,
                                                creditLimitField.text)
                                } else {
                                    success = customersViewModel.createCustomer(
                                                nameField.text,
                                                phoneField.text,
                                                addressField.text,
                                                creditLimitField.text)
                                }

                                if (success) {
                                    appViewModel.refreshOverview()
                                    if (editingCustomerId.length === 0) {
                                        root.clearForm()
                                    }
                                }
                            }
                        }

                        Button {
                            text: "Xóa"
                            enabled: editingCustomerId.length > 0
                            onClicked: {
                                if (customersViewModel.deleteCustomer(editingCustomerId)) {
                                    appViewModel.refreshOverview()
                                    root.clearForm()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
