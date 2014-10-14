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
    width: 80
    height: 80
    type: "SPIRIT"
    rotation: 0
    id: that

    property color defaultColor: "lime"
    property real minOpacity: 0.4
    property real maxOpacity: 0.9

    // The source
    Rectangle {
        id: source
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        width: 10
        height: 10
        color: "black"
        opacity: 0.9
        smooth: true
        radius: 50
    }

    Rectangle {
        id: beam1
        transformOrigin: Item.Bottom
        anchors.bottom: source.top
        anchors.top: parent.top
        anchors.horizontalCenter: source.horizontalCenter
        width: 2
        color: defaultColor
        smooth: true
        rotation: -30
        opacity: 0.4
    }

    Rectangle {
        id: beam2
        transformOrigin: Item.Bottom
        anchors.bottom: source.top
        anchors.top: parent.top
        anchors.horizontalCenter: source.horizontalCenter
        width: 2
        color: defaultColor
        smooth: true
        rotation: -20
        opacity: 0.4
    }

    Rectangle {
        id: beam3
        transformOrigin: Item.Bottom
        anchors.bottom: source.top
        anchors.top: parent.top
        anchors.horizontalCenter: source.horizontalCenter
        width: 2
        color: defaultColor
        smooth: true
        rotation: -10
        opacity: 0.4
    }

    Rectangle {
        id: beam4
        transformOrigin: Item.Bottom
        anchors.bottom: source.top
        anchors.top: parent.top
        anchors.horizontalCenter: source.horizontalCenter
        width: 2
        color: defaultColor
        smooth: true
        rotation: 0
        opacity: 0.4
    }

    Rectangle {
        id: beam5
        transformOrigin: Item.Bottom
        anchors.bottom: source.top
        anchors.top: parent.top
        anchors.horizontalCenter: source.horizontalCenter
        width: 2
        color: defaultColor
        smooth: true
        rotation: 10
        opacity: 0.4
    }


    Rectangle {
        id: beam6
        transformOrigin: Item.Bottom
        anchors.bottom: source.top
        anchors.top: parent.top
        anchors.horizontalCenter: source.horizontalCenter
        width: 2
        color: defaultColor
        smooth: true
        rotation: 20
        opacity: 0.4
    }

    Rectangle {
        id: beam7
        transformOrigin: Item.Bottom
        anchors.bottom: source.top
        anchors.top: parent.top
        anchors.horizontalCenter: source.horizontalCenter
        width: 2
        color: defaultColor
        smooth: true
        rotation: 30
        opacity: 0.4
    }

    SequentialAnimation {
        id: beamsAnimation
        loops: Animation.Infinite
        running: true

        //ParallelAnimation {
            PropertyAnimation { target: beam1; property: "opacity"; to: 0.9; duration: 500 }
            PropertyAnimation { target: beam1; property: "opacity"; to: 0.4; duration: 500 }
        //}
        //ParallelAnimation {
            PropertyAnimation { target: beam2; property: "opacity"; to: 0.9; duration: 500 }
            PropertyAnimation { target: beam2; property: "opacity"; to: 0.4; duration: 500 }
        //}
        //ParallelAnimation {
            PropertyAnimation { target: beam3; property: "opacity"; to: 0.9; duration: 500 }
            PropertyAnimation { target: beam3; property: "opacity"; to: 0.4; duration: 500 }
        //}
        //ParallelAnimation {
            PropertyAnimation { target: beam4; property: "opacity"; to: 0.9; duration: 500 }
            PropertyAnimation { target: beam4; property: "opacity"; to: 0.4; duration: 500 }
        //}
        //ParallelAnimation {
            PropertyAnimation { target: beam5; property: "opacity"; to: 0.9; duration: 500 }
            PropertyAnimation { target: beam5; property: "opacity"; to: 0.4; duration: 500 }

            PropertyAnimation { target: beam6; property: "opacity"; to: 0.9; duration: 500 }
            PropertyAnimation { target: beam6; property: "opacity"; to: 0.4; duration: 500 }
            PropertyAnimation { target: beam7; property: "opacity"; to: 0.9; duration: 500 }
            PropertyAnimation { target: beam7; property: "opacity"; to: 0.4; duration: 500 }
    }

    states: State {
        name: "alarm"
        PropertyChanges { target: beam1; color : "red"}
        PropertyChanges { target: beam2; color : "red"}
        PropertyChanges { target: beam3; color : "red"}
        PropertyChanges { target: beam4; color : "red"}
        PropertyChanges { target: beam5; color : "red"}
        PropertyChanges { target: beam6; color : "red"}
        PropertyChanges { target: beam7; color : "red"}
    }

   onCollision: {
       var array = [ beam1, beam2, beam3, beam4, beam5, beam6, beam7 ]
       var i = 0
       if (collide) {
           var angle = Math.atan((ord - source.y) / (abs - source.x)) * 180 / Math.PI
           var index = 0

           console.log("angle " + angle)

           beamsAnimation.running = false

           for (i = 0; i < 7 ; i++) {
               array[i].color = "red"
               array[i].opacity = minOpacity
           }

           var normalizedAngle = 0
           if (angle <= 0) {
               normalizedAngle = 90 - Math.abs(angle)
           } else {
               normalizedAngle = - (90 - angle)
           }

           for (i = 0; i < 7; i++) {
               if (array[i].rotation < normalizedAngle) {
                   index = i
               }
           }
           array[index].opacity = 0.9
           intrusionDetected(1)
       } else {
           for (i = 0; i < 7; i++) {
               array[i].color = defaultColor
               array[i].opacity = minOpacity
           }
           beamsAnimation.running = true
       }
   }
}
