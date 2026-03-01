import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import StockMaster

Flickable {
    id: root

    contentWidth: width
    contentHeight: contentColumn.implicitHeight + 24
    clip: true

    property real trendPeak: Math.max(1, appViewModel.monthlyTrendPeakVnd)

    function alertFill(severity) {
        if (severity === "danger") {
            return "#FFF0F0"
        }
        if (severity === "warning") {
            return "#FFF7E8"
        }
        return "#EEF5FF"
    }

    function alertBorder(severity) {
        if (severity === "danger") {
            return "#E6B3B3"
        }
        if (severity === "warning") {
            return "#E6C781"
        }
        return "#BBD2F1"
    }

    function alertText(severity) {
        if (severity === "danger") {
            return "#9C3C3C"
        }
        if (severity === "warning") {
            return "#7A5713"
        }
        return "#2C5D99"
    }

    function barHeight(value, availableHeight) {
        if (value <= 0) {
            return 6
        }

        return Math.max(12, availableHeight * value / trendPeak)
    }

    component LegendDot: Rectangle {
        implicitWidth: 10
        implicitHeight: 10
        radius: 5
    }

    ColumnLayout {
        id: contentColumn

        width: root.width
        spacing: 18

        SectionHeader {
            Layout.fillWidth: true
            title: "Tổng quan vận hành và phân tích"
            subtitle: "Dashboard kết hợp số liệu xử lý hằng ngày với xu hướng bán hàng và thu tiền."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 1200 ? 5 : width >= 900 ? 3 : 2
            rowSpacing: 12
            columnSpacing: 12

            MetricCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 124
                title: "Khách hàng"
                value: appViewModel.customerCount.toString()
                accent: "#2D6CDF"
            }

            MetricCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 124
                title: "Sản phẩm"
                value: appViewModel.productCount.toString()
                accent: "#2E8B57"
            }

            MetricCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 124
                title: "Đơn mở"
                value: appViewModel.openOrderCount.toString()
                accent: "#C9861A"
            }

            MetricCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 124
                title: "Sắp hết hàng"
                value: appViewModel.lowStockCount.toString()
                accent: "#BA4A4A"
            }

            MetricCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 124
                title: "Mặt hàng sắp hết hạn"
                value: appViewModel.expiringSoonProductCount.toString()
                accent: "#CC7A1A"
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 980 ? 2 : 1
            rowSpacing: 12
            columnSpacing: 12

            Rectangle {
                id: receivableCard

                Layout.fillWidth: true
                Layout.preferredHeight: parent.width >= 980
                    ? Math.max(200, alertsCard.implicitHeight)
                    : implicitHeight
                Layout.minimumHeight: Layout.preferredHeight
                implicitHeight: receivableContent.implicitHeight + 28
                radius: 12
                border.color: "#C9D8EA"

                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: "#EEF5FF"
                    }
                    GradientStop {
                        position: 1.0
                        color: "#FFFFFF"
                    }
                }

                ColumnLayout {
                    id: receivableContent

                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Rectangle {
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: receivableBadge.implicitWidth + 18
                        radius: 8
                        color: "#DCEBFF"
                        border.color: "#A9C8F3"

                        Text {
                            id: receivableBadge

                            anchors.centerIn: parent
                            text: "Chỉ số trọng yếu"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: "#2D6CDF"
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.minimumHeight: 4
                    }

                    Text {
                        text: "Công nợ phải thu"
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        color: "#304D70"
                    }

                    Text {
                        text: appViewModel.receivableVndText
                        font.pixelSize: 40
                        font.weight: Font.DemiBold
                        color: "#142033"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Tổng công nợ của các đơn đang ở trạng thái Confirmed hoặc PartiallyPaid. Nên đối chiếu cùng khối thu tiền ở biểu đồ bên dưới."
                        wrapMode: Text.WordWrap
                        font.pixelSize: 13
                        color: "#58708F"
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.minimumHeight: 4
                    }
                }
            }

            Rectangle {
                id: alertsCard

                Layout.fillWidth: true
                Layout.preferredHeight: implicitHeight
                Layout.minimumHeight: implicitHeight
                implicitHeight: alertsContent.implicitHeight + 28
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"
                clip: true

                Column {
                    id: alertsContent

                    x: 14
                    y: 14
                    width: Math.max(0, parent.width - 28)
                    spacing: 8

                    Text {
                        text: "Cảnh báo vận hành"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Text {
                        visible: appViewModel.operationalAlerts.length === 0
                        text: "Chưa có cảnh báo cần xử lý ngay."
                        font.pixelSize: 13
                        color: "#7286A0"
                    }

                    Repeater {
                        model: appViewModel.operationalAlerts

                        delegate: Rectangle {
                            property var alertData: modelData

                            width: alertsContent.width
                            height: 56
                            radius: 10
                            color: root.alertFill(alertData.severity)
                            border.width: 1
                            border.color: root.alertBorder(alertData.severity)

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    radius: 7
                                    color: root.alertBorder(alertData.severity)
                                    Layout.preferredHeight: 24
                                    Layout.preferredWidth: badgeLabel.implicitWidth + 14

                                    Text {
                                        id: badgeLabel

                                        anchors.centerIn: parent
                                        text: alertData.badge
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                        color: root.alertText(alertData.severity)
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    Text {
                                        text: alertData.title
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: "#20324A"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: alertData.detail
                                        font.pixelSize: 12
                                        color: "#536883"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
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
            Layout.preferredHeight: 340
            radius: 12
            color: "#FFFFFF"
            border.color: "#D8E2EE"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: "Doanh số và thu tiền 7 ngày gần nhất"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            color: "#1B2B40"
                        }

                        Text {
                            text: "Cột xanh: doanh số đơn đã ghi nhận. Cột cam: tiền thu thực tế theo phiếu thu."
                            font.pixelSize: 12
                            color: "#58708F"
                        }
                    }

                    RowLayout {
                        spacing: 10

                        RowLayout {
                            spacing: 4

                            LegendDot {
                                color: "#2D6CDF"
                            }

                            Text {
                                text: "Doanh số"
                                font.pixelSize: 12
                                color: "#4A5B72"
                            }
                        }

                        RowLayout {
                            spacing: 4

                            LegendDot {
                                color: "#CC7A1A"
                            }

                            Text {
                                text: "Thu tiền"
                                font.pixelSize: 12
                                color: "#4A5B72"
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: "#F8FBFF"
                    border.color: "#DDE8F3"

                    Text {
                        anchors.centerIn: parent
                        visible: !appViewModel.hasMonthlyActivity
                        text: "Chưa có dữ liệu giao dịch trong 7 ngày gần đây."
                        font.pixelSize: 13
                        color: "#7286A0"
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10
                        visible: appViewModel.hasMonthlyActivity

                        Repeater {
                            model: appViewModel.monthlyTrends

                            delegate: Item {
                                property var trend: modelData

                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                Column {
                                    anchors.fill: parent
                                    spacing: 8

                                    Text {
                                        width: parent.width
                                        horizontalAlignment: Text.AlignHCenter
                                        text: trend.salesVnd > trend.collectedVnd
                                            ? trend.salesText
                                            : trend.collectedText
                                        font.pixelSize: 10
                                        color: "#59708C"
                                        elide: Text.ElideRight
                                    }

                                    Item {
                                        id: plotArea

                                        width: parent.width
                                        height: Math.max(56, parent.height - 34)

                                        Row {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.bottom: parent.bottom
                                            spacing: 6

                                            Repeater {
                                                model: [
                                                    {
                                                        "label": "Doanh số",
                                                        "color": "#2D6CDF",
                                                        "value": trend.salesVnd,
                                                        "valueText": trend.salesText
                                                    },
                                                    {
                                                        "label": "Thu tiền",
                                                        "color": "#CC7A1A",
                                                        "value": trend.collectedVnd,
                                                        "valueText": trend.collectedText
                                                    }
                                                ]

                                                delegate: Item {
                                                    property var barData: modelData

                                                    width: 20
                                                    height: plotArea.height
                                                    z: hoverArea.containsMouse ? 2 : 0

                                                    Rectangle {
                                                        id: valueTooltip

                                                        visible: hoverArea.containsMouse
                                                        x: Math.round((parent.width - width) / 2)
                                                        y: Math.max(0, barRect.y - height - 6)
                                                        width: tooltipText.implicitWidth + 14
                                                        height: tooltipText.implicitHeight + 8
                                                        radius: 8
                                                        color: "#1F3048"
                                                        opacity: 0.96

                                                        Text {
                                                            id: tooltipText

                                                            anchors.centerIn: parent
                                                            text: barData.label + ": " + barData.valueText
                                                            font.pixelSize: 10
                                                            font.weight: Font.DemiBold
                                                            color: "#FFFFFF"
                                                        }
                                                    }

                                                    Rectangle {
                                                        id: barRect

                                                        anchors.horizontalCenter: parent.horizontalCenter
                                                        anchors.bottom: parent.bottom
                                                        width: 16
                                                        height: root.barHeight(barData.value, plotArea.height - 18)
                                                        radius: 5
                                                        color: barData.color
                                                    }

                                                    MouseArea {
                                                        id: hoverArea

                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        acceptedButtons: Qt.NoButton
                                                        cursorShape: Qt.PointingHandCursor
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        width: parent.width
                                        horizontalAlignment: Text.AlignHCenter
                                        text: trend.monthLabel
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                        color: "#37506D"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 980 ? 2 : 1
            rowSpacing: 12
            columnSpacing: 12

            Rectangle {
                id: topCustomersCard

                Layout.fillWidth: true
                Layout.preferredHeight: implicitHeight
                Layout.minimumHeight: implicitHeight
                implicitHeight: topCustomersContent.implicitHeight + 28
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"
                clip: true

                Column {
                    id: topCustomersContent

                    x: 14
                    y: 14
                    width: Math.max(0, parent.width - 28)
                    spacing: 8

                    Text {
                        text: "Top khách hàng theo doanh số"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Text {
                        visible: appViewModel.topCustomers.length === 0
                        text: "Chưa có đơn đã ghi nhận để xếp hạng khách hàng."
                        font.pixelSize: 13
                        color: "#7286A0"
                    }

                    Repeater {
                        model: appViewModel.topCustomers

                        delegate: Rectangle {
                            property var customerData: modelData

                            width: topCustomersContent.width
                            height: 62
                            radius: 10
                            color: "#F8FBFF"
                            border.width: 1
                            border.color: "#DDE8F3"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    Layout.preferredWidth: 28
                                    Layout.preferredHeight: 28
                                    radius: 14
                                    color: "#DCEBFF"
                                    border.color: "#A9C8F3"

                                    Text {
                                        anchors.centerIn: parent
                                        text: (index + 1).toString()
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: "#2D6CDF"
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    Text {
                                        text: customerData.code + " - " + customerData.name
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: "#20324A"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: "Doanh số: " + customerData.salesText
                                              + " | Công nợ: " + customerData.receivableText
                                        font.pixelSize: 12
                                        color: "#536883"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: topProductsCard

                Layout.fillWidth: true
                Layout.preferredHeight: implicitHeight
                Layout.minimumHeight: implicitHeight
                implicitHeight: topProductsContent.implicitHeight + 28
                radius: 12
                color: "#FFFFFF"
                border.color: "#D8E2EE"
                clip: true

                Column {
                    id: topProductsContent

                    x: 14
                    y: 14
                    width: Math.max(0, parent.width - 28)
                    spacing: 8

                    Text {
                        text: "Top sản phẩm bán chạy"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: "#1B2B40"
                    }

                    Text {
                        visible: appViewModel.topProducts.length === 0
                        text: "Chưa có item đơn đã ghi nhận để xếp hạng sản phẩm."
                        font.pixelSize: 13
                        color: "#7286A0"
                    }

                    Repeater {
                        model: appViewModel.topProducts

                        delegate: Rectangle {
                            property var productData: modelData

                            width: topProductsContent.width
                            height: 62
                            radius: 10
                            color: "#F8FBFF"
                            border.width: 1
                            border.color: "#DDE8F3"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    Layout.preferredWidth: 28
                                    Layout.preferredHeight: 28
                                    radius: 14
                                    color: "#E3F6EA"
                                    border.color: "#B9DDC5"

                                    Text {
                                        anchors.centerIn: parent
                                        text: (index + 1).toString()
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: "#2E8B57"
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    Text {
                                        text: productData.sku + " - " + productData.name
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: "#20324A"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: "Đã bán: " + productData.soldQty
                                              + " | Doanh số: " + productData.salesText
                                        font.pixelSize: 12
                                        color: "#536883"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
