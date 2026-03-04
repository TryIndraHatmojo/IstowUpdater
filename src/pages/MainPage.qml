import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IstowUpdater

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("iStow Updater")

    color: "#F0F4F8"

    // ── Model ──────────────────────────────────────────────
    ShipModel {
        id: shipModel
    }

    // ── StackView untuk navigasi ───────────────────────────
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: mainContent
    }

    // ── Halaman Utama ──────────────────────────────────────
    Component {
        id: mainContent

        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // ── Header ─────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    color: "#1A3A5C"

                    RowLayout {
                        anchors {
                            fill: parent
                            leftMargin: 20
                            rightMargin: 20
                        }

                        Text {
                            text: "iStow Updater Form"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#FFFFFF"
                        }

                        Item { Layout.fillWidth: true }
                    }
                }

                // ── Konten Utama ────────────────────────────
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 48, 480)
                        spacing: 0

                        // Card
                        Rectangle {
                            Layout.fillWidth: true
                            height: cardContent.implicitHeight + 48
                            radius: 12
                            color: "#FFFFFF"

                            // Border / shadow simulasi
                            Rectangle {
                                anchors { fill: parent; margins: -1 }
                                radius: parent.radius + 1
                                color: "transparent"
                                border.color: "#E2E8F0"
                                border.width: 1
                                z: -1
                            }

                            ColumnLayout {
                                id: cardContent
                                anchors {
                                    fill: parent
                                    margins: 24
                                }
                                spacing: 20

                                Text {
                                    text: "Pilih Kapal"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#1A3A5C"
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: "#E2E8F0"
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        text: "Nama Kapal"
                                        font.pixelSize: 13
                                        color: "#4A5568"
                                    }

                                    ComboBox {
                                        id: shipComboBox
                                        Layout.fillWidth: true
                                        height: 44
                                        model: shipModel.ships
                                        textRole: "nama"
                                        valueRole: "idship"
                                        displayText: currentIndex === -1 ? "— Pilih Kapal —" : currentText
                                        font.pixelSize: 14

                                        background: Rectangle {
                                            radius: 8
                                            color: shipComboBox.hovered ? "#EBF4FF" : "#FFFFFF"
                                            border.color: shipComboBox.activeFocus ? "#2B6CB0" : "#CBD5E0"
                                            border.width: shipComboBox.activeFocus ? 2 : 1
                                        }

                                        contentItem: Text {
                                            leftPadding: 12
                                            rightPadding: 8
                                            text: shipComboBox.displayText
                                            font: shipComboBox.font
                                            color: shipComboBox.currentIndex === -1 ? "#A0AEC0" : "#2D3748"
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }

                                        delegate: ItemDelegate {
                                            width: shipComboBox.width
                                            required property var modelData
                                            required property int index

                                            contentItem: ColumnLayout {
                                                spacing: 2
                                                Text {
                                                    text: modelData["nama"] ?? ""
                                                    font.pixelSize: 14
                                                    font.bold: true
                                                    color: "#2D3748"
                                                    elide: Text.ElideRight
                                                    Layout.fillWidth: true
                                                }
                                                Text {
                                                    text: modelData["company"] ?? ""
                                                    font.pixelSize: 11
                                                    color: "#718096"
                                                    elide: Text.ElideRight
                                                    visible: (modelData["company"] ?? "") !== ""
                                                    Layout.fillWidth: true
                                                }
                                            }

                                            highlighted: shipComboBox.highlightedIndex === index
                                            background: Rectangle {
                                                color: highlighted ? "#EBF4FF" : "transparent"
                                            }
                                        }

                                        popup: Popup {
                                            y: shipComboBox.height + 4
                                            width: shipComboBox.width
                                            implicitHeight: contentItem.implicitHeight
                                            padding: 0

                                            contentItem: ListView {
                                                clip: true
                                                implicitHeight: Math.min(contentHeight, 240)
                                                model: shipComboBox.delegateModel
                                                currentIndex: shipComboBox.highlightedIndex
                                                ScrollIndicator.vertical: ScrollIndicator {}
                                            }

                                            background: Rectangle {
                                                radius: 8
                                                color: "#FFFFFF"
                                                border.color: "#CBD5E0"
                                                border.width: 1
                                            }
                                        }

                                        onCurrentIndexChanged: {
                                            if (currentIndex >= 0) {
                                                let ship = shipModel.ships[currentIndex]
                                                console.log("[MainPage] Kapal dipilih:", ship["nama"], "| ID:", ship["idship"])
                                            }
                                        }
                                    }

                                    // Info kapal terpilih
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: shipInfo.implicitHeight + 16
                                        radius: 8
                                        color: "#EBF8FF"
                                        border.color: "#BEE3F8"
                                        border.width: 1
                                        visible: shipComboBox.currentIndex >= 0

                                        ColumnLayout {
                                            id: shipInfo
                                            anchors { fill: parent; margins: 12 }
                                            spacing: 4

                                            property var selectedShip: shipComboBox.currentIndex >= 0
                                                                       ? shipModel.getShipAt(shipComboBox.currentIndex)
                                                                       : null

                                            Text { text: "Tipe: "             + (shipInfo.selectedShip?.["tipe"]     ?? "-"); font.pixelSize: 12; color: "#2C5282" }
                                            Text { text: "Perusahaan: "       + (shipInfo.selectedShip?.["company"]  ?? "-"); font.pixelSize: 12; color: "#2C5282" }
                                            Text { text: "DB ID: "            + (shipInfo.selectedShip?.["dbid"]     ?? "-"); font.pixelSize: 12; color: "#2C5282" }
                                            Text { text: "Voyage Terakhir: " + (shipInfo.selectedShip?.["novoyage"] ?? "-"); font.pixelSize: 12; color: "#2C5282" }
                                        }
                                    }
                                }

                                Button {
                                    Layout.fillWidth: true
                                    height: 44
                                    text: "Lanjutkan"
                                    enabled: shipComboBox.currentIndex >= 0

                                    background: Rectangle {
                                        radius: 8
                                        color: parent.enabled ? (parent.hovered ? "#2B6CB0" : "#1A3A5C") : "#CBD5E0"
                                    }

                                    contentItem: Text {
                                        text: parent.text
                                        font.pixelSize: 14
                                        font.bold: true
                                        color: parent.enabled ? "#FFFFFF" : "#718096"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    onClicked: {
                                        let ship = shipModel.ships[shipComboBox.currentIndex]
                                        console.log("[MainPage] Lanjutkan dengan kapal:", ship["nama"])
                                        // TODO: navigasi ke halaman berikutnya
                                    }
                                }

                                // ── Separator ──────────────────
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: "#E2E8F0"
                                    Layout.topMargin: 4
                                }

                                // ── Tombol Import .istow ───────
                                Button {
                                    Layout.fillWidth: true
                                    height: 44
                                    text: "📦 Import dari file (.istow)"
                                    font.pixelSize: 14

                                    background: Rectangle {
                                        radius: 8
                                        color: parent.hovered ? "#2F855A" : "#276749"
                                    }

                                    contentItem: Text {
                                        text: parent.text
                                        font: parent.font
                                        color: "#FFFFFF"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    onClicked: {
                                        console.log("[MainPage] Navigasi ke Import .istow")
                                        stackView.push(importPageComponent)
                                    }
                                }
                            }
                        }

                        Text {
                            Layout.topMargin: 16
                            Layout.alignment: Qt.AlignHCenter
                            text: "Tidak ada data kapal tersedia."
                            font.pixelSize: 13
                            color: "#A0AEC0"
                            visible: shipModel.ships.length === 0
                        }
                    }
                }
            }
        }
    }

    // ── Component: Halaman Import .istow ───────────────────
    Component {
        id: importPageComponent

        BrowseDotIstow {
            shipModel: shipModel

            onBackRequested: {
                stackView.pop()
            }
        }
    }
}
