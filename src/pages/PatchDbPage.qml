import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import IstowUpdater

Item {
    id: root

    // ── PatchDbModel instance ───────────────────────
    PatchDbModel {
        id: patchModel
    }

    // ── FileDialog ──────────────────────────────────
    FileDialog {
        id: fileDialog
        title: "Pilih file .istow"
        nameFilters: ["iStow Files (*.istow)"]
        onAccepted: {
            let path = selectedFile.toString()
            selectedFilePath.text = path
            selectedTable = ""
            showOnlyDiff = true
            patchModel.loadAndCompare(path)
        }
    }

    property bool showOnlyDiff: true
    property string selectedTable: ""

    // ── Reactive data: reload when table or version changes ──
    property var currentColumns: []
    property var currentNewRows: []
    property var currentOldRows: []
    property var currentStats: ({})
    property int lastDataVersion: -1

    function loadTableData() {
        if (!selectedTable || !patchModel.loaded) {
            currentColumns = []
            currentNewRows = []
            currentOldRows = []
            currentStats = ({})
            return
        }
        currentColumns = patchModel.getColumns(selectedTable)
        currentNewRows = patchModel.getNewDbRows(selectedTable)
        currentOldRows = patchModel.getOldDbRows(selectedTable)
        currentStats = patchModel.getTableStats(selectedTable)
    }

    onSelectedTableChanged: loadTableData()

    Connections {
        target: patchModel
        function onDataVersionChanged() {
            if (root.lastDataVersion !== patchModel.dataVersion) {
                root.lastDataVersion = patchModel.dataVersion
                root.loadTableData()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ══════════════════════════════════════════════
        // Header
        // ══════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#1A3A5C"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 16

                Text {
                    text: "🔧 Patch DB Data"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#FFFFFF"
                }

                Item { Layout.fillWidth: true }

                // Legend
                Row {
                    spacing: 14
                    Row {
                        spacing: 4
                        Rectangle { width: 10; height: 10; radius: 2; color: "#FEF9C3"; border.color: "#F59E0B"; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Beda"; font.pixelSize: 11; color: "#CBD5E0"; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Row {
                        spacing: 4
                        Rectangle { width: 10; height: 10; radius: 2; color: "#DCFCE7"; border.color: "#22C55E"; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Baru"; font.pixelSize: 11; color: "#CBD5E0"; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Row {
                        spacing: 4
                        Rectangle { width: 10; height: 10; radius: 2; color: "#FEE2E2"; border.color: "#EF4444"; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Old Only"; font.pixelSize: 11; color: "#CBD5E0"; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Row {
                        spacing: 4
                        Rectangle { width: 10; height: 10; radius: 2; color: "#EFF6FF"; border.color: "#3B82F6"; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Pending"; font.pixelSize: 11; color: "#CBD5E0"; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }
        }

        // ══════════════════════════════════════════════
        // File Picker Bar
        // ══════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            height: 52
            color: "#F8FAFC"
            border.color: "#E2E8F0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Button {
                    text: "📁 Browse"
                    font.pixelSize: 12
                    font.bold: true
                    implicitHeight: 34

                    background: Rectangle {
                        radius: 6
                        color: parent.hovered ? "#2B6CB0" : "#1A3A5C"
                    }
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: fileDialog.open()
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 34
                    radius: 6
                    color: "#FFFFFF"
                    border.color: "#CBD5E0"
                    border.width: 1

                    Text {
                        id: selectedFilePath
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        text: "Belum ada file dipilih"
                        font.pixelSize: 12
                        color: text === "Belum ada file dipilih" ? "#A0AEC0" : "#2D3748"
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideMiddle
                    }
                }

                // Ship badges (when loaded)
                Row {
                    spacing: 6
                    visible: patchModel.loaded

                    Rectangle {
                        width: sn.implicitWidth + 16; height: 26; radius: 13; color: "#1A3A5C"
                        Text { id: sn; anchors.centerIn: parent; text: "🚢 " + (patchModel.shipDetails["nama"] ?? ""); font.pixelSize: 11; font.bold: true; color: "#FFF" }
                    }
                    Rectangle {
                        width: db.implicitWidth + 16; height: 26; radius: 13; color: "#374151"
                        Text { id: db; anchors.centerIn: parent; text: "Target: " + (patchModel.shipDetails["dbid"] ?? ""); font.pixelSize: 10; color: "#E5E7EB" }
                    }
                    Rectangle {
                        width: newDbBadge.implicitWidth + 16; height: 26; radius: 13; color: "#065F46"
                        Text { id: newDbBadge; anchors.centerIn: parent; text: "📦 File DB Baru: " + patchModel.newDbFileName; font.pixelSize: 10; font.bold: true; color: "#D1FAE5" }
                    }
                }
            }
        }

        // ══════════════════════════════════════════════
        // Status bar
        // ══════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            height: patchModel.statusMessage !== "" ? 32 : 0
            visible: patchModel.statusMessage !== ""
            color: {
                if (patchModel.statusMessage.includes("❌")) return "#FEF2F2"
                if (patchModel.statusMessage.includes("✅")) return "#F0FDF4"
                return "#FFFBEB"
            }
            border.color: {
                if (patchModel.statusMessage.includes("❌")) return "#FECACA"
                if (patchModel.statusMessage.includes("✅")) return "#BBF7D0"
                return "#FDE68A"
            }
            border.width: 1

            Text {
                anchors.fill: parent
                anchors.leftMargin: 16
                text: patchModel.statusMessage
                font.pixelSize: 11
                color: "#4A5568"
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        // ══════════════════════════════════════════════
        // Loading
        // ══════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            height: 50
            visible: patchModel.comparing
            color: "#EFF6FF"

            Row {
                anchors.centerIn: parent
                spacing: 10

                BusyIndicator { running: patchModel.comparing; implicitWidth: 24; implicitHeight: 24 }
                Text { text: "Membandingkan data..."; font.pixelSize: 13; color: "#1E40AF"; anchors.verticalCenter: parent.verticalCenter }
            }
        }

        // ══════════════════════════════════════════════
        // MAIN CONTENT: Sidebar + 2 panels
        // ══════════════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            visible: patchModel.loaded

            // ── LEFT SIDEBAR: Table List ────────────────
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 220
                Layout.minimumWidth: 180
                color: "#F1F5F9"
                border.color: "#E2E8F0"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Sidebar header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        color: "#E2E8F0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 8
                            spacing: 4

                            Text {
                                text: "📋 Tabel"
                                font.pixelSize: 13
                                font.bold: true
                                color: "#1E293B"
                            }

                            Item { Layout.fillWidth: true }

                            // Filter buttons
                            Rectangle {
                                width: diffBtn.implicitWidth + 8
                                height: 22
                                radius: 11
                                color: showOnlyDiff ? "#FEF9C3" : "#FFFFFF"
                                border.color: showOnlyDiff ? "#F59E0B" : "#CBD5E0"
                                border.width: 1

                                Text {
                                    id: diffBtn
                                    anchors.centerIn: parent
                                    text: "Δ " + patchModel.diffTableList.length
                                    font.pixelSize: 10
                                    font.bold: showOnlyDiff
                                    color: showOnlyDiff ? "#854D0E" : "#64748B"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: showOnlyDiff = true
                                }
                            }

                            Rectangle {
                                width: allBtn.implicitWidth + 8
                                height: 22
                                radius: 11
                                color: !showOnlyDiff ? "#EFF6FF" : "#FFFFFF"
                                border.color: !showOnlyDiff ? "#3B82F6" : "#CBD5E0"
                                border.width: 1

                                Text {
                                    id: allBtn
                                    anchors.centerIn: parent
                                    text: "All " + patchModel.allTableList.length
                                    font.pixelSize: 10
                                    font.bold: !showOnlyDiff
                                    color: !showOnlyDiff ? "#1E40AF" : "#64748B"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: showOnlyDiff = false
                                }
                            }
                        }
                    }

                    // Table list
                    ListView {
                        id: tableListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: showOnlyDiff ? patchModel.diffTableList : patchModel.allTableList
                        currentIndex: -1

                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            id: tableItem
                            width: tableListView.width
                            height: 42
                            color: {
                                if (selectedTable === modelData) return "#DBEAFE"
                                return itemMouse.containsMouse ? "#E2E8F0" : "transparent"
                            }
                            border.color: selectedTable === modelData ? "#3B82F6" : "transparent"
                            border.width: selectedTable === modelData ? 2 : 0

                            property bool isDiff: patchModel.diffTableList.indexOf(modelData) >= 0

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 8
                                spacing: 6

                                // Diff indicator dot
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: tableItem.isDiff ? "#F59E0B" : "#22C55E"
                                    visible: true
                                }

                                Text {
                                    text: modelData
                                    font.pixelSize: 12
                                    font.bold: selectedTable === modelData
                                    color: selectedTable === modelData ? "#1E40AF" : "#334155"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }

                            MouseArea {
                                id: itemMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    selectedTable = modelData
                                }
                            }
                        }
                    }
                }
            }

            // ── MAIN AREA: 2 panels ────────────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // "Select a table" state
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: selectedTable === ""

                    Text { text: "👈"; font.pixelSize: 48; Layout.alignment: Qt.AlignHCenter }
                    Text { text: "Pilih tabel dari daftar di kiri"; font.pixelSize: 15; color: "#94A3B8"; Layout.alignment: Qt.AlignHCenter }
                    Text { text: "untuk melihat dan membandingkan data"; font.pixelSize: 12; color: "#CBD5E0"; Layout.alignment: Qt.AlignHCenter }
                }

                // Table stats bar + panels
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    visible: selectedTable !== ""

                    // Stats bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        color: "#FFFFFF"
                        border.color: "#E2E8F0"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12

                            Text {
                                text: "📊 " + selectedTable
                                font.pixelSize: 14
                                font.bold: true
                                color: "#1E293B"
                            }

                            Text {
                                text: "PK: " + (patchModel.loaded ? patchModel.getPrimaryKey(selectedTable) : "")
                                font.pixelSize: 11
                                color: "#64748B"
                                font.family: "Consolas"
                            }

                            Item { Layout.fillWidth: true }

                            Row {
                                spacing: 8

                                Rectangle {
                                    visible: (currentStats.newOnly || 0) > 0
                                    width: noTxt.implicitWidth + 14; height: 22; radius: 11
                                    color: "#DCFCE7"; border.color: "#86EFAC"
                                    Text { id: noTxt; anchors.centerIn: parent; text: "+" + (currentStats.newOnly || 0) + " baru"; font.pixelSize: 10; font.bold: true; color: "#166534" }
                                }
                                Rectangle {
                                    visible: (currentStats.diffCount || 0) > 0
                                    width: dcTxt.implicitWidth + 14; height: 22; radius: 11
                                    color: "#FEF9C3"; border.color: "#FDE68A"
                                    Text { id: dcTxt; anchors.centerIn: parent; text: (currentStats.diffCount || 0) + " beda"; font.pixelSize: 10; font.bold: true; color: "#854D0E" }
                                }
                                Rectangle {
                                    visible: (currentStats.oldOnly || 0) > 0
                                    width: ooTxt.implicitWidth + 14; height: 22; radius: 11
                                    color: "#FEE2E2"; border.color: "#FECACA"
                                    Text { id: ooTxt; anchors.centerIn: parent; text: (currentStats.oldOnly || 0) + " old only"; font.pixelSize: 10; font.bold: true; color: "#991B1B" }
                                }
                                Rectangle {
                                    visible: (currentStats.matchCount || 0) > 0
                                    width: mcTxt.implicitWidth + 14; height: 22; radius: 11
                                    color: "#F1F5F9"; border.color: "#CBD5E0"
                                    Text { id: mcTxt; anchors.centerIn: parent; text: "✓ " + (currentStats.matchCount || 0); font.pixelSize: 10; color: "#64748B" }
                                }
                            }
                        }
                    }

                    // Two panels side by side
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        // Left: DB Baru (readonly)
                        PatchDbTableView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            mode: "readonly"
                            tableName: selectedTable
                            patchModel: patchModel
                            primaryKey: patchModel.loaded ? patchModel.getPrimaryKey(selectedTable) : ""
                            columns: currentColumns
                            rows: currentNewRows

                            onDataRefreshNeeded: root.loadTableData()
                        }

                        // Divider
                        Rectangle {
                            Layout.fillHeight: true
                            width: 2
                            color: "#CBD5E0"
                        }

                        // Right: DB Target (editable)
                        PatchDbTableView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            mode: "editable"
                            tableName: selectedTable
                            patchModel: patchModel
                            primaryKey: patchModel.loaded ? patchModel.getPrimaryKey(selectedTable) : ""
                            columns: currentColumns
                            rows: currentOldRows

                            onDataRefreshNeeded: root.loadTableData()
                        }
                    }
                }
            }
        }

        // ══════════════════════════════════════════════
        // Welcome State (before loading)
        // ══════════════════════════════════════════════
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !patchModel.loaded && !patchModel.comparing

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 16

                Text {
                    text: "🔧"
                    font.pixelSize: 64
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Patch DB Data"
                    font.pixelSize: 22
                    font.bold: true
                    color: "#1E293B"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Pilih file .istow untuk membandingkan data database.\nAnda dapat melihat perbedaan dan memperbaiki (patch) data secara selektif."
                    font.pixelSize: 13
                    color: "#64748B"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                    wrapMode: Text.Wrap
                    Layout.maximumWidth: 420
                }

                Button {
                    text: "📁 Pilih File .istow"
                    font.pixelSize: 14
                    font.bold: true
                    implicitHeight: 44
                    implicitWidth: 200
                    Layout.alignment: Qt.AlignHCenter

                    background: Rectangle {
                        radius: 10
                        color: parent.hovered ? "#2B6CB0" : "#1A3A5C"
                    }
                    contentItem: Text {
                        text: parent.text; font: parent.font; color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: fileDialog.open()
                }
            }
        }
    }
}
