import QtQuick
import QtQuick.Controls
import QtQuick.Window
import StockMaster

ApplicationWindow {
    id: window

    width: 1320
    height: 840
    visible: true
    visibility: Window.Maximized
    title: appViewModel.appTitle

    minimumWidth: 980
    minimumHeight: 640

    AppShell {
        anchors.fill: parent
    }
}
