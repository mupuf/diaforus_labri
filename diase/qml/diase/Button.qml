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
import QtQuick 1.0

Rectangle {

    property alias text: label.text
    property alias icon: icon.source
    property alias textRotation: label.rotation
    property alias textSize: label.font.pointSize

    signal clicked

    color: "black"
    width: 75
    height: 50
    opacity: 0.7
    radius: 15
    smooth: true

    gradient: Gradient {
        GradientStop { id: stop1; position: 0.0; color: "white" }
        GradientStop { id: stop2; position: 0.75; color: 'black' }
    }

    Rectangle {
        id: shade
        anchors.fill: parent
        radius: 15
        color: "red"
        opacity: 0
    }

    Text {
        id: label
        color: "white"
        transformOrigin: Item.Center
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 10
    }

    Image {
        id: icon
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        onClicked: parent.clicked()
    }

    states: State {
        when: mouse.pressed
        PropertyChanges { target: shade; opacity: 0.8}
    }

}
