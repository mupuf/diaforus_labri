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

Item {
    // public properties
    property int area: 0
    property int sensorId: 0
    property int range: 0;
    property string type: null
    property string className: "Sensor"
    property int modelIndex: 0
    property int z_coord: 0
    z: 2

    // public signals
    signal collision(bool collide, int abs, int ord)
    signal moveTo(int x, int y)
    signal mouseAcquired(bool acquired)
    signal intrusionDetected(int value)
    signal sensorClicked

    // private properties
    property real noiseP: 0

    Rectangle {
        id: ref
        width: 5
        height: 5
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
    }

    Rectangle {
        id: rotationBox
        width: 100
        height: 100
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        color: "transparent"
        border.color: "black"
        border.width: 2
        radius: 50
        opacity: 0.4
        visible: false
        smooth: true

           Rectangle {
            id: rotationButton
            width: 10
            height: 10
            color: "black"
            opacity: 0.8
            radius: 50
            anchors.top: rotationBox.top
            anchors.horizontalCenter: rotationBox.horizontalCenter
            visible: false
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                var Y = mouseY- ref.y
                var X = mouseX - ref.x
                var angle = 0

                if (X > 0) {
                    angle = Math.atan(Y / X) * 180 / Math.PI
                } else if (X < 0 && Y >= 0) {
                    angle = (Math.atan(Y / X) + Math.PI) * 180 / Math.PI
                } else if (X < 0 && Y < 0) {
                    angle = (Math.atan(Y / X) - Math.PI) * 180 / Math.PI
                } else if (X == 0 && Y >= 0) {
                    angle = (Math.PI / 2) * 180 / Math.PI;
                } else if (X == 0 && Y < 0) {
                    angle = (- Math.PI / 2) * 180 / Math.PI;
                }

                console.log("ref.x " + ref.x)
                console.log("ref.y " + ref.y)
                console.log("mouseX " + mouseX)
                console.log("mouseY " + mouseY)
                console.log("diffY " + (mouseY - ref.y))
                console.log("diffX " + (mouseX - ref.x))
                rotationBox. rotation = 0
                var normalizedAngle = 0
                if (angle <= 0) {
                    normalizedAngle = 90 - Math.abs(angle)
                    //normalizedAngle = Math.abs(normalizedAngle - rotationBox.rotation)
                } else {
                    normalizedAngle = 90 + angle
                }

                console.log("angle " + angle)
                console.log("normalizedAngle " + normalizedAngle)
                parent.parent.rotation = angle
            }
        }
    }

    // Random noise
    Timer {
        interval: 1000
        repeat: true
        running: true

        onTriggered: {
            // Bernouilli law
            var number = Math.random()
            if (number < noiseP)
                intrusionDetected(1)
        }
    }

    MouseArea {
        property bool rotate: false
        id: movementMouseArea
        anchors.fill: parent
        hoverEnabled: false
        onPressAndHold: {
            //rotate = !rotate
            //rotationBox.visible = rotate
            //rotationButton.visible = rotate
            //enabled = !rotate
        }
        onClicked: sensorClicked()
        onPressed:  {
            hoverEnabled = pressed
            mouseAcquired(pressed)
        }
        onReleased: {
            hoverEnabled = !released
            mouseAcquired(!released)
        }
        onPositionChanged: {
            moveTo(mouseX, mouseY)
        }
    }
}
