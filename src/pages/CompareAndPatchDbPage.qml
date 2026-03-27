import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import IstowUpdater

Item {
    id: root

    // ── CompareAndPatchDbModel instance ──────────────
    CompareAndPatchDbModel {
        id: compareModel
    }

    // ── FileDialogs ─────────────────────────────────
    FileDialog {
        id: newDbDialog
        title: "Pilih New DB (.db)"
        nameFilters: ["Database Files (*.db)"]
        onAccepted: {
            let path = selectedFile.toString()
            newDbPathText.text = path
            compareModel.setNewDbPath(path)
        }
    }

    FileDialog {
        id: targetDbDialog
        title: "Pilih Target Patch DB (.db)"
        nameFilters: ["Database Files (*.db)"]
        onAccepted: {
            let path = selectedFile.toString()
            targetDbPathText.text = path
            compareModel.setTargetDbPath(path)
        }
    }

    // ── State ───────────────────────────────────────
    property bool showOnlyDiff: true
    property string selectedTable: ""

    // ── Reactive data ───────────────────────────────
    property var currentColumns: []
    property var currentNewRows: []
    property var currentOldRows: []
    property var currentStats: ({})
    property int lastDataVersion: -1

    // Derived: both paths selected
    property bool bothPathsSelected: compareModel.newDbPath !== "" && compareModel.targetDbPath !== ""

    function loadTableData() {
        if (!selectedTable || !compareModel.loaded) {
            currentColumns = []
            currentNewRows = []
            currentOldRows = []
            currentStats = ({})
            return
        }
        currentColumns = compareModel.getColumns(selectedTable)
        currentNewRows = compareModel.getNewDbRows(selectedTable)
        currentOldRows = compareModel.getOldDbRows(selectedTable)
        currentStats = compareModel.getTableStats(selectedTable)
    }

    onSelectedTableChanged: loadTableData()

    Connections {
        target: compareModel
        function onDataVersionChanged() {
            if (root.lastDataVersion !== compareModel.dataVersion) {
                root.lastDataVersion = compareModel.dataVersion
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
                        text: "🔀 Compare and Patch DB"
                        font.pixelSize: 20
                        font.bold: true
                        color: "#FFFFFF"
                    }

                    Item { Layout.fillWidth: true }

                    // Legend (visible when data loaded)
                    Row {
                        spacing: 14
                        visible: compareModel.loaded

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
            // File Pickers (2 rows)
            // ══════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                height: filePickerLayout.implicitHeight + 24
                color: "#F8FAFC"
                border.color: "#E2E8F0"
                border.width: 1

                ColumnLayout {
                    id: filePickerLayout
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Row 1: New DB
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: "New DB:"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#1E293B"
                            Layout.preferredWidth: 80
                        }

                        Button {
                            text: "📁 Browse"
                            font.pixelSize: 11
                            font.bold: true
                            implicitHeight: 32

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
                            onClicked: newDbDialog.open()
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            radius: 6
                            color: "#FFFFFF"
                            border.color: "#CBD5E0"
                            border.width: 1

                            Text {
                                id: newDbPathText
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                text: "Belum ada file dipilih"
                                font.pixelSize: 11
                                color: text === "Belum ada file dipilih" ? "#A0AEC0" : "#2D3748"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideMiddle
                            }
                        }
                    }

                    // Row 2: Target DB
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: "Target DB:"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#1E293B"
                            Layout.preferredWidth: 80
                        }

                        Button {
                            text: "📁 Browse"
                            font.pixelSize: 11
                            font.bold: true
                            implicitHeight: 32

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
                            onClicked: targetDbDialog.open()
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            radius: 6
                            color: "#FFFFFF"
                            border.color: "#CBD5E0"
                            border.width: 1

                            Text {
                                id: targetDbPathText
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                text: "Belum ada file dipilih"
                                font.pixelSize: 11
                                color: text === "Belum ada file dipilih" ? "#A0AEC0" : "#2D3748"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideMiddle
                            }
                        }
                    }
                }
            }

            // ══════════════════════════════════════════════
            // Action Buttons Row
            // ══════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                height: 52
                color: "#FFFFFF"
                border.color: "#E2E8F0"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    // Step 1: Preview Structure
                    Button {
                        text: "🔍 Preview Structure"
                        font.pixelSize: 12
                        font.bold: true
                        implicitHeight: 36
                        enabled: bothPathsSelected

                        background: Rectangle {
                            radius: 6
                            color: parent.enabled
                                ? (parent.hovered ? "#7C3AED" : "#6D28D9")
                                : "#CBD5E0"
                        }
                        contentItem: Text {
                            text: parent.text; font: parent.font
                            color: parent.enabled ? "#FFFFFF" : "#718096"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            selectedTable = ""
                            compareModel.previewStructure()
                        }
                    }

                    // Step 2: Apply Sync Structure
                    Button {
                        text: "⚡ Apply Sync Structure"
                        font.pixelSize: 12
                        font.bold: true
                        implicitHeight: 36
                        enabled: bothPathsSelected && compareModel.structurePreviewed && !compareModel.structureSynced

                        background: Rectangle {
                            radius: 6
                            color: parent.enabled
                                ? (parent.hovered ? "#059669" : "#10B981")
                                : "#CBD5E0"
                        }
                        contentItem: Text {
                            text: parent.text; font: parent.font
                            color: parent.enabled ? "#FFFFFF" : "#718096"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            compareModel.applySyncStructure()
                        }
                    }

                    // Sync Structure done indicator
                    Rectangle {
                        visible: compareModel.structureSynced
                        width: ssLabel.implicitWidth + 16
                        height: 28
                        radius: 14
                        color: "#DCFCE7"
                        border.color: "#86EFAC"

                        Text {
                            id: ssLabel
                            anchors.centerIn: parent
                            text: "✅ Structure Synced"
                            font.pixelSize: 11
                            font.bold: true
                            color: "#166534"
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Step 3: Sync DB Data
                    Button {
                        text: "📊 Sync DB Data"
                        font.pixelSize: 12
                        font.bold: true
                        implicitHeight: 36
                        enabled: bothPathsSelected && compareModel.structureSynced && !compareModel.loaded

                        background: Rectangle {
                            radius: 6
                            color: parent.enabled
                                ? (parent.hovered ? "#2563EB" : "#1E40AF")
                                : "#CBD5E0"
                        }
                        contentItem: Text {
                            text: parent.text; font: parent.font
                            color: parent.enabled ? "#FFFFFF" : "#718096"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            selectedTable = ""
                            showOnlyDiff = true
                            compareModel.syncData()
                        }
                    }
                }
            }

            // ══════════════════════════════════════════════
            // Status Bar
            // ══════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                height: compareModel.statusMessage !== "" ? 32 : 0
                visible: compareModel.statusMessage !== ""
                color: {
                    if (compareModel.statusMessage.includes("❌")) return "#FEF2F2"
                    if (compareModel.statusMessage.includes("✅")) return "#F0FDF4"
                    return "#FFFBEB"
                }
                border.color: {
                    if (compareModel.statusMessage.includes("❌")) return "#FECACA"
                    if (compareModel.statusMessage.includes("✅")) return "#BBF7D0"
                    return "#FDE68A"
                }
                border.width: 1

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    text: compareModel.statusMessage
                    font.pixelSize: 11
                    color: "#4A5568"
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            // ══════════════════════════════════════════════
            // Structure Logs Card
            // ══════════════════════════════════════════════
            Rectangle {
                id: structLogsCard
                Layout.fillWidth: true
                height: compareModel.structureLogs.length > 0 && !compareModel.loaded ? 220 : 0
                visible: height > 0
                color: "#FFFFFF"
                border.color: "#BEE3F8"
                border.width: 1

                Behavior on height { NumberAnimation { duration: 200 } }

                ColumnLayout {
                    anchors { fill: parent; margins: 16 }
                    spacing: 8

                    Text {
                        text: "🔍 Log Structure Compare"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#2A4365"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#BEE3F8"
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            model: compareModel.structureLogs
                            delegate: Text {
                                text: modelData
                                font.pixelSize: 11
                                font.family: "Consolas"
                                color: {
                                    if (modelData.includes("❌") || modelData.includes("Gagal")) return "#E53E3E"
                                    if (modelData.includes("🆕") || modelData.includes("✅")) return "#2F855A"
                                    if (modelData.includes("📎") || modelData.includes("⚡") || modelData.includes("👁")) return "#2B6CB0"
                                    if (modelData.includes("📊")) return "#6B46C1"
                                    if (modelData.includes("ℹ️")) return "#D97706"
                                    return "#4A5568"
                                }
                                wrapMode: Text.Wrap
                                width: ListView.view.width
                            }
                        }
                    }
                }
            }

            // ══════════════════════════════════════════════
            // Loading Indicator
            // ══════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                height: 50
                visible: compareModel.comparing
                color: "#EFF6FF"

                Row {
                    anchors.centerIn: parent
                    spacing: 10

                    BusyIndicator { running: compareModel.comparing; implicitWidth: 24; implicitHeight: 24 }
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
                visible: compareModel.loaded

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
                                    width: diffBtnText.implicitWidth + 8
                                    height: 22
                                    radius: 11
                                    color: showOnlyDiff ? "#FEF9C3" : "#FFFFFF"
                                    border.color: showOnlyDiff ? "#F59E0B" : "#CBD5E0"
                                    border.width: 1

                                    Text {
                                        id: diffBtnText
                                        anchors.centerIn: parent
                                        text: "Δ " + compareModel.diffTableList.length
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
                                    width: allBtnText.implicitWidth + 8
                                    height: 22
                                    radius: 11
                                    color: !showOnlyDiff ? "#EFF6FF" : "#FFFFFF"
                                    border.color: !showOnlyDiff ? "#3B82F6" : "#CBD5E0"
                                    border.width: 1

                                    Text {
                                        id: allBtnText
                                        anchors.centerIn: parent
                                        text: "All " + compareModel.allTableList.length
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
                            model: showOnlyDiff ? compareModel.diffTableList : compareModel.allTableList
                            currentIndex: -1

                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: Rectangle {
                                id: tableItem
                                width: tableListView.width
                                height: 42
                                color: {
                                    if (selectedTable === modelData) return "#DBEAFE"
                                    return tableMouse.containsMouse ? "#E2E8F0" : "transparent"
                                }
                                border.color: selectedTable === modelData ? "#3B82F6" : "transparent"
                                border.width: selectedTable === modelData ? 2 : 0

                                property bool isDiff: compareModel.diffTableList.indexOf(modelData) >= 0

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
                                    }

                                    Text {
                                        text: modelData
                                        font.pixelSize: 12
                                        font.bold: selectedTable === modelData
                                        color: selectedTable === modelData ? "#1E40AF" : "#334155"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    ColumnLayout {
                                        spacing: 2
                                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                                        Rectangle {
                                            width: newCntText.implicitWidth + 8
                                            height: 14
                                            radius: 4
                                            color: "#DCFCE7"
                                            Layout.alignment: Qt.AlignRight
                                            Text {
                                                id: newCntText
                                                anchors.centerIn: parent
                                                text: "New: " + compareModel.getNewRowCount(modelData) + " Rows"
                                                font.pixelSize: 9
                                                color: "#065F46"
                                            }
                                        }

                                        Rectangle {
                                            width: oldCntText.implicitWidth + 8
                                            height: 14
                                            radius: 4
                                            color: "#FEE2E2"
                                            Layout.alignment: Qt.AlignRight
                                            Text {
                                                id: oldCntText
                                                anchors.centerIn: parent
                                                text: "Target: " + compareModel.getOldRowCount(modelData) + " Rows"
                                                font.pixelSize: 9
                                                color: "#991B1B"
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    id: tableMouse
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
                                    text: "PK: " + (compareModel.loaded ? compareModel.getPrimaryKey(selectedTable) : "")
                                    font.pixelSize: 11
                                    color: "#64748B"
                                    font.family: "Consolas"
                                }

                                Item { Layout.fillWidth: true }

                                Row {
                                    spacing: 8

                                    Rectangle {
                                        visible: (currentStats.newOnly || 0) > 0
                                        width: noText.implicitWidth + 14; height: 22; radius: 11
                                        color: "#DCFCE7"; border.color: "#86EFAC"
                                        Text { id: noText; anchors.centerIn: parent; text: "+" + (currentStats.newOnly || 0) + " baru"; font.pixelSize: 10; font.bold: true; color: "#166534" }
                                    }
                                    Rectangle {
                                        visible: (currentStats.diffCount || 0) > 0
                                        width: dcText.implicitWidth + 14; height: 22; radius: 11
                                        color: "#FEF9C3"; border.color: "#FDE68A"
                                        Text { id: dcText; anchors.centerIn: parent; text: (currentStats.diffCount || 0) + " beda"; font.pixelSize: 10; font.bold: true; color: "#854D0E" }
                                    }
                                    Rectangle {
                                        visible: (currentStats.oldOnly || 0) > 0
                                        width: ooText.implicitWidth + 14; height: 22; radius: 11
                                        color: "#FEE2E2"; border.color: "#FECACA"
                                        Text { id: ooText; anchors.centerIn: parent; text: (currentStats.oldOnly || 0) + " old only"; font.pixelSize: 10; font.bold: true; color: "#991B1B" }
                                    }
                                    Rectangle {
                                        visible: (currentStats.matchCount || 0) > 0
                                        width: mcText.implicitWidth + 14; height: 22; radius: 11
                                        color: "#F1F5F9"; border.color: "#CBD5E0"
                                        Text { id: mcText; anchors.centerIn: parent; text: "✓ " + (currentStats.matchCount || 0); font.pixelSize: 10; color: "#64748B" }
                                    }
                                }
                            }
                        }

                        // Two panels side by side
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 2

                            // Left: New DB (readonly)
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 0

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 32
                                    color: "#F8FAFC"
                                    border.color: "#E2E8F0"
                                    border.width: 1
                                    visible: ((currentStats.newOnly || 0) > 0) || ((currentStats.diffCount || 0) > 0)

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 8

                                        Text {
                                            text: "Tindakan Massal:"
                                            font.pixelSize: 11
                                            color: "#64748B"
                                        }

                                        Button {
                                            visible: (currentStats.newOnly || 0) > 0
                                            text: "Add All (" + currentStats.newOnly + ")"
                                            font.pixelSize: 11
                                            font.bold: true
                                            implicitHeight: 30
                                            background: Rectangle {
                                                radius: 4
                                                color: parent.hovered ? "#059669" : "#10B981"
                                            }
                                            contentItem: Text {
                                                text: parent.text; font: parent.font; color: "#FFFFFF"
                                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: compareModel.addAllNewRows(selectedTable)
                                        }

                                        Button {
                                            visible: (currentStats.diffCount || 0) > 0
                                            text: "Replace All (" + currentStats.diffCount + ")"
                                            font.pixelSize: 11
                                            font.bold: true
                                            implicitHeight: 30
                                            background: Rectangle {
                                                radius: 4
                                                color: parent.hovered ? "#D97706" : "#F59E0B"
                                            }
                                            contentItem: Text {
                                                text: parent.text; font: parent.font; color: "#FFFFFF"
                                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: compareModel.replaceAllDiffRows(selectedTable)
                                        }

                                        Item { Layout.fillWidth: true }
                                    }
                                }

                                PatchDbTableView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    mode: "readonly"
                                    tableName: selectedTable
                                    patchModel: compareModel
                                    primaryKey: compareModel.loaded ? compareModel.getPrimaryKey(selectedTable) : ""
                                    columns: currentColumns
                                    rows: currentNewRows

                                    onDataRefreshNeeded: root.loadTableData()
                                }
                            }

                            // Divider
                            Rectangle {
                                Layout.fillHeight: true
                                width: 2
                                color: "#CBD5E0"
                            }

                            // Right: Target DB (editable)
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 0

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 32
                                    color: "#F8FAFC"
                                    border.color: "#E2E8F0"
                                    border.width: 1
                                    visible: (currentStats.oldOnly || 0) > 0

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 8

                                        Text {
                                            text: "Tindakan Massal:"
                                            font.pixelSize: 11
                                            color: "#64748B"
                                        }

                                        Button {
                                            visible: (currentStats.oldOnly || 0) > 0
                                            text: "Delete All Old Only (" + currentStats.oldOnly + ")"
                                            font.pixelSize: 11
                                            font.bold: true
                                            implicitHeight: 30
                                            background: Rectangle {
                                                radius: 4
                                                color: parent.hovered ? "#DC2626" : "#EF4444"
                                            }
                                            contentItem: Text {
                                                text: parent.text; font: parent.font; color: "#FFFFFF"
                                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: compareModel.deleteAllOldOnlyRows(selectedTable)
                                        }

                                        Item { Layout.fillWidth: true }
                                    }
                                }

                                PatchDbTableView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    mode: "editable"
                                    tableName: selectedTable
                                    patchModel: compareModel
                                    primaryKey: compareModel.loaded ? compareModel.getPrimaryKey(selectedTable) : ""
                                    columns: currentColumns
                                    rows: currentOldRows

                                    onDataRefreshNeeded: root.loadTableData()
                                }
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
                visible: !compareModel.loaded && !compareModel.comparing && compareModel.structureLogs.length === 0

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 16

                    Text {
                        text: "🔀"
                        font.pixelSize: 64
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Compare and Patch DB"
                        font.pixelSize: 22
                        font.bold: true
                        color: "#1E293B"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Pilih file New DB dan Target Patch DB untuk membandingkan\ndan menyinkronkan struktur serta data database."
                        font.pixelSize: 13
                        color: "#64748B"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        wrapMode: Text.Wrap
                        Layout.maximumWidth: 460
                    }

                    Text {
                        text: "Langkah:\n1. Browse New DB dan Target DB\n2. Preview Structure → Apply Sync Structure\n3. Sync DB Data → Patch data secara selektif"
                        font.pixelSize: 12
                        color: "#94A3B8"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        wrapMode: Text.Wrap
                        Layout.maximumWidth: 400
                    }
                }
            }
        

            // ══════════════════════════════════════════════
            // Layout Spacer (mencegah panel menyebar saat loading/log tampil)
            // ══════════════════════════════════════════════
            Item {
                Layout.fillHeight: true
                visible: !compareModel.loaded && (compareModel.comparing || compareModel.structureLogs.length > 0)
            }
    }
}
