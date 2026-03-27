import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*!
 * PatchDbTableView — Komponen tabel scrollable (horizontal + vertikal)
 * Mendukung 2 mode: "readonly" (panel kiri) dan "editable" (panel kanan).
 *
 * Property utama:
 *   - columns: [{name, type, pk}]
 *   - rows: [{col1: val, col2: val, _pk, _status, _diffCols}]
 *   - mode: "readonly" | "editable"
 */
Rectangle {
    id: tableRoot

    property var columns: []
    property var rows: []
    property string mode: "readonly"   // "readonly" | "editable"
    property string tableName: ""
    property var patchModel: null
    property string primaryKey: ""

    // Pending changes tracking (editable mode)
    property var pendingChanges: ({})
    property int pendingCount: 0

    // Signals
    signal rowReplaced(var pkValue)
    signal rowAdded(var pkValue)
    signal changesSaved()
    signal dataRefreshNeeded()

    color: "transparent"
    clip: true

    readonly property int colMinWidth: 140
    readonly property int actionColWidth: mode === "readonly" ? 200 : 0
    readonly property int rowHeight: 38
    readonly property int headerHeight: 40
    readonly property real totalContentWidth: {
        var w = 0
        for (var i = 0; i < columns.length; i++) {
            w += Math.max(colMinWidth, _getColWidth(columns[i].name))
        }
        return w + actionColWidth + 4
    }

    function _getColWidth(colName) {
        if (!colName) return colMinWidth
        var len = colName.length
        return Math.max(colMinWidth, Math.min(len * 10 + 24, 280))
    }

    function _isDiffCol(row, colName) {
        if (!row || !row._diffCols) return false
        var dc = row._diffCols.toString()
        if (dc === "") return false
        var cols = dc.split(",")
        return cols.indexOf(colName) >= 0
    }

    function _getRowBgColor(row, rowIndex) {
        if (!row) return rowIndex % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
        if (row._status === "new_only") return "#F0FDF4"
        if (row._status === "old_only") return "#FEF2F2"
        return rowIndex % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
    }

    function _getCellBgColor(row, colName, rowIndex) {
        // Pending change highlight (editable mode)
        if (mode === "editable" && row) {
            var key = String(row._pk) + "::" + colName
            if (pendingChanges[key] !== undefined) return "#EFF6FF"
        }
        // Diff highlight
        if (_isDiffCol(row, colName)) return "#FEF9C3"
        return "transparent"
    }

    function _getCellBorderColor(row, colName) {
        if (mode === "editable" && row) {
            var key = String(row._pk) + "::" + colName
            if (pendingChanges[key] !== undefined) return "#3B82F6"
        }
        if (_isDiffCol(row, colName)) return "#F59E0B"
        return "#E2E8F0"
    }

    function markPending(pk, col, value) {
        var changes = Object.assign({}, pendingChanges)
        changes[String(pk) + "::" + col] = value
        pendingChanges = changes
        pendingCount = Object.keys(pendingChanges).length
        console.log("Pending changes updated, count:", pendingCount)
    }

    function clearPending() {
        pendingChanges = ({})
        pendingCount = 0
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Panel Title ─────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: mode === "readonly" ? "#1E40AF" : "#065F46"
            radius: 6

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Text {
                    text: mode === "readonly" ? "📦 DB Baru (.istow)" : "🗄️ DB Target (iStowV2)"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#FFFFFF"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: rows.length + " rows"
                    font.pixelSize: 11
                    color: "#CBD5E0"
                }
            }
        }

        // ── Save Button (editable mode) ─────────────────
        Rectangle {
            Layout.fillWidth: true
            height: (mode === "editable" && pendingCount > 0) ? 44 : 0
            Layout.preferredHeight: height
            visible: height > 0
            color: "#EFF6FF"
            border.color: "#3B82F6"
            border.width: 1
            radius: 6
            clip: true
            z: 10

            Behavior on height { NumberAnimation { duration: 200 } }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                visible: pendingCount > 0

                Text {
                    text: "⚡ " + pendingCount + " perubahan pending"
                    font.pixelSize: 12
                    color: "#1E40AF"
                    Layout.fillWidth: true
                }

                Button {
                    text: "💾 Simpan Semua"
                    font.pixelSize: 12
                    font.bold: true
                    implicitHeight: 30

                    background: Rectangle {
                        radius: 6
                        color: parent.hovered ? "#2563EB" : "#1E40AF"
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        var changeList = []
                        for (var key in pendingChanges) {
                            var parts = key.split("::")
                            changeList.push({
                                "pk": parts[0],
                                "column": parts.slice(1).join("::"),
                                "value": pendingChanges[key]
                            })
                        }
                        if (patchModel && patchModel.savePendingChanges(tableName, changeList)) {
                            clearPending()
                            changesSaved()
                            dataRefreshNeeded()
                        }
                    }
                }

                Button {
                    text: "✖"
                    font.pixelSize: 14
                    implicitWidth: 30
                    implicitHeight: 30
                    background: Rectangle {
                        radius: 6
                        color: parent.hovered ? "#FEE2E2" : "transparent"
                    }
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: "#EF4444"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: { clearPending(); dataRefreshNeeded() }
                }
            }
        }

        // ── Table Area (scrollable H+V) ─────────────────
        Flickable {
            id: flickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: totalContentWidth
            contentHeight: headerHeight + rows.length * rowHeight + 4
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.AutoFlickIfNeeded

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
                height: 10
            }
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 10
            }

            Column {
                id: tableContent
                spacing: 0

                // ── Header Row ──────────────────────────
                Row {
                    id: headerRow
                    spacing: 0

                    Repeater {
                        model: columns

                        Rectangle {
                            width: Math.max(colMinWidth, _getColWidth(modelData.name))
                            height: headerHeight
                            color: "#1A3A5C"
                            border.color: "#0F2A43"
                            border.width: 1

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 6
                                text: modelData.name + (modelData.pk ? " 🔑" : "")
                                font.pixelSize: 12
                                font.bold: true
                                color: "#FFFFFF"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // Action column header (readonly mode)
                    Rectangle {
                        visible: mode === "readonly"
                        width: actionColWidth
                        height: headerHeight
                        color: "#1A3A5C"
                        border.color: "#0F2A43"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "Aksi"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#FFFFFF"
                        }
                    }
                }

                // ── Data Rows ───────────────────────────
                Repeater {
                    id: rowRepeater
                    model: rows

                    Row {
                        id: dataRow
                        spacing: 0
                        property var rowData: modelData
                        property int rowIdx: index

                        // Status indicator (left border glow)
                        Rectangle {
                            width: 3
                            height: rowHeight
                            color: {
                                if (!rowData) return "transparent"
                                if (rowData._status === "new_only") return "#22C55E"
                                if (rowData._status === "old_only") return "#EF4444"
                                if (rowData._status === "diff") return "#F59E0B"
                                return "transparent"
                            }
                        }

                        // Data cells
                        Repeater {
                            model: columns

                            Rectangle {
                                id: cellRect
                                width: Math.max(colMinWidth, _getColWidth(modelData.name)) - (index === 0 ? 3 : 0)
                                height: rowHeight
                                color: {
                                    var base = _getRowBgColor(rowData, rowIdx)
                                    var cell = _getCellBgColor(rowData, modelData.name, rowIdx)
                                    return cell !== "transparent" ? cell : base
                                }
                                border.color: _getCellBorderColor(rowData, modelData.name)
                                border.width: 1

                                property string cellValue: {
                                    if (!rowData) return ""
                                    var v = rowData[modelData.name]
                                    return v !== undefined && v !== null ? String(v) : ""
                                }
                                property string colName: modelData.name

                                // Readonly mode: selectable + copy on click
                                TextInput {
                                    visible: mode === "readonly"
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 6
                                    text: cellRect.cellValue
                                    font.pixelSize: 12
                                    font.family: "Consolas"
                                    color: "#2D3748"
                                    readOnly: true
                                    selectByMouse: true
                                    verticalAlignment: Text.AlignVCenter
                                    clip: true
                                }

                                // Editable mode: text input
                                TextInput {
                                    id: editInput
                                    visible: mode === "editable"
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 6
                                    text: {
                                        // Show pending value if exists
                                        var key = String(rowData ? rowData._pk : "") + "::" + cellRect.colName
                                        if (pendingChanges[key] !== undefined) return pendingChanges[key]
                                        return cellRect.cellValue
                                    }
                                    font.pixelSize: 12
                                    font.family: "Consolas"
                                    color: "#2D3748"
                                    verticalAlignment: Text.AlignVCenter
                                    selectByMouse: true
                                    clip: true

                                    onEditingFinished: {
                                        if (!rowData) return
                                        var originalVal = cellRect.cellValue
                                        if (text !== originalVal) {
                                            markPending(rowData._pk, cellRect.colName, text)
                                        }
                                    }
                                }
                            }
                        }

                        // Action buttons column (readonly mode)
                        Rectangle {
                            visible: mode === "readonly"
                            width: actionColWidth
                            height: rowHeight
                            color: _getRowBgColor(rowData, rowIdx)
                            border.color: "#E2E8F0"
                            border.width: 1

                            Row {
                                anchors.centerIn: parent
                                spacing: 4

                                // Replace by ID
                                Button {
                                    text: "▶ Replace"
                                    font.pixelSize: 10
                                    font.bold: true
                                    implicitHeight: 26
                                    implicitWidth: 88

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? "#E67E22" : "#F59E0B"
                                    }
                                    contentItem: Text {
                                        text: parent.text; font: parent.font
                                        color: "#FFFFFF"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        if (patchModel && rowData) {
                                            patchModel.replaceRowById(tableName, rowData._pk)
                                            rowReplaced(rowData._pk)
                                            dataRefreshNeeded()
                                        }
                                    }
                                }

                                // Add Row (only for new_only rows)
                                Button {
                                    visible: rowData && rowData._status === "new_only"
                                    text: "+ Add"
                                    font.pixelSize: 10
                                    font.bold: true
                                    implicitHeight: 26
                                    implicitWidth: 70

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? "#059669" : "#10B981"
                                    }
                                    contentItem: Text {
                                        text: parent.text; font: parent.font
                                        color: "#FFFFFF"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        if (patchModel && rowData) {
                                            patchModel.addRowToOldDb(tableName, rowData._pk)
                                            rowAdded(rowData._pk)
                                            dataRefreshNeeded()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Empty state ─────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 60
            visible: rows.length === 0
            color: "#F8FAFC"
            radius: 6

            Text {
                anchors.centerIn: parent
                text: "Tidak ada data"
                font.pixelSize: 13
                color: "#94A3B8"
                font.italic: true
            }
        }
    }
}
