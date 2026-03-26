import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: root
    modal: true
    focus: true
    padding: 30
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int selectedId: -1
    property string selectedSourceFile: ""
    property bool hasExistingInstallerFile: false
    property string existingInstallerPath: ""

    signal saveRequested(int selectedId,
                         string version,
                         string sourceFileUrl)

    function resetForm() {
        selectedId = -1
        selectedSourceFile = ""
        hasExistingInstallerFile = false
        existingInstallerPath = ""
        versionField.text = ""
        selectedFileLabel.text = "Belum ada file .exe dipilih"
        validationLabel.text = ""
    }

    function openForCreate() {
        resetForm()
        open()
    }

    function openForEdit(item) {
        selectedId = item.id
        selectedSourceFile = ""
        hasExistingInstallerFile = item.installer_path !== undefined &&
                                   item.installer_path !== null &&
                                   item.installer_path.toString().trim() !== ""
        existingInstallerPath = hasExistingInstallerFile ? item.installer_path : ""
        versionField.text = item.version
        selectedFileLabel.text = hasExistingInstallerFile
            ? item.installer_path
            : "Belum ada file .exe dipilih"
        validationLabel.text = ""
        open()
    }

    function closeAndReset() {
        close()
        resetForm()
    }

    width: Math.min((parent ? parent.width : 900) * 0.92, 700)
    x: (parent ? (parent.width - width) / 2 : 0)
    y: (parent ? (parent.height - implicitHeight) / 2 : 0)

    background: Rectangle {
        radius: 12
        color: "#FFFFFF"
        border.color: "#D1D5DB"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Text {
            text: root.selectedId > 0 ? "Edit Installer" : "Tambah Installer"
            font.pixelSize: 18
            font.bold: true
            color: "#1F2937"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: "Versi"
                Layout.preferredWidth: 90
            }

            TextField {
                id: versionField
                Layout.fillWidth: true
                placeholderText: "Contoh: 2.5.2.16-2.4.1.1"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: "File .exe"
                Layout.preferredWidth: 90
            }

            AppButton {
                text: "Browse"
                tone: "secondary"
                onClicked: exeDialog.open()
            }

            Text {
                id: selectedFileLabel
                Layout.fillWidth: true
                text: "Belum ada file .exe dipilih"
                color: "#4B5563"
                elide: Text.ElideMiddle
            }
        }

        Label {
            id: validationLabel
            Layout.fillWidth: true
            color: "#DC2626"
            wrapMode: Text.Wrap
            visible: text.length > 0
        }

        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
            }

            AppButton {
                tone: "secondary"
                text: "Batal"
                onClicked: root.closeAndReset()
            }

            AppButton {
                text: root.selectedId > 0 ? "Update" : "Tambah"
                onClicked: {
                    const cleanVersion = versionField.text.trim()
                    const needsFile = root.selectedId <= 0 || !root.hasExistingInstallerFile

                    if (cleanVersion.length === 0) {
                        validationLabel.text = "Versi wajib diisi."
                        return
                    }

                    if (needsFile && root.selectedSourceFile.trim().length === 0) {
                        validationLabel.text = "File installer .exe wajib dipilih."
                        return
                    }

                    validationLabel.text = ""
                    root.saveRequested(
                        root.selectedId,
                        versionField.text,
                        root.selectedSourceFile)
                }
            }
        }
    }

    FileDialog {
        id: exeDialog
        title: "Pilih installer (.exe)"
        nameFilters: ["Installer executable (*.exe)"]
        fileMode: FileDialog.OpenFile

        onAccepted: {
            root.selectedSourceFile = selectedFile.toString()
            selectedFileLabel.text = selectedFile.toString().replace("file:///", "")
        }
    }
}
