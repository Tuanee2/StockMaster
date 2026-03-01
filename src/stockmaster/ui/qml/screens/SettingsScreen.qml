import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StockMaster

Item {
    id: root

    readonly property bool hasDownloadedPackage: settingsViewModel.downloadedFilePath.length > 0
    readonly property color statusFill: hasDownloadedPackage
        ? "#E6F6EA"
        : (settingsViewModel.updateAvailable ? "#EEF4FF" : "#F4F7FB")
    readonly property color statusBorder: hasDownloadedPackage
        ? "#B9DFC1"
        : (settingsViewModel.updateAvailable ? "#B7D2FF" : "#D8E2EE")
    readonly property color statusTextColor: hasDownloadedPackage
        ? "#24543A"
        : (settingsViewModel.updateAvailable ? "#214875" : "#40536C")

    onVisibleChanged: {
        if (visible) {
            settingsViewModel.checkForUpdates()
        }
    }

    Component.onCompleted: {
        if (visible) {
            settingsViewModel.checkForUpdates()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        SectionHeader {
            Layout.fillWidth: true
            title: "Cập nhật phần mềm"
            subtitle: "Khi mở mục này, ứng dụng sẽ tự kiểm tra, tải và mở gói cập nhật nếu GitHub Release có bản mới phù hợp."
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 14
            color: "#FFFFFF"
            border.color: "#D8E2EE"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: statusContent.implicitHeight + 28
                    radius: 12
                    color: root.statusFill
                    border.color: root.statusBorder

                    Column {
                        id: statusContent

                        x: 14
                        y: 14
                        width: parent.width - 28
                        spacing: 6

                        Text {
                            text: "Trạng thái cập nhật"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: "#1B2B40"
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: settingsViewModel.statusText
                            font.pixelSize: 13
                            color: root.statusTextColor
                            lineHeight: 1.35
                        }

                        Text {
                            width: parent.width
                            visible: settingsViewModel.downloadProgress >= 0 && settingsViewModel.downloading
                            text: settingsViewModel.downloadProgress >= 0
                                ? "Tiến độ tải: " + settingsViewModel.downloadProgress + "%"
                                : ""
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: "#365478"
                        }
                    }
                }

                ProgressBar {
                    Layout.fillWidth: true
                    visible: settingsViewModel.downloading
                    from: 0
                    to: 100
                    value: settingsViewModel.downloadProgress >= 0
                        ? settingsViewModel.downloadProgress
                        : 0
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: versionGrid.implicitHeight + 28
                    radius: 12
                    color: "#F6FAFF"
                    border.color: "#D8E6F4"

                    GridLayout {
                        id: versionGrid

                        x: 14
                        y: 14
                        width: parent.width - 28
                        columns: width >= 720 ? 2 : 1
                        columnSpacing: 18
                        rowSpacing: 10

                        Column {
                            spacing: 4

                            Text {
                                text: "Phiên bản hiện tại"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                color: "#4A5B72"
                            }

                            Text {
                                text: settingsViewModel.currentVersion
                                font.pixelSize: 24
                                font.weight: Font.DemiBold
                                color: "#142033"
                            }
                        }

                        Column {
                            spacing: 4

                            Text {
                                text: "Phiên bản mới nhất trên release"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                color: "#4A5B72"
                            }

                            Text {
                                text: settingsViewModel.latestVersion.length > 0
                                    ? settingsViewModel.latestVersion
                                    : "Chưa xác định"
                                font.pixelSize: 24
                                font.weight: Font.DemiBold
                                color: settingsViewModel.updateAvailable ? "#1F5FA6" : "#142033"
                            }
                        }
                    }
                }

                Button {
                    id: updateButton

                    Layout.fillWidth: true
                    implicitHeight: 48
                    enabled: !settingsViewModel.busy
                    text: settingsViewModel.actionLabel
                    hoverEnabled: true
                    onClicked: settingsViewModel.checkForUpdates()

                    background: Rectangle {
                        radius: 12
                        color: !updateButton.enabled
                            ? "#B9CCE2"
                            : (updateButton.down
                                ? "#1C4D82"
                                : (updateButton.hovered ? "#2B6CB0" : "#255D9C"))
                        border.width: 1
                        border.color: !updateButton.enabled ? "#AFC1D8" : "#1A4A7B"
                    }

                    contentItem: Text {
                        text: updateButton.text
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: safetyContent.implicitHeight + 28
                    radius: 12
                    color: "#FBFCFE"
                    border.color: "#D8E2EE"

                    Column {
                        id: safetyContent

                        x: 14
                        y: 14
                        width: parent.width - 28
                        spacing: 10

                        Text {
                            text: "An toàn dữ liệu"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: "#1B2B40"
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "Bản cập nhật sẽ tải gói cài đặt mới và tự mở gói sau khi tải xong. Dữ liệu cũ vẫn được giữ nguyên vì SQLite đang nằm trong thư mục AppData riêng của người dùng."
                            font.pixelSize: 13
                            color: "#465970"
                            lineHeight: 1.35
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WrapAnywhere
                            text: "DB hiện tại: " + settingsViewModel.databasePath
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: "#27435E"
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WrapAnywhere
                            visible: root.hasDownloadedPackage
                            text: "Gói đã tải: " + settingsViewModel.downloadedFilePath
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: "#24543A"
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
}
