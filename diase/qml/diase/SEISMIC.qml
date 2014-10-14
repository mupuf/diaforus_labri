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

Sensor {
    width: 100
    height: 100
    type: "SEISMIC"

    property color defaultColor: "lime"

    Rectangle {
        id: areaDetection
        width: 30
        height: 30
        radius: 50
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        color: "transparent"

        border.color: defaultColor
        border.width: 2
        opacity: 0.7
        smooth: true
    }

    Rectangle {
        id: backgroundAreaDetection
        width: 30
        height: 30
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        radius: 50
        color: defaultColor
        opacity: 0.3
        smooth: true
    }

    Rectangle {
        id: sensor
        width: 10
        height: 10
        radius: 50
        color: "black"
        opacity: 0.9
        smooth: true

        anchors.verticalCenter: areaDetection.verticalCenter
        anchors.horizontalCenter: areaDetection.horizontalCenter
    }

    states: State {
        name: "alarm"
        PropertyChanges { target: areaDetection; border.color: "red" }
        PropertyChanges { target: backgroundAreaDetection; color: "red" }
    }

    SequentialAnimation {
        loops: Animation.Infinite
        running: true

        ParallelAnimation {
            PropertyAnimation { target: backgroundAreaDetection; properties:"width, height"; to: 30; duration: 500 }
            PropertyAnimation { target: areaDetection; properties:"width, height"; to: 30; duration: 500 }
        }
        ParallelAnimation {
            PropertyAnimation { target: backgroundAreaDetection; property:"opacity"; to: 0.3; duration: 500 }
            PropertyAnimation { target: areaDetection; property:"opacity"; to: 0.7; duration: 500 }
        }

        ParallelAnimation {
            PropertyAnimation { target: backgroundAreaDetection; properties:"width, height"; to: 55; duration: 500 }
            PropertyAnimation { target: areaDetection; properties:"width, height"; to: 55; duration: 500 }
        }
        ParallelAnimation {
            PropertyAnimation { target: backgroundAreaDetection; properties:"width, height"; to: 80; duration: 500 }
            PropertyAnimation { target: areaDetection; properties:"width, height"; to: 80; duration: 500 }
        }
        ParallelAnimation {
            PropertyAnimation { target: backgroundAreaDetection; properties:"width, height"; to: 100; duration: 500 }
            PropertyAnimation { target: areaDetection; properties:"width, height"; to: 100; duration: 500 }
        }
        ParallelAnimation {
            PropertyAnimation { target: backgroundAreaDetection; property:"opacity"; to: 0; duration: 500 }
            PropertyAnimation { target: areaDetection; property:"opacity"; to: 0; duration: 500 }
        }
    }

    onCollision: {
        defaultColor = collide ? "red" : "lime"
        if (collide)
            intrusionDetected(1)
    }
}
