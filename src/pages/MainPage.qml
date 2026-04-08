import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IstowUpdater

Window {
    id: window
    width: 640
    height: 480
    visibility: Window.Maximized
    visible: true
    title: qsTr("iStow Updater")

    color: "#F0F4F8"

    ShipModel {
        id: shipModel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 64
            color: "#1A3A5C"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                Text {
                    text: "iStow Updater"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#FFFFFF"
                }

                Item { Layout.fillWidth: true }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#F7FAFC"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                TabBar {
                    id: mainTabBar
                    Layout.fillWidth: true
                    currentIndex: 0

                    TabButton { text: "Create/Update Assets and Sync DB Structure" }
                    TabButton { text: "Rollback Assets" }
                    TabButton { text: "Update Application" }
                    TabButton { text: "Rollback Application" }
                    TabButton { text: "Patch DB Data" }
                    TabButton { text: "Compare and Patch DB" }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: mainTabBar.currentIndex

                    Rectangle {
                        color: "transparent"
                        BrowseDotIstow {
                            anchors.fill: parent
                            shipModel: shipModel
                            showBackButton: false
                        }
                    }

                    RollbackAssetsPage {
                        anchors.fill: parent
                    }

                    UpdateApplicationPage {
                        anchors.fill: parent
                    }

                    RollbackApplicationPage {
                        anchors.fill: parent
                    }

                    PatchDbPage {
                        anchors.fill: parent
                    }

                    CompareAndPatchDbPage {
                        anchors.fill: parent
                    }
                }
            }
        }
    }
}
