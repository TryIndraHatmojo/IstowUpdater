import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import IstowUpdater

Item {
    id: root

    BackupSessionModel {
        id: backupModel
    }

    // Modal konfirmasi rollback
    Dialog {
        id: confirmDialog
        title: "Konfirmasi Rollback"
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        
        property int pendingSessionId: 0
        property string targetDirUrl: ""

        ColumnLayout {
            spacing: 10
            Text {
                text: "Apakah Anda yakin ingin melakukan rollback?"
                font.bold: true
            }
            Text {
                text: "PERINGATAN: Semua file di direktori iStow yang dipilih akan dihapus!\nPastikan Anda sudah memilih folder yang benar."
                color: "red"
                wrapMode: Text.Wrap
                Layout.preferredWidth: 300
            }
        }

        onAccepted: {
            const ok = backupModel.rollbackAppSession(pendingSessionId, targetDirUrl)
            if (ok) {
                rollbackResult.text = "Rollback aplikasi berhasil di direktori " + targetDirUrl
            } else {
                rollbackResult.text = "Rollback aplikasi gagal. " + backupModel.statusMessage
            }
            rollbackDialog.open()
        }
    }

    // Dialog pemberitahuan hasil rollback
    Dialog {
        id: rollbackDialog
        title: "Hasil Rollback"
        modal: true
        standardButtons: Dialog.Ok
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)

        Text {
            id: rollbackResult
            text: ""
            wrapMode: Text.Wrap
            width: 300
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Pilih Folder iStow"
        currentFolder: "file:///C:/"
        
        property int pendingSessionId: 0
        property string pendingFolderName: ""

        onAccepted: {
            confirmDialog.pendingSessionId = pendingSessionId
            confirmDialog.targetDirUrl = selectedFolder.toString()
            confirmDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            height: 56
            radius: 10
            color: "#1A3A5C"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Text {
                    text: "Rollback Application"
                    color: "#FFFFFF"
                    font.pixelSize: 18
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    tone: "secondary"
                    text: "Refresh"
                    enabled: !backupModel.rollingBack
                    onClicked: backupModel.reload()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: statusText.implicitHeight + 20
            radius: 8
            color: "#FFFBEB"
            border.color: "#F6E05E"
            border.width: 1
            visible: backupModel.statusMessage !== ""

            Text {
                id: statusText
                anchors.fill: parent
                anchors.margins: 10
                text: backupModel.statusMessage
                font.pixelSize: 12
                color: "#744210"
                wrapMode: Text.Wrap
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.6
                    Layout.fillHeight: true
                    spacing: 8

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: sessionList
                            width: parent.width
                            model: backupModel.appSessions
                            spacing: 6
                            headerPositioning: ListView.OverlayHeader

                            header: Rectangle {
                                width: sessionList.width
                                height: 42
                                radius: 6
                                color: "#EEF2F7"
                                border.color: "#D9E2EC"
                                border.width: 1
                                z: 2

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 8

                                    Text { text: "App Name"; font.bold: true; Layout.preferredWidth: 100; color: "#2D3748" }
                                    Text { text: "Version"; font.bold: true; Layout.preferredWidth: 80; color: "#2D3748" }
                                    Text { text: "Folder Name"; font.bold: true; Layout.fillWidth: true; color: "#2D3748" }
                                    Text { text: "Created At"; font.bold: true; Layout.preferredWidth: 140; color: "#2D3748" }
                                    Text { text: "Aksi"; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.preferredWidth: 100; color: "#2D3748" }
                                }
                            }

                            delegate: Rectangle {
                                required property var modelData

                                width: sessionList.width
                                height: 54
                                radius: 6
                                color: index % 2 === 0 ? "#FFFFFF" : "#F8FAFC"
                                border.color: "#E2E8F0"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 8

                                    Text {
                                        text: modelData.app_name ?? "-"
                                        Layout.preferredWidth: 100
                                        color: "#2D3748"
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        text: modelData.version ?? "-"
                                        Layout.preferredWidth: 80
                                        color: "#2D3748"
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        text: modelData.folder_name ?? "-"
                                        Layout.fillWidth: true
                                        color: "#2D3748"
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        text: modelData.created_at_text ?? "-"
                                        Layout.preferredWidth: 140
                                        color: "#2D3748"
                                        elide: Text.ElideRight
                                    }

                                    AppButton {
                                        text: backupModel.rollingBack ? "Proses..." : "Rollback"
                                        Layout.preferredWidth: 100
                                        enabled: !backupModel.rollingBack
                                        onClicked: {
                                            folderDialog.pendingSessionId = Number(modelData.id ?? 0)
                                            folderDialog.pendingFolderName = String(modelData.folder_name ?? "")
                                            folderDialog.open()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: backupModel.appSessions.length === 0 ? "Belum ada data backup application sessions." : ""
                        color: "#718096"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        visible: backupModel.appSessions.length === 0
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: parent.width * 0.4
                    radius: 8
                    color: "#F7FAFC"
                    border.color: "#D9E2EC"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Log Proses"
                                font.bold: true
                                color: "#4A5568"
                            }
                            Item { Layout.fillWidth: true }
                            AppButton {
                                text: "Bersihkan"
                                onClicked: backupModel.clearLogs()
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#FFFFFF"
                            border.color: "#E2E8F0"
                            border.width: 1
                            radius: 4

                            ScrollView {
                                anchors.fill: parent
                                anchors.margins: 6
                                clip: true

                                ListView {
                                    model: backupModel.rollbackLogs
                                    delegate: Text {
                                        text: modelData
                                        font.pixelSize: 12
                                        font.family: "Courier"
                                        color: "#4A5568"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
