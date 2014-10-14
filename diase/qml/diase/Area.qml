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
    property string className: "Area"
    property int modelIndex: 0
    property alias name: label.text
    property alias ackEnabled : mouseArea.enabled
    signal ack(variant obj)

    id: area
    color: "transparent"
    border.color: "cornflowerblue"
    border.width: 2
    smooth: true

    states: State {
        name: "alarm"
        PropertyChanges { target: area; color: "red"; border.color: "red"; opacity: 0.2 }
    }

    Text {
        id: label
        text: "Area <id>"
        color: "white"
        font.bold: true
        smooth: true
        anchors.bottom: parent.top
        anchors.right: parent.right
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: false
        onClicked: ack(area)
    }
}
