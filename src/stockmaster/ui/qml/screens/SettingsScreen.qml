import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import StockMaster

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Thiết lập"
            subtitle: "Khung cho cấu hình đồng bộ, backup/restore và chính sách dữ liệu."
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#FFFFFF"
            border.color: "#D8E2EE"

            Column {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8

                Text {
                    text: "Tính năng sẽ triển khai"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: "#1B2B40"
                }

                Text {
                    text: "- Cấu hình SQLite path\n- Cấu hình Supabase sync\n- Backup/Restore dữ liệu\n- Log và diagnostics"
                    font.pixelSize: 13
                    color: "#4A5B72"
                    lineHeight: 1.35
                }
            }
        }
    }
}
