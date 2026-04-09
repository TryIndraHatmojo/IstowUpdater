import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import IstowUpdater

Item {
    id: root

    // ── Signal untuk navigasi kembali ──────────────────
    signal backRequested()

    // ── Referensi model ────────────────────────────────
    property var shipModel: null
    property bool showBackButton: true

    // ── IstowImporter instance ─────────────────────────
    IstowImporter {
        id: importer

        onImportFinished: function(success, message) {
            if (success && shipModel) {
                shipModel.reload()
            }
            resultPopup.show(success, message)
        }
    }

    LogModel {
        id: logModelExporter
    }

    FileDialog {
        id: logExportDialog
        title: "Export Logs to TXT"
        nameFilters: ["Text files (*.txt)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        property var logsToExport: []
        onAccepted: {
            if (logModelExporter.exportStringListTxt(logsToExport, selectedFile)) {
                console.log("✅ Berhasil ekspor local log.")
            } else {
                console.log("❌ Gagal ekspor local log.")
            }
        }
    }

    // ── FileDialog ─────────────────────────────────────
    FileDialog {
        id: fileDialog
        title: "Pilih file .istow"
        nameFilters: ["iStow Files (*.istow)"]
        onAccepted: {
            let path = selectedFile.toString()
            selectedFilePath.text = path
            console.log("[BrowseDotIstow] File dipilih:", path)

            // Reset state
            detailsCard.visible = false
            fallbackFormCard.visible = false
            importBtn.enabled = false

            // Baca metadata otomatis
            if (importer.readDetails(path)) {
                detailsCard.visible = true
                importBtn.enabled = true
            } else {
                fallbackFormCard.visible = true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Header ─────────────────────────────────────
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

                Button {
                    text: "← Kembali"
                    flat: true
                    visible: root.showBackButton
                    enabled: root.showBackButton
                    font.pixelSize: 14
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#FFFFFF"
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#2B6CB0" : "transparent"
                        radius: 6
                    }
                    onClicked: root.backRequested()
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Import File .istow"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#FFFFFF"
                }

                Item { Layout.fillWidth: true }
                // Spacer untuk balance
                Item { width: 90 }
            }
        }

        // ── Konten Utama ───────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent
                contentWidth: availableWidth

                ColumnLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width - 48, 520)
                    spacing: 16

                    Item { height: 8 } // Top spacing

                    // ── Card: File Picker ───────────────
                    Rectangle {
                        Layout.fillWidth: true
                        height: filePickerContent.implicitHeight + 48
                        radius: 12
                        color: "#FFFFFF"
                        border.color: "#E2E8F0"
                        border.width: 1

                        ColumnLayout {
                            id: filePickerContent
                            anchors { fill: parent; margins: 24 }
                            spacing: 16

                            Text {
                                text: "📁 Pilih File .istow"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#1A3A5C"
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: "#E2E8F0"
                            }

                            Text {
                                text: "Pilih file .istow yang berisi data kapal untuk di-import. Asset akan diekstrak ke folder kerja dan struktur database akan diupdate."
                                font.pixelSize: 12
                                color: "#718096"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Button {
                                    text: "Browse..."
                                    font.pixelSize: 14
                                    font.bold: true
                                    implicitHeight: 40

                                    background: Rectangle {
                                        radius: 8
                                        color: parent.hovered ? "#2B6CB0" : "#1A3A5C"
                                    }

                                    contentItem: Text {
                                        text: parent.text
                                        font: parent.font
                                        color: "#FFFFFF"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    onClicked: fileDialog.open()
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 40
                                    radius: 8
                                    color: "#F7FAFC"
                                    border.color: "#CBD5E0"
                                    border.width: 1

                                    Text {
                                        id: selectedFilePath
                                        anchors {
                                            fill: parent
                                            leftMargin: 12
                                            rightMargin: 12
                                        }
                                        text: "Belum ada file dipilih"
                                        font.pixelSize: 12
                                        color: text === "Belum ada file dipilih" ? "#A0AEC0" : "#2D3748"
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideMiddle
                                    }
                                }
                            }
                        }
                    }

                    // ── Card: Metadata ──────────────────
                    Rectangle {
                        id: detailsCard
                        Layout.fillWidth: true
                        height: detailsContent.implicitHeight + 48
                        radius: 12
                        color: "#FFFFFF"
                        border.color: "#E2E8F0"
                        border.width: 1
                        visible: false

                        ColumnLayout {
                            id: detailsContent
                            anchors { fill: parent; margins: 24 }
                            spacing: 12

                            Text {
                                text: "📋 Metadata Kapal"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#1A3A5C"
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: "#E2E8F0"
                            }

                            // Info grid
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 16
                                rowSpacing: 8

                                Text { text: "Nama Kapal:"; font.pixelSize: 12; color: "#718096"; font.bold: true }
                                Text { text: importer.shipDetails["nama"] ?? "-"; font.pixelSize: 12; color: "#2D3748"; Layout.fillWidth: true; elide: Text.ElideRight }

                                Text { text: "Tipe:"; font.pixelSize: 12; color: "#718096"; font.bold: true }
                                Text { text: importer.shipDetails["tipe"] ?? "-"; font.pixelSize: 12; color: "#2D3748" }

                                Text { text: "Perusahaan:"; font.pixelSize: 12; color: "#718096"; font.bold: true }
                                Text { text: importer.shipDetails["company"] ?? "-"; font.pixelSize: 12; color: "#2D3748" }

                                Text { text: "ID Kapal:"; font.pixelSize: 12; color: "#718096"; font.bold: true }
                                Text { text: String(importer.shipDetails["idship"] ?? "-"); font.pixelSize: 12; color: "#2D3748" }

                                Text { text: "DB File:"; font.pixelSize: 12; color: "#718096"; font.bold: true }
                                Text { text: importer.shipDetails["dbid"] ?? "-"; font.pixelSize: 12; color: "#2D3748" }

                                Text { text: "Prefix:"; font.pixelSize: 12; color: "#718096"; font.bold: true }
                                Text { text: importer.shipDetails["fileprefix"] ?? "-"; font.pixelSize: 12; color: "#2D3748" }

                                Text { text: "Versi:"; font.pixelSize: 12; color: "#718096"; font.bold: true }
                                Text { text: String(importer.shipDetails["version"] ?? "-"); font.pixelSize: 12; color: "#2D3748" }
                            }
                        }
                    }

                    // ── Card: Fallback Form ──────────────
                    Rectangle {
                        id: fallbackFormCard
                        Layout.fillWidth: true
                        height: fallbackFormContent.implicitHeight + 48
                        radius: 12
                        color: "#FFFFFF"
                        border.color: "#F6AD55"
                        border.width: 1
                        visible: false

                        ColumnLayout {
                            id: fallbackFormContent
                            anchors { fill: parent; margins: 24 }
                            spacing: 12

                            Text {
                                text: "⚠️ Metadata Tidak Ditemukan"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#C05621"
                            }

                            Text {
                                text: "File .istow yang dipilih tidak memiliki file .details. Silakan isi form berikut untuk melanjutkan."
                                font.pixelSize: 12
                                color: "#718096"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: "#E2E8F0"
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 16
                                rowSpacing: 8

                                Text { text: "Nama Kapal:"; font.pixelSize: 12; color: "#4A5568" }
                                TextField { id: fNama; Layout.fillWidth: true; placeholderText: "Misal: MV. Sally Fortune" }

                                Text { text: "Tipe:"; font.pixelSize: 12; color: "#4A5568" }
                                TextField { id: fTipe; Layout.fillWidth: true; placeholderText: "Misal: Container" }

                                Text { text: "Perusahaan:"; font.pixelSize: 12; color: "#4A5568" }
                                TextField { id: fCompany; Layout.fillWidth: true; placeholderText: "Misal: PT. Indonesian Fortune Lloyd" }

                                Text { text: "ID Kapal:"; font.pixelSize: 12; color: "#4A5568" }
                                TextField { id: fIdship; Layout.fillWidth: true; placeholderText: "Misal: 55"; validator: IntValidator {} }

                                Text { text: "DB File:"; font.pixelSize: 12; color: "#4A5568" }
                                TextField { id: fDbid; Layout.fillWidth: true; placeholderText: "Misal: 55_MVSallyFortune.db" }

                                Text { text: "Prefix:"; font.pixelSize: 12; color: "#4A5568" }
                                TextField { id: fFileprefix; Layout.fillWidth: true; placeholderText: "Misal: 55_MVSallyFortune_" }

                                Text { text: "Versi:"; font.pixelSize: 12; color: "#4A5568" }
                                TextField { id: fVersion; Layout.fillWidth: true; placeholderText: "Misal: 0"; validator: IntValidator {} }
                            }

                            Button {
                                text: "Simpan & Lanjutkan"
                                Layout.fillWidth: true
                                height: 40
                                font.pixelSize: 14
                                font.bold: true

                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? "#DD6B20" : "#C05621"
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: "#FFFFFF"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                onClicked: {
                                    let details = {
                                        nama: fNama.text,
                                        tipe: fTipe.text,
                                        company: fCompany.text,
                                        idship: parseInt(fIdship.text) || 0,
                                        dbid: fDbid.text,
                                        fileprefix: fFileprefix.text,
                                        version: parseInt(fVersion.text) || 0
                                    }
                                    if (importer.generateDetailsAndLoad(details)) {
                                        fallbackFormCard.visible = false
                                        detailsCard.visible = true
                                        importBtn.enabled = true
                                    }
                                }
                            }
                        }
                    }

                    // ── Status Message ──────────────────
                    Rectangle {
                        Layout.fillWidth: true
                        height: statusText.implicitHeight + 20
                        radius: 8
                        color: "#FFFBEB"
                        border.color: "#F6E05E"
                        border.width: 1
                        visible: importer.statusMessage !== ""

                        Text {
                            id: statusText
                            anchors {
                                fill: parent
                                margins: 10
                            }
                            text: importer.statusMessage
                            font.pixelSize: 12
                            color: "#744210"
                            wrapMode: Text.Wrap
                        }
                    }

                    // ── Card: Import Logs ────────────────
                    Rectangle {
                        id: logsCard
                        Layout.fillWidth: true
                        height: 200
                        radius: 12
                        color: "#FFFFFF"
                        border.color: "#E2E8F0"
                        border.width: 1
                        visible: importer.importLogs.length > 0

                        ColumnLayout {
                            anchors { fill: parent; margins: 16 }
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                
                                Text {
                                    text: "📝 Log Import File"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#1A3A5C"
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                Button {
                                    text: "Export Log"
                                    height: 24
                                    font.pixelSize: 11
                                    onClicked: {
                                        logExportDialog.logsToExport = importer.importLogs
                                        logExportDialog.open()
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: "#E2E8F0"
                            }

                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                ListView {
                                    model: importer.importLogs
                                    delegate: Text {
                                        text: modelData
                                        font.pixelSize: 12
                                        color: modelData.includes("REPLACE") ? "#B7791F" : (modelData.includes("LEWATI") ? "#E53E3E" : "#2F855A")
                                        wrapMode: Text.Wrap
                                        width: ListView.view.width
                                    }
                                }
                            }
                        }
                    }

                    // ── Card: DB Compare Logs ────────────
                    Rectangle {
                        id: dbLogsCard
                        Layout.fillWidth: true
                        height: 220
                        radius: 12
                        color: "#FFFFFF"
                        border.color: "#BEE3F8"
                        border.width: 1
                        visible: importer.dbCompareLogs.length > 0

                        ColumnLayout {
                            anchors { fill: parent; margins: 16 }
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                
                                Text {
                                    text: "🔍 Log Database Compare"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#2A4365"
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                Button {
                                    text: "Export Log"
                                    height: 24
                                    font.pixelSize: 11
                                    onClicked: {
                                        logExportDialog.logsToExport = importer.dbCompareLogs
                                        logExportDialog.open()
                                    }
                                }
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
                                    model: importer.dbCompareLogs
                                    delegate: Text {
                                        text: modelData
                                        font.pixelSize: 11
                                        font.family: "Consolas"
                                        color: {
                                            if (modelData.includes("❌") || modelData.includes("Gagal")) return "#E53E3E"
                                            if (modelData.includes("🆕") || modelData.includes("✅")) return "#2F855A"
                                            if (modelData.includes("📎") || modelData.includes("⚡") || modelData.includes("👁")) return "#2B6CB0"
                                            if (modelData.includes("📊")) return "#6B46C1"
                                            return "#4A5568"
                                        }
                                        wrapMode: Text.Wrap
                                        width: ListView.view.width
                                    }
                                }
                            }
                        }
                    }

                    // ── Action Buttons ──────────────────
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Button {
                            id: importBtn
                            Layout.fillWidth: true
                            height: 44
                            text: importer.importing ? "Mengimport..." : "⬇ Import Assets & Sync DB Structure"
                            enabled: false
                            font.pixelSize: 14
                            font.bold: true

                            background: Rectangle {
                                radius: 8
                                color: parent.enabled
                                       ? (parent.hovered ? "#2F855A" : "#276749")
                                       : "#CBD5E0"
                            }

                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.enabled ? "#FFFFFF" : "#718096"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                if (importer.importing) return

                                let path = fileDialog.selectedFile.toString()
                                console.log("[BrowseDotIstow] Mulai import:", path)

                                importer.clearLogs()
                                importer.clearDbLogs()

                                // 1. Import assets (skip .db)
                                let ok = importer.importAssets(path)
                                if (!ok) return

                                // 2. Ekstrak DB ke temporary
                                let tempDb = importer.extractDbToTemp(path)
                                if (tempDb !== "") {
                                    console.log("[BrowseDotIstow] DB temp:", tempDb)

                                    // 3. Compare & Migrate DB (additive only)
                                    importer.compareAndMigrateDb(tempDb)
                                }

                                // 4. Register metadata ke LocalDB
                                importer.registerToLocalDb()
                            }
                        }
                    }

                    Item { height: 16 } // Bottom spacing
                }
            }
        }
    }

    // ── Result Popup ───────────────────────────────────
    Popup {
        id: resultPopup
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 400)
        height: popupContent.implicitHeight + 48
        modal: true
        dim: true

        property bool isSuccess: false
        property string resultMessage: ""

        function show(success, message) {
            isSuccess = success
            resultMessage = message
            open()
        }

        background: Rectangle {
            radius: 12
            color: "#FFFFFF"
            border.color: resultPopup.isSuccess ? "#48BB78" : "#FC8181"
            border.width: 2
        }

        ColumnLayout {
            id: popupContent
            anchors { fill: parent; margins: 24 }
            spacing: 16

            Text {
                text: resultPopup.isSuccess ? "✅ Import Berhasil" : "❌ Import Gagal"
                font.pixelSize: 18
                font.bold: true
                color: resultPopup.isSuccess ? "#276749" : "#9B2C2C"
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: resultPopup.resultMessage
                font.pixelSize: 13
                color: "#4A5568"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                Layout.fillWidth: true
                height: 40
                text: "OK"
                font.pixelSize: 14
                font.bold: true

                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? "#2B6CB0" : "#1A3A5C"
                }

                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: resultPopup.close()
            }
        }
    }
}
