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

Image {
    property string className: "Node"
    property int modelIndex: 0
    property int nodeId: 0
    property alias imgSrc: extraIcon.source
    signal mouseAcquired(bool acquired)
    signal moveTo(int x, int y)
    signal mouseEntered
    signal mouseExited
    signal nodeClicked

    id: node
    width: 48
    height: 48
    fillMode: Image.PreserveAspectFit
    smooth: true
    source: "images/antenne.png"

    states: State {
        name: "fail"
        PropertyChanges { target: node; source: "images/antenne_alarme.png" }
    }

    Text {
    	 objectName: "label"
	 color: "white"
         font.bold: true
         smooth: true
         anchors.bottom: parent.top
         anchors.left: parent.right
	 text: "Node " + nodeId
    }

    Image {
        id: extraIcon
        anchors.left: parent.right
        anchors.bottom: parent.bottom
        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    MouseArea {
        property bool moving: false
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent

        onClicked: nodeClicked()
        onEntered: mouseEntered()
        onExited:  mouseExited()
        onPressed:  {
            moving = pressed
            mouseAcquired(pressed)
        }
        onReleased: {
            moving = !released
            mouseAcquired(!released)
        }
        onPositionChanged: {
            if (moving)
                moveTo(mouseX, mouseY)
        }
    }
}
