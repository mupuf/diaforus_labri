/*
 *   Copyright (C) 2012  Romain Perier <romain.perier@labri.fr>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 1.1


Rectangle {
    smooth: true
    color: "white"
    radius: 20
    border.color: "black"
    border.width: 2


    property string nodeName: ""
    property string resourceName: ""
    property string value: ""
    property string delegatedPlugin: ""
    property int modelEntryIndex: 0
    property int nodeId : 0

    signal exit()

    function updateContent(index, keys, values) {
        var result = null

        if (index != modelEntryIndex)
            return;
        result = values.split("|")
        value = result[result.length - 1]
    }

    Text {
        id: close
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.right: parent.right
        height: 10
        width: 10
        text: "+"
        font.pointSize: 12
        rotation: 45
        visible: false
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: {
                mouseArea.enabled = false
                close.color = "red"
            }
            onExited: {
                mouseArea.enabled = true
                close.color = "black"
            }
            onClicked: {
                exit()
            }
        }
    }

    Text {
        id: refresh
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.right: close.left
        height: 10
        width: 10
        text: "-"
        font.pointSize: 12
        visible: false
    }

    Text { anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter; text: nodeName }
    Column {
        id: column
        anchors.centerIn: parent
        spacing: 2
        Text { text: resourceName }
        Text { id: textValue; text: value }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onPressAndHold: {
            close.visible = !close.visible
        }
    }

    onValueChanged: textValue.text = value
}
