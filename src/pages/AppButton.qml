import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string tone: "primary" // primary | secondary | danger
    property string iconText: ""
    property bool iconOnly: false
    property int iconPixelSize: 14

    implicitHeight: 38
    font.pixelSize: 13
    font.bold: true
    spacing: 6
    leftPadding: 12
    rightPadding: 12

    readonly property string foregroundColor: {
        if (!control.enabled) return "#718096"
        return control.tone === "secondary" ? "#1F2937" : "#FFFFFF"
    }

    background: Rectangle {
        radius: 8
        color: {
            if (!control.enabled) return "#CBD5E0"
            if (control.tone === "danger") return control.hovered ? "#9B2C2C" : "#C53030"
            if (control.tone === "secondary") return control.hovered ? "#CBD5E0" : "#E2E8F0"
            return control.hovered ? "#2B6CB0" : "#1A3A5C"
        }
    }

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: control.iconText.length > 0 && !control.iconOnly && control.text.length > 0 ? control.spacing : 0

            Text {
                visible: control.iconText.length > 0
                text: control.iconText
                font.pixelSize: control.iconPixelSize
                font.bold: true
                color: control.foregroundColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                visible: !control.iconOnly && control.text.length > 0
                text: control.text
                font: control.font
                color: control.foregroundColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }
    }
}