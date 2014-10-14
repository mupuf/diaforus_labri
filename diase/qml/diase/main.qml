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
    anchors.fill: parent
    color: "black"

    property bool editable: false
    property bool deployment: false
    property bool scenarioStarted: false
    signal qmlItemCreated(variant item)
    signal reset
    signal startAnimation
    signal remove(variant item)

    MapView {
        id: mapView
        objectName: "mapView"
        source: "images/carte_stic.png"

        states: State {
            when: mapView.movingVertically || mapView.movingHorizontally
            PropertyChanges { target: verticalScrollBar; opacity: 1 }
            PropertyChanges { target: horizontalScrollBar; opacity: 1 }
        }
    }

    // Attach scrollbars to the right and bottom edges of the view.
    ScrollBar {
        id: verticalScrollBar
        width: 12; height: mapView.height-12
        anchors.right: mapView.right
        opacity: 0
        orientation: Qt.Vertical
        position: mapView.visibleArea.yPosition
        pageSize: mapView.visibleArea.heightRatio
    }

    ScrollBar {
        id: horizontalScrollBar
        width: mapView.width-12; height: 12
        anchors.bottom: mapView.bottom
        opacity: 0
        orientation: Qt.Horizontal
        position: mapView.visibleArea.xPosition
        pageSize: mapView.visibleArea.widthRatio
    }

    // Attach a button box to the right of the view
    Column {
        id: buttonBox
        width: 50
        //anchors.verticalCenterOffset: 0
        anchors.rightMargin: 25
        anchors.right: horizontalScrollBar.right
        anchors.verticalCenter: mapView.verticalCenter
        spacing: 2

        Button {
            icon: "qrc:/icons/intruder.png"
            visible: editable

            onClicked: {
                drawingmouseArea.enabled = true
                drawingmouseArea.drawingType = "intruder"
                drawingmouseArea.hoverEnabled = true
            }
        }

        Button {
            icon: "qrc:/icons/car-white.png"
            visible: editable

            onClicked: {
                drawingmouseArea.enabled = true
                drawingmouseArea.drawingType = "car"
                drawingmouseArea.hoverEnabled = true
            }
        }

        Button {
            icon: "qrc:/icons/path.png"
            visible: editable

            onClicked: {
                if (!drawingmouseArea.enabled) {
                    drawingmouseArea.enabled = true
                    drawingmouseArea.drawingType = "path"
                    drawingmouseArea.hoverEnabled = true
                } else if (drawingmouseArea.enabled && drawingmouseArea.drawingType == "path") {
                    drawingmouseArea.enabled = false
                    drawingmouseArea.drawingType = ""
                    drawingmouseArea.hoverEnabled = false
                    drawingmouseArea.tempObj.destroy()
                }
            }
        }

        Button {
            text: "SEISMIC"
            visible: deployment
            onClicked: {
                drawingmouseArea.drawingType = "sensor"
                drawingmouseArea.sensorType = "SEISMIC"
                drawingmouseArea.enabled = true
                drawingmouseArea.hoverEnabled = true
            }
        }

        Button {
            text: "PIR"
            visible: deployment
            onClicked: {
                drawingmouseArea.drawingType = "sensor"
                drawingmouseArea.sensorType = "PIR"
                drawingmouseArea.enabled = true
                drawingmouseArea.hoverEnabled = true
            }
        }

        Button {
            text: "Node"
            visible: deployment
            onClicked: {
                drawingmouseArea.drawingType = "node"
                drawingmouseArea.enabled = true
                drawingmouseArea.hoverEnabled = true
            }
        }

        Button {
            text: "+"
            textSize: 16
            textRotation: 45
            visible: editable || deployment
            onClicked: {
                drawingmouseArea.drawingType = "remove"
                drawingmouseArea.enabled = true
                drawingmouseArea.hoverEnabled = false
            }
        }

        Button {
            icon: "qrc:/icons/start.png"
            visible: editable
            onClicked: {
                if (!scenarioStarted) {
                    scenarioStarted = true
                    icon = "qrc:/icons/stop.png"
                    startAnimation();
                } else {
                    scenarioStarted = false
                    icon = "qrc:/icons/start.png"
                    reset()
                }
            }
        }

    }


    MouseArea {
        id: drawingmouseArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: buttonBox.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: horizontalScrollBar.height
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        enabled: false
        hoverEnabled: false

        property string drawingType
        property string sensorType
        property variant srcPath: null
        property variant dstPath: null
        property variant tempObj: null
        property int numberOfClicks: 0
        property int nodeidcount: 0
        property int sensoridcount: 0
        property real clickedMouseX: 0
        property real clickedMouseY: 0

        onPositionChanged: {
            var mapX = mapView.contentX + mouseX
            var mapY = mapView.contentY + mouseY
            var comp =  null

            if (drawingType == "path") {
                if (srcPath && dstPath == null) {
                    if (tempObj)
                        tempObj.destroy()
                    comp = Qt.createComponent("Path.qml")
                    tempObj = comp.createObject(mapView.contentItem, { "x1": srcPath.x, "y1": srcPath.y, "x2": mapX, "y2": mapY, "opacity": 0.4})
                }
            }
            else if (drawingType == "intruder") {

                if (tempObj)
                    tempObj.destroy()
                comp = Qt.createComponent("Intruder.qml")
                tempObj = comp.createObject(mapView.contentItem, { "x": mapX - 10, "y": mapY - 10, "opacity": 0.4})
            }
            else if (drawingType == "sensor") {
                if (tempObj)
                    tempObj.destroy()
                comp = Qt.createComponent(sensorType + ".qml")
                if (sensorType == "SEISMIC")
                    tempObj = comp.createObject(mapView.contentItem, { "x": mapX - 50, "y": mapY - 50, "opacity": 0.4})
                else if (sensorType == "PIR")
                    if (numberOfClicks == 0)
                        tempObj = comp.createObject(mapView.contentItem, { "x": mapX, "y": mapY, "opacity": 0.4})
                    else if (numberOfClicks == 1)
                        tempObj = comp.createObject(mapView.contentItem, { "x": clickedMouseX, "y": clickedMouseY, "opacity": 0.4, "rotation": mapX - clickedMouseX})
            }
            else if (drawingType == "node") {
                if (tempObj)
                    tempObj.destroy()
                comp = Qt.createComponent("Node.qml")
                tempObj = comp.createObject(mapView.contentItem, { "x": mapX - 24, "y": mapY - 24, "opacity": 0.4})
            }
            else if (drawingType == "area") {
                if (tempObj)
                    tempObj.destroy()
                var diameter = 400
                comp = Qt.createComponent("Area.qml")
                tempObj = comp.createObject(mapView.contentItem, { "width": diameter, "height": diameter, "x": mapX - diameter / 2, "y": mapY - diameter / 2, "opacity": 0.4})

            }
            else if (drawingType == "car") {
                if (tempObj)
                    tempObj.destroy()
                comp = Qt.createComponent("Intruder.qml")
                tempObj = comp.createObject(mapView.contentItem, { "skin": "qrc:/icons/car-black.png", "width": 32, "height": 16, "x": mapX - 10, "y": mapY - 10, "opacity": 0.4})
            }
        }

        onClicked: {
            var mapX = mapView.contentX + mouseX
            var mapY = mapView.contentY + mouseY
            var component = null
            var obj = null

            console.log("mouseX " + mapX + "," + "mouseY " + mapY)

            if (drawingType == "path") {

                if (mouse.button == Qt.RightButton) {
                    if (tempObj)
                        tempObj.destroy()
                    dstPath = null
                    srcPath = null
                    drawingType = ""
                    enabled = false
                    hoverEnabled = false
                    qmlItemCreated(obj)
                    return
                }

                if (srcPath == null) {
                    srcPath = Qt.point(mapX, mapY)
                    return
                }

                if (dstPath == null) {
                    dstPath = Qt.point(mapX, mapY)
                    tempObj.destroy()
                }
                component = Qt.createComponent("Path.qml")
                obj = component.createObject(mapView.contentItem, { "x1": srcPath.x, "y1": srcPath.y, "x2": dstPath.x, "y2": dstPath.y})
                srcPath = dstPath
                dstPath = null
                qmlItemCreated(obj)
            }
            else if (drawingType == "intruder") {
                if (tempObj)
                    tempObj.destroy()
                component = Qt.createComponent("Intruder.qml")
                obj = component.createObject(mapView.contentItem, { "x": mapX - 10, "y": mapY - 10})
                enabled = false
                hoverEnabled = false
                drawingType = ""
                qmlItemCreated(obj)

            }
            else if (drawingType == "sensor") {

                if (tempObj)
                    tempObj.destroy()
                component = Qt.createComponent(sensorType + ".qml")
                if (sensorType == "SEISMIC") {
                    obj = component.createObject(mapView.contentItem, { "objectName": "::SensorSEISMIC-"+sensoridcount, "sensorId": sensoridcount,"x": mapX - 50, "y": mapY - 50})
                }
                else if (sensorType == "PIR") {
                    if (numberOfClicks == 0) {
                        numberOfClicks = 1
                        clickedMouseX = mapX
                        clickedMouseY = mapY
                        return
                    }
                    obj = component.createObject(mapView.contentItem, { "objectName": "::SensorPIR-"+sensoridcount, "sensorId": sensoridcount, "x": clickedMouseX, "y": clickedMouseY, "rotation": mapX - clickedMouseX})
                    numberOfClicks = 0
                }
                enabled = false
                hoverEnabled = false
                drawingType = ""
                sensoridcount++
                qmlItemCreated(obj)
            }
            else if (drawingType == "node") {
                if (tempObj)
                    tempObj.destroy()
                component = Qt.createComponent("Node.qml")
                obj = component.createObject(mapView.contentItem, {"objectName": "::Node"+nodeidcount, "nodeId": nodeidcount, "x": mapX, "y": mapY})
                enabled = false
                hoverEnabled = false
                drawingType = ""
                nodeidcount++
                qmlItemCreated(obj)
            }
            else if (drawingType == "area") {
                if (tempObj)
                    tempObj.destroy()
                var diameter = 400
                component = Qt.createComponent("Area.qml")
                obj = component.createObject(mapView.contentItem, { "width": diameter,"height": diameter, "x": mapX - diameter / 2, "y": mapY - diameter / 2})
                enabled = false
                hoverEnabled = false
                drawingType = ""
                qmlItemCreated(obj)
            }
            else if (drawingType == "remove") {
                obj = mapView.contentItem.childAt(mapX, mapY)

                if (editable && (obj.className == "Intruder" || obj.className == "Path")) {
                    obj.destroy()
                }
                else if (deployment && (obj.className == "Sensor" || obj.className == "Node")) {
                    remove(obj)
                }
                enabled = false
                drawingType = ""
            }
            else if (drawingType == "car") {
                if (tempObj)
                    tempObj.destroy()
                component = Qt.createComponent("Intruder.qml")
                obj = component.createObject(mapView.contentItem, { "skin": "qrc:/icons/car-black.png", "width": 32, "height": 16, "speed": 50, "x": mapX - 10, "y": mapY - 10 })
                enabled = false
                hoverEnabled = false
                drawingType = ""
                qmlItemCreated(obj)
            }
        }
    }

}
