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

Sensor  {
    width: 70
    height: 10
    type: "PIR"

    property alias color: beam.color

    // The source
    Rectangle {
        id: source
        anchors.top: parent.top
        anchors.left: parent.left
        width: 10
        height: 10
        color: "black"
        opacity: 0.9
        smooth: true
    }

    // The beam
    Rectangle {
        id: beam
        anchors.left: source.right
        anchors.right: parent.right
        anchors.verticalCenter: source.verticalCenter
        width: 20
        height: 2
        color: "lime"
        smooth: true
        opacity: 0.7
    }

    states: State {
        name: "alarm"
        PropertyChanges { target: beam; color: "red" }
    }

    onCollision: {
        beam.color = collide ? "red" : "lime"
        if (collide)
            intrusionDetected(1)
    }
}
