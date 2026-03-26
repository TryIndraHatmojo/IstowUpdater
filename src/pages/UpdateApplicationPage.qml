import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IstowUpdater

Item {
	id: root

	property string lastStatusMessageLogged: ""
	property int installerNameColumnWidth: 220
	property int installerVersionColumnWidth: 130
	property int installerActionColumnWidth: 190

	function timestampNow() {
		return new Date().toLocaleString()
	}

	function appendLog(message, level) {
		if (!message || message.trim().length === 0)
			return

		logModel.insert(0, {
			time: timestampNow(),
			text: message,
			level: level
		})
	}

	InstallerModel {
		id: installerModel
	}

	Connections {
		target: installerModel
		function onStatusMessageChanged() {
			if (installerModel.statusMessage && installerModel.statusMessage !== root.lastStatusMessageLogged) {
				root.lastStatusMessageLogged = installerModel.statusMessage
				root.appendLog(installerModel.statusMessage, "info")
			}
		}
	}

	ListModel {
		id: logModel
	}

	InstallerFormPopup {
		id: installerFormPopup
		parent: root

		onSaveRequested: function (id, version, sourceFileUrl) {
			let ok = false
			if (id > 0) {
				ok = installerModel.updateInstaller(id, version, sourceFileUrl)
				root.appendLog(ok ? "Update installer berhasil (ID " + id + ")." : "Update installer gagal (ID " + id + ").", ok ? "success" : "error")
			} else {
				ok = installerModel.createInstaller(version, sourceFileUrl)
				root.appendLog(ok ? "Tambah installer berhasil." : "Tambah installer gagal.", ok ? "success" : "error")
			}

			if (ok) {
				installerFormPopup.closeAndReset()
			}
		}
	}

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: 16
		spacing: 12

		Rectangle {
			Layout.fillWidth: true
			Layout.fillHeight: true
			radius: 10
			border.color: "#D1D5DB"
			color: "#FFFFFF"

			RowLayout {
				anchors.fill: parent
				anchors.margins: 14
				spacing: 10

				ColumnLayout {
					Layout.fillWidth: true
					Layout.fillHeight: true
					spacing: 10
					Layout.preferredWidth: parent.width * 0.5

					Rectangle {
						Layout.fillWidth: true
						height: 44
						radius: 8
						color: "#F8FAFC"
						border.color: "#E2E8F0"

						RowLayout {
							anchors.fill: parent
							anchors.leftMargin: 12
							anchors.rightMargin: 12

							Text {
								text: "Daftar Installer"
								font.pixelSize: 16
								font.bold: true
								color: "#1F2937"
							}

							Item {
								Layout.fillWidth: true
							}

							AppButton {
								text: "Tambah Installer"
								onClicked: installerFormPopup.openForCreate()
							}
						}
					}

					Rectangle {
						Layout.fillWidth: true
						Layout.fillHeight: true
						radius: 8
						color: "#FFFFFF"
						border.color: "#E5E7EB"

						ColumnLayout {
							anchors.fill: parent
							anchors.margins: 8
							spacing: 8

							ListView {
								id: installerList
								Layout.fillWidth: true
								Layout.fillHeight: true
								clip: true
								model: installerModel.installers
								spacing: 6
								headerPositioning: ListView.OverlayHeader

								header: Rectangle {
									width: installerList.width
									height: 40
									radius: 6
									color: "#EEF2F7"
									z: 2

									RowLayout {
										anchors.fill: parent
										anchors.leftMargin: 10
										anchors.rightMargin: 10
										spacing: 8

										Text {
											text: "Nama"
											font.bold: true
											Layout.preferredWidth: root.installerNameColumnWidth
											color: "#1F2937"
										}

										Text {
											text: "Versi"
											font.bold: true
											Layout.preferredWidth: root.installerVersionColumnWidth
											color: "#1F2937"
										}

										Text {
											text: "Aksi"
											font.bold: true
											Layout.preferredWidth: root.installerActionColumnWidth
											horizontalAlignment: Text.AlignHCenter
											color: "#1F2937"
										}
									}
								}

								delegate: Rectangle {
									width: ListView.view.width
									height: 56
									radius: 6
									border.color: "#E5E7EB"
									color: index % 2 === 0 ? "#FFFFFF" : "#FAFAFA"

									RowLayout {
										anchors.fill: parent
										anchors.margins: 10
										spacing: 8

										Text {
											text: modelData.installer_name
											Layout.preferredWidth: root.installerNameColumnWidth
											font.bold: true
											color: "#111827"
											elide: Text.ElideRight
										}

										Text {
											text: modelData.version
											Layout.preferredWidth: root.installerVersionColumnWidth
											color: "#374151"
											elide: Text.ElideRight
										}

										RowLayout {
											Layout.preferredWidth: root.installerActionColumnWidth
											spacing: 6

											AppButton {
												text: "Install"
												Layout.preferredWidth: 92
												onClicked: {
													const ok = installerModel.installInstaller(modelData.id)
													root.appendLog(ok ? "Installer dijalankan: " + modelData.installer_name : "Gagal menjalankan installer: " + modelData.installer_name,
													              ok ? "success" : "error")
												}
											}

											AppButton {
												tone: "secondary"
												iconText: "✎"
												iconOnly: true
												Layout.preferredWidth: 40
												ToolTip.visible: hovered
												ToolTip.text: "Edit"
												onClicked: installerFormPopup.openForEdit(modelData)
											}

											AppButton {
												tone: "danger"
												iconText: "✕"
												iconOnly: true
												Layout.preferredWidth: 40
												ToolTip.visible: hovered
												ToolTip.text: "Delete"
												onClicked: {
													const ok = installerModel.deleteInstaller(modelData.id)
													root.appendLog(ok ? "Installer dihapus: " + modelData.installer_name : "Gagal menghapus installer: " + modelData.installer_name,
													              ok ? "success" : "error")
												}
											}
										}
									}
								}
							}

							Text {
								Layout.fillWidth: true
								text: installerModel.installers.length === 0 ? "Belum ada data installer." : ""
								visible: installerModel.installers.length === 0
								horizontalAlignment: Text.AlignHCenter
								color: "#6B7280"
							}
						}
					}
				}

				Rectangle {
					Layout.fillWidth: true
					Layout.fillHeight: true
					Layout.preferredWidth: parent.width * 0.5
					radius: 8
					color: "#FFFFFF"
					border.color: "#E5E7EB"

					ColumnLayout {
						anchors.fill: parent
						anchors.margins: 10
						spacing: 8

						RowLayout {
							Layout.fillWidth: true

							Text {
								text: "Panel Log"
								font.pixelSize: 16
								font.bold: true
								color: "#1F2937"
							}

							Item {
								Layout.fillWidth: true
							}

							AppButton {
								tone: "secondary"
								text: "Clear"
								enabled: logModel.count > 0
								onClicked: logModel.clear()
							}
						}

						Rectangle {
							Layout.fillWidth: true
							height: 1
							color: "#E5E7EB"
						}

						ListView {
							Layout.fillWidth: true
							Layout.fillHeight: true
							clip: true
							model: logModel
							spacing: 6

							delegate: Rectangle {
								width: ListView.view.width
								radius: 6
								color: model.level === "error" ? "#FFF1F2" : (model.level === "success" ? "#ECFDF3" : "#F8FAFC")
								border.color: model.level === "error" ? "#FECACA" : (model.level === "success" ? "#BBF7D0" : "#E5E7EB")
								height: logText.implicitHeight + 16

								Column {
									anchors.fill: parent
									anchors.margins: 8
									spacing: 2

									Text {
										text: model.time
										font.pixelSize: 11
										color: "#6B7280"
									}

									Text {
										id: logText
										text: model.text
										font.pixelSize: 12
										wrapMode: Text.Wrap
										color: model.level === "error" ? "#991B1B" : (model.level === "success" ? "#065F46" : "#1F2937")
									}
								}
							}
						}

						Text {
							Layout.fillWidth: true
							horizontalAlignment: Text.AlignHCenter
							text: logModel.count === 0 ? "Belum ada log aktivitas." : ""
							visible: logModel.count === 0
							color: "#6B7280"
						}
					}
				}
			}
		}

		Label {
			Layout.fillWidth: true
			text: installerModel.statusMessage
			color: "#2563EB"
			wrapMode: Text.Wrap
		}
	}
}
