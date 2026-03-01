import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Item {
    id: root

    property string editingProductId: ""
    property string currentProductSku: "Tự sinh khi lưu"
    property string selectedLotId: ""
    property string selectedLotNo: ""

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

    function resetLotFields() {
        const today = new Date()
        const expiry = new Date(today.getTime())
        expiry.setMonth(expiry.getMonth() + 6)

        lotNoField.text = ""
        lotDateField.text = Qt.formatDate(today, "yyyy-MM-dd")
        lotExpiryField.text = Qt.formatDate(expiry, "yyyy-MM-dd")
        lotQtyField.text = ""
        lotCostField.text = ""
    }

    function clearProductForm() {
        editingProductId = ""
        currentProductSku = "Tự sinh khi lưu"
        selectedLotId = ""
        selectedLotNo = ""

        nameField.text = ""
        unitField.text = "thung"
        defaultPriceField.text = ""
        isActiveCheck.checked = true

        productList.currentIndex = -1
        productsViewModel.selectProduct("")
        resetLotFields()
        movementQtyField.text = ""
    }

    function loadProductFromRow(row) {
        const product = productsViewModel.getProduct(row)
        if (!product.productId) {
            return
        }

        editingProductId = product.productId
        currentProductSku = product.sku
        selectedLotId = ""
        selectedLotNo = ""

        nameField.text = product.name
        unitField.text = product.unit
        defaultPriceField.text = product.defaultWholesalePriceText
        isActiveCheck.checked = product.isActive

        productsViewModel.selectProduct(product.productId)
        movementQtyField.text = ""
        resetLotFields()
    }

    Component.onCompleted: {
        productsViewModel.reload()
        appViewModel.refreshOverview()
        clearProductForm()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Sản phẩm, lô và xuất nhập"
            subtitle: "CRUD sản phẩm + thêm lô có hạn dùng + thao tác nhập/xuất tồn kho theo từng lô."
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.preferredWidth: Math.max(320, Math.min(460, root.width * 0.42))
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

                        SolidTextField {
                            id: searchField

                            Layout.fillWidth: true
                            placeholderText: "Tìm theo SKU, tên hoặc đơn vị"
                            onTextChanged: productsViewModel.filterText = text
                        }

                        ActionButton {
                            text: "Xóa lọc"
                            fillColor: "#607D9C"
                            enabled: searchField.text.length > 0
                            onClicked: searchField.clear()
                        }
                    }

                    Text {
                        text: "Kết quả: " + productsViewModel.productCount
                        font.pixelSize: 13
                        color: "#4A5B72"
                    }

                    ListView {
                        id: productList

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: productsViewModel

                        delegate: Rectangle {
                            required property int index
                            required property string productId
                            required property string sku
                            required property string name
                            required property string unit
                            required property string defaultWholesalePriceText
                            required property int totalOnHand
                            required property int lotCount

                            width: ListView.view.width
                            height: 88
                            radius: 10
                            color: ListView.isCurrentItem ? "#DBE9FF" : "#F9FBFE"
                            border.width: 1
                            border.color: ListView.isCurrentItem ? "#8FB2E8" : "#DCE6F2"

                            Column {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 3

                                Text {
                                    text: sku + " - " + name
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                    color: "#1D3047"
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Text {
                                    text: "Đơn vị: " + unit + " | Giá sỉ: " + defaultWholesalePriceText
                                    font.pixelSize: 12
                                    color: "#4D617B"
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Text {
                                    text: "Tồn: " + totalOnHand + " | Số lô: " + lotCount
                                    font.pixelSize: 12
                                    color: "#4D617B"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    productList.currentIndex = index
                                    root.loadProductFromRow(index)
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
                    spacing: 12

                    Text {
                        text: editingProductId.length > 0 ? "Sửa sản phẩm" : "Thêm sản phẩm"
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
                            text: "Mã sản phẩm"
                            color: "#30465F"
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.currentProductSku
                            color: "#1D3047"
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                        }

                        Label {
                            text: "Tên sản phẩm *"
                            color: "#30465F"
                        }

                        SolidTextField {
                            id: nameField

                            Layout.fillWidth: true
                            placeholderText: "Nhập tên sản phẩm"
                        }

                        Label {
                            text: "Đơn vị"
                            color: "#30465F"
                        }

                        SolidTextField {
                            id: unitField

                            Layout.fillWidth: true
                            placeholderText: "VD: thung"
                        }

                        Label {
                            text: "Giá sỉ mặc định (VND)"
                            color: "#30465F"
                        }

                        SolidTextField {
                            id: defaultPriceField

                            Layout.fillWidth: true
                            placeholderText: "VD: 500000"
                            inputMethodHints: Qt.ImhDigitsOnly
                        }

                        Label {
                            text: "Trạng thái"
                            color: "#30465F"
                        }

                        CheckBox {
                            id: isActiveCheck

                            text: "Đang kinh doanh"
                            checked: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ActionButton {
                            text: "Tạo mới"
                            fillColor: "#607D9C"
                            onClicked: root.clearProductForm()
                        }

                        ActionButton {
                            text: editingProductId.length > 0 ? "Lưu thay đổi" : "Thêm sản phẩm"
                            fillColor: "#2D6CDF"
                            onClicked: {
                                let success = false
                                if (editingProductId.length > 0) {
                                    success = productsViewModel.updateProduct(
                                                editingProductId,
                                                root.currentProductSku,
                                                nameField.text,
                                                unitField.text,
                                                defaultPriceField.text,
                                                isActiveCheck.checked)
                                } else {
                                    success = productsViewModel.createProduct(
                                                "",
                                                nameField.text,
                                                unitField.text,
                                                defaultPriceField.text,
                                                isActiveCheck.checked)
                                }

                                if (success) {
                                    appViewModel.refreshOverview()
                                    inventoryViewModel.reload()
                                    if (editingProductId.length === 0) {
                                        root.clearProductForm()
                                    } else {
                                        productsViewModel.selectProduct(editingProductId)
                                    }
                                }
                            }
                        }

                        ActionButton {
                            text: "Xóa"
                            fillColor: "#C05A2A"
                            enabled: editingProductId.length > 0
                            onClicked: {
                                if (productsViewModel.deleteProduct(editingProductId)) {
                                    appViewModel.refreshOverview()
                                    inventoryViewModel.reload()
                                    root.clearProductForm()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#E4ECF5"
                    }

                    Text {
                        text: productsViewModel.hasSelectedProduct
                            ? "Lô của " + productsViewModel.selectedProductName + " | Tồn tổng: "
                              + productsViewModel.selectedProductTotalOnHand
                            : "Chọn một sản phẩm để quản lý lô và xuất/nhập"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        Layout.minimumHeight: 42
                        visible: productsViewModel.hasSelectedProduct
                                 && productsViewModel.selectedProductExpiringSoonLotCount > 0
                        radius: 8
                        color: "#FFF4DA"
                        border.color: "#E3C676"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            verticalAlignment: Text.AlignVCenter
                            color: "#7A5713"
                            font.pixelSize: 13
                            text: "Cảnh báo: " + productsViewModel.selectedProductExpiringSoonLotCount
                                  + " lô sẽ hết hạn trong vòng 30 ngày."
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        enabled: productsViewModel.hasSelectedProduct
                        spacing: 8

                        SolidTextField {
                            id: lotNoField

                            width: Math.max(140, Math.min(220, parent.width * 0.3))
                            placeholderText: "Mã lô (tùy chọn)"
                        }

                        SolidTextField {
                            id: lotDateField

                            width: Math.max(120, Math.min(170, parent.width * 0.22))
                            placeholderText: "Ngày nhập (YYYY-MM-DD)"
                        }

                        SolidTextField {
                            id: lotExpiryField

                            width: Math.max(120, Math.min(170, parent.width * 0.22))
                            placeholderText: "Hạn dùng (YYYY-MM-DD)"
                        }

                        SolidTextField {
                            id: lotQtyField

                            width: Math.max(86, Math.min(110, parent.width * 0.15))
                            placeholderText: "SL nhập"
                            inputMethodHints: Qt.ImhDigitsOnly
                        }

                        SolidTextField {
                            id: lotCostField

                            width: Math.max(100, Math.min(140, parent.width * 0.2))
                            placeholderText: "Giá vốn"
                            inputMethodHints: Qt.ImhDigitsOnly
                        }

                        ActionButton {
                            text: "Thêm lô"
                            fillColor: "#2D6CDF"
                            onClicked: {
                                if (productsViewModel.addLotToSelectedProduct(
                                            lotNoField.text,
                                            lotDateField.text,
                                            lotExpiryField.text,
                                            lotQtyField.text,
                                            lotCostField.text)) {
                                    appViewModel.refreshOverview()
                                    inventoryViewModel.reload()
                                    root.selectedLotId = ""
                                    root.selectedLotNo = ""
                                    root.resetLotFields()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 170
                        radius: 10
                        color: "#F9FBFE"
                        border.color: "#DCE6F2"

                        ListView {
                            id: lotList

                            anchors.fill: parent
                            anchors.margins: 8
                            clip: true
                            spacing: 6
                            model: productsViewModel.selectedProductLots

                            delegate: Rectangle {
                                property var lotData: modelData

                                width: ListView.view.width
                                height: 72
                                radius: 8
                                color: root.selectedLotId === lotData.lotId
                                    ? "#E6F1FF"
                                    : lotData.isExpired
                                        ? "#FFF1F1"
                                        : lotData.isExpiringSoon
                                            ? "#FFF8E8"
                                            : "#FFFFFF"
                                border.width: 1
                                border.color: root.selectedLotId === lotData.lotId
                                    ? "#8FB2E8"
                                    : lotData.isExpired
                                        ? "#E7B9B9"
                                        : lotData.isExpiringSoon
                                            ? "#E6C781"
                                            : "#DCE6F2"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Text {
                                            text: lotData.lotNo + " | Ngày nhập: " + lotData.receivedDate
                                                  + " | HSD: " + lotData.expiryDate
                                            font.pixelSize: 13
                                            font.weight: Font.DemiBold
                                            color: "#1D3047"
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Text {
                                            text: "Tồn lô: " + lotData.onHandQty + " | Giá vốn: " + lotData.unitCostText
                                            font.pixelSize: 12
                                            color: "#4D617B"
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Rectangle {
                                        visible: lotData.isExpired || lotData.isExpiringSoon
                                        radius: 6
                                        color: lotData.isExpired ? "#FDE0E0" : "#FFF0CB"
                                        border.color: lotData.isExpired ? "#D78F8F" : "#D8B266"
                                        Layout.preferredHeight: 26
                                        Layout.preferredWidth: warningText.implicitWidth + 14

                                        Text {
                                            id: warningText

                                            anchors.centerIn: parent
                                            text: lotData.isExpired
                                                ? "Quá hạn"
                                                : "Còn " + lotData.daysToExpiry + " ngày"
                                            color: lotData.isExpired ? "#8F2D2D" : "#7A5713"
                                            font.pixelSize: 11
                                            font.weight: Font.DemiBold
                                        }
                                    }

                                    ActionButton {
                                        text: "Chọn"
                                        fillColor: "#607D9C"
                                        onClicked: {
                                            root.selectedLotId = lotData.lotId
                                            root.selectedLotNo = lotData.lotNo
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.selectedLotId = lotData.lotId
                                        root.selectedLotNo = lotData.lotNo
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        enabled: productsViewModel.hasSelectedProduct
                        spacing: 8

                        Text {
                            text: selectedLotNo.length > 0
                                ? "Lô đang chọn: " + selectedLotNo
                                : "Chưa chọn lô"
                            color: "#30465F"
                            font.pixelSize: 13
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        SolidTextField {
                            id: movementQtyField

                            Layout.preferredWidth: 110
                            Layout.minimumWidth: 80
                            placeholderText: "Số lượng"
                            inputMethodHints: Qt.ImhDigitsOnly
                        }

                        ActionButton {
                            text: "Nhập +"
                            fillColor: "#2E8B57"
                            enabled: selectedLotId.length > 0
                            onClicked: {
                                if (productsViewModel.stockInLot(selectedLotId, movementQtyField.text)) {
                                    appViewModel.refreshOverview()
                                    inventoryViewModel.reload()
                                    movementQtyField.clear()
                                }
                            }
                        }

                        ActionButton {
                            text: "Xuất -"
                            fillColor: "#C05A2A"
                            enabled: selectedLotId.length > 0
                            onClicked: {
                                if (productsViewModel.stockOutLot(selectedLotId, movementQtyField.text)) {
                                    appViewModel.refreshOverview()
                                    inventoryViewModel.reload()
                                    movementQtyField.clear()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 8
                        visible: productsViewModel.lastError.length > 0
                        color: "#FFECEC"
                        border.color: "#F0BABA"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            verticalAlignment: Text.AlignVCenter
                            text: productsViewModel.lastError
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
