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
import CustomComponent 1.0

DashedLine {
    property string className: "Range"
    property alias distance: label.text

    penWidth: 2.0
    color: "yellow"
    penDashed: true
    smooth: true
    //opacity: 0.5

    Rectangle {
        id: info
        color: "black"
        opacity: 0.5
        anchors.centerIn: parent
        width: label.width+8
        height: label.height+8
        visible: false

        Text {
            id: label
            text: "0%"
            color: "white"
            anchors.centerIn: info
        }
    }

    MouseArea {
        id: mouseArea
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        height: penWidth
        transformOrigin: Item.Left
        hoverEnabled: true

        function fillLine() {
            var distance = Math.sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))
            var angle = Math.atan((y2-y1)/(x2-x1)) * 180 / Math.PI

            mouseArea.width = distance
            mouseArea.rotation = angle
        }

        function anchorToLeft() {
            mouseArea.anchors.left = mouseArea.parent.left
            mouseArea.anchors.right = undefined
            mouseArea.transformOrigin = Item.Left
        }

        function anchorToRight() {
            mouseArea.anchors.right = mouseArea.parent.right
            mouseArea.anchors.left = undefined
            mouseArea.transformOrigin = Item.Right
        }

        onRotationChanged: {
            if (rotation < 0)
                anchorToLeft()
            else if (rotation >= 0)
                anchorToRight()
        }

        onEntered:info.visible = true
        onExited: info.visible = false
    }

    onX1Changed: mouseArea.fillLine()
    onY1Changed: mouseArea.fillLine()
    onX2Changed: mouseArea.fillLine()
    onY2Changed: mouseArea.fillLine()
}
