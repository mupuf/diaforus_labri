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
    id: rect
    objectName: "intruder"
    width: 30
    height: 45
    color: "transparent"
    smooth: true
    z: 1

    // Public properties
    property variant destination: null
    property alias duration: translation.duration
    property alias leftSide:  image.mirror
    property string className: "Intruder"
    property alias skin: image.source
    property real speed: 6 // km/h
    property real weight: 0 // kg

    // Public signals
    signal hasMoved(variant obj)
    signal startAnimation

    Image  {
        id: image
        anchors.fill: parent
        source: "qrc:/icons/intruder-black.png"
    }

    states: [
        State {
            name: "moving"
            PropertyChanges { target: rect; x: destination.x; y: destination.y }
        },
        State {
            name: "stop"
            PropertyChanges { target: rect; x: destination.x; y: destination.y }
        }
    ]

    transitions: [
        Transition {
            from: "*"; to: "moving"
            SequentialAnimation {
                PropertyAnimation { id: translation; properties: "x,y"; duration: 1000 }
                ScriptAction {
                    scriptName: "movingDone"
                    script: {
                        state = "stop"
                        startAnimation()
                    }
                }
            }
        },
        Transition {
            from: "*"; to: "stop"
            PropertyAnimation { properties: "x,y"; duration: 10}
        }
    ]

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: {
            if (state == "moving")
                hasMoved(rect)
        }
    }
}
