import QtQuick
import QtQuick.Controls
import StockMaster

ApplicationWindow {
    id: window

    width: 1320
    height: 840
    visible: true
    title: appViewModel.appTitle

    minimumWidth: 980
    minimumHeight: 640

    AppShell {
        anchors.fill: parent
    }
}
