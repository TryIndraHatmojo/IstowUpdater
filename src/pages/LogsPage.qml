import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import IstowUpdater

Rectangle {
    id: root
    color: "#FFFFFF"
    
    LogModel {
        id: logModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "Activity Logs"
                font.pixelSize: 24
                font.bold: true
                color: "#2C3E50"
            }
            
            Item { Layout.fillWidth: true }
            
            AppButton {
                text: "Refresh Logs"
                onClicked: logModel.loadLogs()
            }
        }

        // Table
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            radius: 8
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                
                // Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#EDF2F7"
                    radius: 8
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 16
                        
                        Text { text: "Timestamp"; font.bold: true; Layout.preferredWidth: 200 }
                        Text { text: "Action"; font.bold: true; Layout.preferredWidth: 200 }
                        Text { text: "Target/Ship"; font.bold: true; Layout.preferredWidth: 150 }
                        Text { text: "Status"; font.bold: true; Layout.preferredWidth: 100 }
                        Text { text: "Status Message / Details"; font.bold: true; Layout.fillWidth: true }
                    }
                }
                
                ListView {
                    id: logListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: logModel
                    
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: Math.max(36, detailText.implicitHeight + 16)
                        color: index % 2 === 0 ? "#FAFAFA" : "#FFFFFF"
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 16
                            
                            Text { text: model.timestamp; Layout.preferredWidth: 200; font.pixelSize: 13 }
                            Text { text: model.action; Layout.preferredWidth: 200; font.pixelSize: 13 }
                            Text { text: model.target_ship; Layout.preferredWidth: 150; font.pixelSize: 13 }
                            Text { 
                                text: model.status
                                color: model.status === "Success" ? "#2E7D32" : (model.status === "Failed" ? "#C62828" : "#455A64")
                                font.bold: true
                                Layout.preferredWidth: 100 
                                font.pixelSize: 13
                            }
                            Text { 
                                id: detailText
                                text: model.status_message
                                elide: Text.ElideRight
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                                font.pixelSize: 13
                            }
                            AppButton {
                                text: "Tampilkan Log"
                                visible: model.log_path !== undefined && model.log_path !== ""
                                onClicked: {
                                    if (model.log_path !== undefined && model.log_path !== "") {
                                        logModel.openLogPath(model.log_path)
                                    }
                                }
                                Layout.rightMargin: 16
                            }
                        }
                    }
                }
            }
        }
    }
}