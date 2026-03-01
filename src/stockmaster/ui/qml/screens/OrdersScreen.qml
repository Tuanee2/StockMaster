import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Flickable {
    id: root

    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight + 16

    property string selectedItemIdForEdit: ""
    property int orderTabIndex: 0
    property bool itemFormLoading: false
    property var newOrderCustomerOptions: []
    property var orderCustomerOptions: []
    property var productLotOptions: []
    property var allocationPreviewRows: []

    function customerMatches(customer, keyword) {
        const normalized = keyword.trim().toLowerCase()
        if (!normalized) {
            return true
        }

        const code = (customer.code || "").toLowerCase()
        const name = (customer.name || "").toLowerCase()
        const phone = (customer.phone || "").toLowerCase()
        const label = (customer.label || "").toLowerCase()
        return code.indexOf(normalized) >= 0
            || name.indexOf(normalized) >= 0
            || phone.indexOf(normalized) >= 0
            || label.indexOf(normalized) >= 0
    }

    function rebuildCustomerOptions() {
        const allCustomers = ordersViewModel.customers || []
        const currentNewOrderId = root.selectedId(newOrderCustomerCombo)
        const selectedCustomerId = ordersViewModel.selectedOrderCustomerId

        const nextNewOrderOptions = []
        const nextSelectedOrderOptions = []
        for (let i = 0; i < allCustomers.length; ++i) {
            const customer = allCustomers[i]
            if (root.customerMatches(customer, newOrderCustomerSearchField.text)) {
                nextNewOrderOptions.push(customer)
            }
            if (root.customerMatches(customer, orderCustomerSearchField.text)) {
                nextSelectedOrderOptions.push(customer)
            }
        }

        root.newOrderCustomerOptions = nextNewOrderOptions
        root.orderCustomerOptions = nextSelectedOrderOptions

        if (root.newOrderCustomerOptions.length === 0) {
            newOrderCustomerCombo.currentIndex = -1
        } else {
            root.setComboIndexById(newOrderCustomerCombo, currentNewOrderId)
            if (newOrderCustomerCombo.currentIndex < 0) {
                newOrderCustomerCombo.currentIndex = 0
            }
        }

        if (root.orderCustomerOptions.length === 0) {
            orderCustomerCombo.currentIndex = -1
        } else {
            root.setComboIndexById(orderCustomerCombo, selectedCustomerId)
        }
    }

    function refreshScreenData() {
        ordersViewModel.reload()
        root.rebuildCustomerOptions()
        root.rebuildProductLotOptions()
        root.syncUnitPriceFromSelectedProduct(false)
        root.refreshAllocationPreview()
        appViewModel.refreshOverview()
        if (ordersViewModel.orderCount > 0 && !ordersViewModel.hasSelectedOrder) {
            ordersViewModel.selectOrder(ordersViewModel.orderIdAt(0))
        }
    }

    function setComboIndexById(combo, id) {
        if (!combo || !combo.model) {
            return
        }

        for (let i = 0; i < combo.model.length; ++i) {
            if (combo.model[i].id === id) {
                combo.currentIndex = i
                return
            }
        }

        combo.currentIndex = -1
    }

    function selectedId(combo) {
        if (!combo || !combo.model || combo.currentIndex < 0 || combo.currentIndex >= combo.model.length) {
            return ""
        }

        return combo.model[combo.currentIndex].id
    }

    function loadItemToForm(itemData) {
        if (!itemData) {
            return
        }

        itemFormLoading = true
        selectedItemIdForEdit = itemData.orderItemId
        setComboIndexById(productCombo, itemData.productId)
        root.rebuildProductLotOptions()
        root.setComboIndexById(productLotCombo, itemData.primaryLotId || "")
        qtyField.text = itemData.qty.toString()
        unitPriceField.text = itemData.unitPriceVnd.toString()
        discountField.text = itemData.discountVnd.toString()
        root.refreshAllocationPreview()
        itemFormLoading = false
    }

    function clearItemForm() {
        selectedItemIdForEdit = ""
        itemFormLoading = true
        qtyField.text = ""
        discountField.text = ""
        root.rebuildProductLotOptions()
        root.syncUnitPriceFromSelectedProduct(false)
        root.refreshAllocationPreview()
        itemFormLoading = false
    }

    function rebuildProductLotOptions() {
        const currentLotId = root.selectedId(productLotCombo)
        root.productLotOptions = ordersViewModel.lotsForProduct(root.selectedId(productCombo))

        if (root.productLotOptions.length === 0) {
            productLotCombo.currentIndex = -1
        } else {
            root.setComboIndexById(productLotCombo, currentLotId)
            if (productLotCombo.currentIndex < 0) {
                productLotCombo.currentIndex = 0
            }
        }
    }

    function syncUnitPriceFromSelectedProduct(force) {
        const productId = root.selectedId(productCombo)
        if (!productId) {
            if (force) {
                unitPriceField.text = ""
            }
            return
        }

        if (!force && unitPriceField.text.length > 0) {
            return
        }

        unitPriceField.text = ordersViewModel.defaultPriceTextForProduct(productId)
    }

    function refreshAllocationPreview() {
        root.allocationPreviewRows = ordersViewModel.previewAllocations(root.selectedId(productCombo),
                                                                        root.selectedId(productLotCombo),
                                                                        qtyField.text)
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

    component NavTabButton: Button {
        id: navButton

        property int tabIndex: 0

        implicitHeight: 32
        checkable: false

        background: Rectangle {
            radius: 7
            color: root.orderTabIndex === navButton.tabIndex ? "#D5E7FF" : "transparent"
            border.width: 1
            border.color: root.orderTabIndex === navButton.tabIndex ? "#86ACE2" : "#D6E4F3"
        }

        contentItem: Text {
            text: navButton.text
            color: root.orderTabIndex === navButton.tabIndex ? "#1F4F8E" : "#435B79"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        onClicked: root.orderTabIndex = tabIndex
    }

    Connections {
        target: ordersViewModel

        function onSelectedOrderChanged() {
            root.setComboIndexById(orderCustomerCombo, ordersViewModel.selectedOrderCustomerId)
            root.clearItemForm()
        }

        function onLookupChanged() {
            root.rebuildCustomerOptions()
            root.rebuildProductLotOptions()
            root.syncUnitPriceFromSelectedProduct(false)
        }
    }

    Component.onCompleted: {
        root.refreshScreenData()
    }

    onVisibleChanged: {
        if (visible) {
            root.refreshScreenData()
        }
    }

    ColumnLayout {
        id: contentColumn

        width: root.width
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Đơn hàng"
            subtitle: "Dùng thanh tab nhỏ để điều hướng: Tạo Draft, Item & Xử lý, rồi đến Chi tiết đơn."
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 700
            spacing: 12

            Rectangle {
                Layout.preferredWidth: Math.max(320, Math.min(440, root.width * 0.36))
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
                        text: "Danh sách đơn"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Text {
                        text: "Tổng đơn: " + ordersViewModel.orderCount
                        font.pixelSize: 13
                        color: "#4A5B72"
                    }

                    Rectangle {
                        id: orderFilterBlock
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        Layout.minimumHeight: implicitHeight
                        implicitHeight: orderFilterContent.implicitHeight + 16
                        radius: 10
                        color: "#F7FBFF"
                        border.color: "#D7E4F2"
                        clip: true

                        ColumnLayout {
                            id: orderFilterContent
                            x: 8
                            y: 8
                            width: Math.max(0, parent.width - 16)
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                radius: 9
                                color: "#F4F8FD"
                                border.color: "#D9E6F4"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    spacing: 6

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: 7
                                        color: "#D5E7FF"
                                        border.width: 1
                                        border.color: "#86ACE2"

                                        Text {
                                            anchors.centerIn: parent
                                            text: "Tùy chọn lọc danh sách đơn"
                                            color: "#1F4F8E"
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SolidTextField {
                                    id: orderNoQueryField

                                    Layout.fillWidth: true
                                    placeholderText: "Theo mã đơn (VD: ORD00012)"
                                    text: ordersViewModel.queryOrderNo
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SolidTextField {
                                    id: fromDateQueryField

                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    placeholderText: "Từ ngày"
                                    text: ordersViewModel.queryFromDate
                                }

                                SolidTextField {
                                    id: toDateQueryField

                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    placeholderText: "Đến ngày"
                                    text: ordersViewModel.queryToDate
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ActionButton {
                                    Layout.fillWidth: true
                                    text: "Lọc"
                                    fillColor: "#2D6CDF"
                                    onClicked: ordersViewModel.applyOrderQuery(orderNoQueryField.text,
                                                                               fromDateQueryField.text,
                                                                               toDateQueryField.text)
                                }

                                ActionButton {
                                    Layout.fillWidth: true
                                    text: "Xóa lọc"
                                    fillColor: "#607D9C"
                                    onClicked: {
                                        orderNoQueryField.text = ""
                                        fromDateQueryField.text = ""
                                        toDateQueryField.text = ""
                                        ordersViewModel.clearOrderQuery()
                                    }
                                }
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
                                text: "Danh sách đơn theo bộ lọc"
                                font.pixelSize: 12
                                color: "#4D617B"
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 6
                                model: ordersViewModel

                                delegate: Rectangle {
                                    required property string orderId
                                    required property string orderNo
                                    required property string orderDate
                                    required property string status
                                    required property string customerName
                                    required property string totalText
                                    required property int itemCount

                                    width: ListView.view.width
                                    height: 86
                                    radius: 10
                                    color: orderId === ordersViewModel.selectedOrderId ? "#DBE9FF" : "#FFFFFF"
                                    border.width: 1
                                    border.color: orderId === ordersViewModel.selectedOrderId ? "#8FB2E8" : "#DCE6F2"

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 2

                                        Text {
                                            text: orderNo + "  [" + status + "]"
                                            font.pixelSize: 14
                                            font.weight: Font.DemiBold
                                            color: "#1D3047"
                                        }

                                        Text {
                                            text: customerName + " | " + orderDate
                                            font.pixelSize: 12
                                            color: "#4D617B"
                                            elide: Text.ElideRight
                                            width: parent.width
                                        }

                                        Text {
                                            text: "Dòng hàng: " + itemCount + " | Tổng: " + totalText
                                            font.pixelSize: 12
                                            color: "#4D617B"
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            ordersViewModel.selectOrder(orderId)
                                            root.orderTabIndex = 1
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
                Layout.minimumWidth: 460
                Layout.fillHeight: true
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Text {
                        text: ordersViewModel.hasSelectedOrder
                            ? ordersViewModel.selectedOrderNo + " - " + ordersViewModel.selectedOrderStatus
                            : "Điều hướng nghiệp vụ Order"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: 10
                        color: "#F4F8FD"
                        border.color: "#D9E6F4"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 6

                            NavTabButton {
                                Layout.fillWidth: true
                                tabIndex: 0
                                text: "Tạo Draft"
                            }

                            NavTabButton {
                                Layout.fillWidth: true
                                tabIndex: 2
                                text: "Item & Xử lý"
                            }

                            NavTabButton {
                                Layout.fillWidth: true
                                tabIndex: 1
                                text: "Chi tiết đơn"
                            }
                        }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: root.orderTabIndex

                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 10

                                Text {
                                    text: "Tạo đơn Draft mới"
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                    color: "#1B2B40"
                                }

                                SolidTextField {
                                    id: newOrderCustomerSearchField

                                    Layout.fillWidth: true
                                    placeholderText: "Tìm khách theo tên, mã hoặc số điện thoại"
                                    onTextChanged: root.rebuildCustomerOptions()
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    SolidComboBox {
                                        id: newOrderCustomerCombo

                                        width: Math.max(220, Math.min(400, parent.width * 0.45))
                                        model: root.newOrderCustomerOptions
                                        textRole: "label"
                                    }

                                    SolidTextField {
                                        id: orderDateField

                                        width: 140
                                        placeholderText: "YYYY-MM-DD"
                                        text: Qt.formatDate(new Date(), "yyyy-MM-dd")
                                    }

                                    ActionButton {
                                        text: "Tạo Draft"
                                        fillColor: "#2D6CDF"
                                        onClicked: {
                                            if (ordersViewModel.createDraftOrder(root.selectedId(newOrderCustomerCombo), orderDateField.text)) {
                                                appViewModel.refreshOverview()
                                                root.orderTabIndex = 2
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 10
                                    color: "#F9FBFE"
                                    border.color: "#DCE6F2"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Tạo draft rồi chuyển qua tab 'Item & Xử lý' để thêm hàng, hoặc 'Chi tiết đơn' để xem nhanh."
                                        color: "#4D617B"
                                        font.pixelSize: 13
                                    }
                                }
                            }
                        }

                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 10

                                Text {
                                    text: ordersViewModel.hasSelectedOrder
                                        ? "Ngày đơn: " + ordersViewModel.selectedOrderDate + " | Tổng: " + ordersViewModel.selectedOrderTotalText
                                        : "Chọn một đơn từ danh sách bên trái."
                                    font.pixelSize: 13
                                    color: "#4A5B72"
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    Text {
                                        text: "Khách hàng"
                                        color: "#30465F"
                                        Layout.preferredWidth: 86
                                    }

                                    SolidTextField {
                                        id: orderCustomerSearchField

                                        Layout.preferredWidth: 220
                                        enabled: ordersViewModel.canEditDraft
                                        placeholderText: "Tìm tên hoặc số điện thoại"
                                        onTextChanged: root.rebuildCustomerOptions()
                                    }

                                    SolidComboBox {
                                        id: orderCustomerCombo

                                        Layout.fillWidth: true
                                        enabled: ordersViewModel.canEditDraft
                                        model: root.orderCustomerOptions
                                        textRole: "label"
                                        onActivated: {
                                            if (ordersViewModel.updateSelectedOrderCustomer(root.selectedId(orderCustomerCombo))) {
                                                appViewModel.refreshOverview()
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 10
                                    color: "#F9FBFE"
                                    border.color: "#DCE6F2"

                                    ListView {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        clip: true
                                        spacing: 6
                                        model: ordersViewModel.selectedOrderItems

                                        delegate: Rectangle {
                                            property var itemData: modelData

                                            width: ListView.view.width
                                            height: itemData.lotSummary && itemData.lotSummary.length > 0 ? 82 : 64
                                            radius: 8
                                            color: "#FFFFFF"
                                            border.width: 1
                                            border.color: "#DCE6F2"

                                            Column {
                                                anchors.fill: parent
                                                anchors.margins: 10
                                                spacing: 2

                                                Text {
                                                    text: itemData.productName + " | SL: " + itemData.qty
                                                    font.pixelSize: 13
                                                    font.weight: Font.DemiBold
                                                    color: "#1D3047"
                                                }

                                                Text {
                                                    text: "Đơn giá: " + itemData.unitPriceText
                                                          + " | Giảm: " + itemData.discountText
                                                          + " | Thành tiền: " + itemData.lineTotalText
                                                    font.pixelSize: 12
                                                    color: "#4D617B"
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }

                                                Text {
                                                    visible: itemData.lotSummary && itemData.lotSummary.length > 0
                                                    text: "Phân lô: " + itemData.lotSummary
                                                    font.pixelSize: 11
                                                    color: "#58708F"
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 10

                                Text {
                                    text: "Quản lý item (chỉ thao tác khi trạng thái Draft)"
                                    font.pixelSize: 13
                                    color: "#4A5B72"
                                    Layout.fillWidth: true
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    SolidComboBox {
                                        id: productCombo

                                        width: Math.max(180, Math.min(300, parent.width * 0.38))
                                        enabled: ordersViewModel.canEditDraft
                                        model: ordersViewModel.products
                                        textRole: "label"
                                        onCurrentIndexChanged: {
                                            if (!root.itemFormLoading) {
                                                root.rebuildProductLotOptions()
                                                root.syncUnitPriceFromSelectedProduct(true)
                                                root.refreshAllocationPreview()
                                            }
                                        }
                                    }

                                    SolidComboBox {
                                        id: productLotCombo

                                        width: Math.max(220, Math.min(320, parent.width * 0.38))
                                        enabled: ordersViewModel.canEditDraft && root.productLotOptions.length > 0
                                        model: root.productLotOptions
                                        textRole: "label"
                                        onCurrentIndexChanged: {
                                            if (!root.itemFormLoading) {
                                                root.refreshAllocationPreview()
                                            }
                                        }
                                    }

                                    SolidTextField {
                                        id: qtyField

                                        width: 88
                                        enabled: ordersViewModel.canEditDraft
                                        placeholderText: "SL"
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        onTextChanged: root.refreshAllocationPreview()
                                    }

                                    SolidTextField {
                                        id: unitPriceField

                                        width: 128
                                        enabled: ordersViewModel.canEditDraft
                                        placeholderText: "Đơn giá"
                                        inputMethodHints: Qt.ImhDigitsOnly
                                    }

                                    SolidTextField {
                                        id: discountField

                                        width: 110
                                        enabled: ordersViewModel.canEditDraft
                                        placeholderText: "Giảm giá"
                                        inputMethodHints: Qt.ImhDigitsOnly
                                    }

                                    ActionButton {
                                        text: selectedItemIdForEdit.length > 0 ? "Cập nhật item" : "Thêm item"
                                        enabled: ordersViewModel.canEditDraft
                                                 && root.selectedId(productCombo).length > 0
                                                 && root.selectedId(productLotCombo).length > 0
                                        fillColor: "#2D6CDF"
                                        onClicked: {
                                            if (ordersViewModel.addOrUpdateDraftItem(
                                                        root.selectedId(productCombo),
                                                        root.selectedId(productLotCombo),
                                                        qtyField.text,
                                                        unitPriceField.text,
                                                        discountField.text)) {
                                                root.clearItemForm()
                                                appViewModel.refreshOverview()
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: previewContent.implicitHeight + 16
                                    visible: ordersViewModel.canEditDraft && root.selectedId(productCombo).length > 0
                                    radius: 10
                                    color: "#F7FBFF"
                                    border.color: "#D7E4F2"

                                    Column {
                                        id: previewContent

                                        x: 8
                                        y: 8
                                        width: parent.width - 16
                                        spacing: 4

                                        Text {
                                            width: parent.width
                                            text: root.productLotOptions.length > 0 && productLotCombo.currentIndex >= 0
                                                ? "Lô đang chọn tối đa: " + root.productLotOptions[productLotCombo.currentIndex].onHandQty
                                                  + " | " + root.productLotOptions[productLotCombo.currentIndex].lotNo
                                                : "Sản phẩm này chưa có lô còn tồn để phân bổ."
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                            color: "#365478"
                                            wrapMode: Text.WordWrap
                                        }

                                        Text {
                                            width: parent.width
                                            visible: qtyField.text.length === 0
                                            text: "Nhập số lượng để xem cách hệ thống phân bổ từ lô đã chọn sang các lô còn lại."
                                            font.pixelSize: 11
                                            color: "#6B819A"
                                            wrapMode: Text.WordWrap
                                        }

                                        Repeater {
                                            model: root.allocationPreviewRows

                                            delegate: Text {
                                                required property var modelData

                                                width: previewContent.width
                                                text: "• " + modelData.label
                                                font.pixelSize: 11
                                                color: modelData.allocatedQty < 0 ? "#8F2D2D" : "#4D617B"
                                                wrapMode: Text.WordWrap
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 10
                                    color: "#F9FBFE"
                                    border.color: "#DCE6F2"

                                    ListView {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        clip: true
                                        spacing: 6
                                        model: ordersViewModel.selectedOrderItems

                                        delegate: Rectangle {
                                            property var itemData: modelData

                                            width: ListView.view.width
                                            height: itemData.lotSummary && itemData.lotSummary.length > 0 ? 88 : 72
                                            radius: 8
                                            color: "#FFFFFF"
                                            border.width: 1
                                            border.color: "#DCE6F2"

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 10
                                                anchors.rightMargin: 10
                                                spacing: 8

                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 2

                                                    Text {
                                                        text: itemData.productName + " | SL: " + itemData.qty
                                                        font.pixelSize: 13
                                                        font.weight: Font.DemiBold
                                                        color: "#1D3047"
                                                    }

                                                    Text {
                                                        text: "Đơn giá: " + itemData.unitPriceText
                                                              + " | Giảm: " + itemData.discountText
                                                              + " | Thành tiền: " + itemData.lineTotalText
                                                        font.pixelSize: 12
                                                        color: "#4D617B"
                                                        elide: Text.ElideRight
                                                        width: parent.width
                                                    }

                                                    Text {
                                                        visible: itemData.lotSummary && itemData.lotSummary.length > 0
                                                        text: "Phân lô: " + itemData.lotSummary
                                                        font.pixelSize: 11
                                                        color: "#58708F"
                                                        elide: Text.ElideRight
                                                        width: parent.width
                                                    }
                                                }

                                                ActionButton {
                                                    text: "Nạp"
                                                    fillColor: "#2D6CDF"
                                                    enabled: ordersViewModel.canEditDraft
                                                    onClicked: root.loadItemToForm(itemData)
                                                }

                                                ActionButton {
                                                    text: "Xóa"
                                                    fillColor: "#C05A2A"
                                                    enabled: ordersViewModel.canEditDraft
                                                    onClicked: {
                                                        if (ordersViewModel.removeDraftItem(itemData.orderItemId)) {
                                                            appViewModel.refreshOverview()
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    ActionButton {
                                        text: "Confirm (trừ kho)"
                                        fillColor: "#2E8B57"
                                        enabled: ordersViewModel.canConfirm
                                        onClicked: {
                                            if (ordersViewModel.confirmSelectedOrder()) {
                                                appViewModel.refreshOverview()
                                                productsViewModel.reload()
                                                inventoryViewModel.reload()
                                            }
                                        }
                                    }

                                    ActionButton {
                                        text: "Void (hoàn kho)"
                                        fillColor: "#C05A2A"
                                        enabled: ordersViewModel.canVoid
                                        onClicked: {
                                            if (ordersViewModel.voidSelectedOrder()) {
                                                appViewModel.refreshOverview()
                                                productsViewModel.reload()
                                                inventoryViewModel.reload()
                                            }
                                        }
                                    }

                                    ActionButton {
                                        text: "Sang thanh toán"
                                        fillColor: "#255D9C"
                                        enabled: ordersViewModel.canOpenPayment
                                        onClicked: {
                                            paymentsViewModel.focusOrder(ordersViewModel.selectedOrderCustomerId,
                                                                         ordersViewModel.selectedOrderId)
                                            appViewModel.currentSection = "Payments"
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
                        visible: ordersViewModel.lastError.length > 0
                        color: "#FFECEC"
                        border.color: "#F0BABA"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            verticalAlignment: Text.AlignVCenter
                            text: ordersViewModel.lastError
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
