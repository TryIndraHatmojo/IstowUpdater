import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IstowUpdater

Item {
	id: root

	BackupSessionModel {
		id: backupModel
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
					text: "Rollback Assets"
					color: "#FFFFFF"
					font.pixelSize: 18
					font.bold: true
				}

				Item { Layout.fillWidth: true }

				Button {
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
				spacing: 8

				ColumnLayout {
					Layout.fillWidth: true
					Layout.fillHeight: true
					spacing: 8

					Rectangle {
						Layout.fillWidth: true
						height: 40
						radius: 6
						color: "#EDF2F7"

						RowLayout {
							anchors.fill: parent
							anchors.leftMargin: 10
							anchors.rightMargin: 10
							spacing: 8

							Text { text: "Ship ID"; font.bold: true; Layout.preferredWidth: 80; color: "#2D3748" }
							Text { text: "Ship Name"; font.bold: true; Layout.preferredWidth: 220; color: "#2D3748" }
							Text { text: "Folder Name"; font.bold: true; Layout.fillWidth: true; color: "#2D3748" }
							Text { text: "Created At"; font.bold: true; Layout.preferredWidth: 170; color: "#2D3748" }
							Text { text: "Aksi"; font.bold: true; horizontalAlignment: Text.AlignHCenter; Layout.preferredWidth: 110; color: "#2D3748" }
						}
					}

					ScrollView {
						Layout.fillWidth: true
						Layout.fillHeight: true
						clip: true

						ListView {
							id: sessionList
							width: parent.width
							model: backupModel.sessions
							spacing: 6

							delegate: Rectangle {
								required property var modelData

								width: sessionList.width
								height: 52
								radius: 6
								color: index % 2 === 0 ? "#FAFAFA" : "#F7FAFC"
								border.color: "#E2E8F0"
								border.width: 1

								RowLayout {
									anchors.fill: parent
									anchors.leftMargin: 10
									anchors.rightMargin: 10
									spacing: 8

									Text {
										text: String(modelData.idship ?? "-")
										Layout.preferredWidth: 80
										color: "#2D3748"
										elide: Text.ElideRight
									}

									Text {
										text: modelData.ship_name ?? "-"
										Layout.preferredWidth: 220
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
										Layout.preferredWidth: 170
										color: "#2D3748"
										elide: Text.ElideRight
									}

									Button {
										text: backupModel.rollingBack ? "Proses..." : "Rollback"
										Layout.preferredWidth: 110
										enabled: !backupModel.rollingBack
										onClicked: {
											const ok = backupModel.rollbackSession(Number(modelData.id ?? 0))
											if (ok) {
												rollbackResult.text = "Rollback berhasil untuk folder: " + (modelData.folder_name ?? "-")
											} else {
												rollbackResult.text = "Rollback gagal untuk folder: " + (modelData.folder_name ?? "-")
											}
											rollbackDialog.open()
										}
									}
								}
							}
						}
					}

					Text {
						Layout.fillWidth: true
						text: backupModel.sessions.length === 0 ? "Belum ada data backup sessions." : ""
						color: "#718096"
						font.pixelSize: 12
						horizontalAlignment: Text.AlignHCenter
						visible: backupModel.sessions.length === 0
					}
				}

				Rectangle {
					Layout.fillHeight: true
					Layout.preferredWidth: 340
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
								text: "Log Rollback"
								font.pixelSize: 14
								font.bold: true
								color: "#1A3A5C"
							}

							Item { Layout.fillWidth: true }

							Button {
								text: "Clear"
								enabled: !backupModel.rollingBack && backupModel.rollbackLogs.length > 0
								onClicked: backupModel.clearLogs()
							}
						}

						Rectangle {
							Layout.fillWidth: true
							height: 1
							color: "#D9E2EC"
						}

						ScrollView {
							Layout.fillWidth: true
							Layout.fillHeight: true
							clip: true

							ListView {
								model: backupModel.rollbackLogs
								spacing: 4

								delegate: Text {
									required property string modelData
									text: modelData
									font.pixelSize: 12
									font.family: "Consolas"
									wrapMode: Text.Wrap
									width: ListView.view.width
									color: {
										if (modelData.includes("ERROR") || modelData.includes("GAGAL")) return "#C53030"
										if (modelData.includes("RESTORE") || modelData.includes("Selesai")) return "#2F855A"
										return "#2D3748"
									}
								}
							}
						}

						Text {
							Layout.fillWidth: true
							text: backupModel.rollbackLogs.length === 0 ? "Belum ada log rollback." : ""
							color: "#718096"
							font.pixelSize: 12
							horizontalAlignment: Text.AlignHCenter
							visible: backupModel.rollbackLogs.length === 0
						}
					}
				}
			}
		}
	}

	Dialog {
		id: rollbackDialog
		modal: true
		title: "Rollback Assets"
		anchors.centerIn: Overlay.overlay
		standardButtons: Dialog.Ok

		Label {
			id: rollbackResult
			text: ""
			wrapMode: Text.Wrap
			width: 320
		}
	}
}
