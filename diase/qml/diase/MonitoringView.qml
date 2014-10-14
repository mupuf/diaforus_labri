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
    id: monitoringView
    width: 800
    height: 600
    color: "#c2bbb8"

    property int currentIndexView: 0
    property alias title: viewTitle.text

    signal nodeClicked(int modelIndex)
    signal coapResourceClicked(string name)
    signal drawingTypeClicked(string name)
    signal viewClicked(int index)
    signal createView()
    signal titleChanged(int index, string newTitle)

    TextEdit {
        id: viewTitle
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Global view"
        font.family: "Helvetica"
        font.pointSize: 20
        focus: false
        smooth: true

        MouseArea {
            anchors.fill: parent
            onPressed: {
                viewTitle.focus = !viewTitle.focus
                if (!viewTitle.focus) {
                    titleChanged(currentIndexView, viewTitle.text)
                }
            }
        }
    }

    ResourceView {
        id: tileGridView
        objectName: "tileGridView"
        type: "tile"
        anchors.top: viewTitle.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.verticalCenter
    }

    ResourceView {
        objectName: "graphGridView"
        type:"graph"
        anchors.top: tileGridView.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    Rectangle {
        id: viewButton
        width: 46
        height: 80
        color: "white"
        border.color: "black"
        border.width: 2
        opacity: 0.9
        radius: 20
        smooth: true

        anchors.horizontalCenter: viewBox.right
        anchors.verticalCenter: viewBox.verticalCenter

        Rectangle {
            id: viewLeftShade
            anchors.fill: parent
            radius: 50
            color: "red"
            smooth: true
            opacity: 0
        }

        MouseArea {
            id: viewLeftMouse
            anchors.fill: parent
            onClicked: viewBox.state = (viewBox.state == "visible") ? "invisible" : "visible"
        }

        states: State {
            when: viewLeftMouse.pressed
            PropertyChanges { target: viewLeftShade; opacity: 0.8}
        }
    }

    Rectangle {
        id: viewBox
        objectName: "viewBox"
        width: 150
        height: 300
        color: "white"
        border.width: 2
        border.color: "black"
        radius: 10
        smooth: true

        anchors.right: parent.left
        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: viewListViewTitle
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            horizontalAlignment: Text.AlignHCenter
            text: "Views"
            font.bold: true
        }

        Text {
            id: createViewItem
            anchors.top: viewListViewTitle.bottom
            anchors.topMargin: 5
            anchors.left: parent.left
            anchors.right: parent.right
            horizontalAlignment: Text.AlignHCenter
            text: "Create view"
            font.italic: true

            Rectangle {
                id: createTileViewTextShape
                color: "cornflowerblue"
                anchors.fill: parent
                opacity: 0.5
                visible: false
            }

            MouseArea {
                anchors.fill: parent
                onPressed: createTileViewTextShape.visible = true
                onReleased: createTileViewTextShape.visible = false
                onClicked: {
                    viewBox.state = (viewBox.state == "visible") ? "invisible" : "visible"
                    monitoringView.createView()
                }
            }
        }

        ListView {
            objectName: "viewsListView"
            anchors.top: createViewItem.bottom
            anchors.topMargin: 5
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            model: ViewModel{}
            delegate: Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: name
                Rectangle {
                    id: viewTextShape
                    color: "cornflowerblue"
                    anchors.fill: parent
                    opacity: 0.5
                    visible: false
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: viewTextShape.visible = true
                    onReleased: viewTextShape.visible = false
                    onClicked: {
                        viewBox.state = (viewBox.state == "visible") ? "invisible" : "visible"
                        viewClicked(index)
                    }
                }
            }
        }

        transitions: [
            Transition {
                from: "*"; to: "visible"
                AnchorAnimation { duration: 800; easing.type: Easing.OutBounce}
            },
            Transition {
                from: "visible"; to: "invisible"
                AnchorAnimation { duration: 150 }
            }
        ]

        states: [
            State {
                name: "visible"
                AnchorChanges { target: viewBox; anchors.right: undefined }
                AnchorChanges { target: viewBox; anchors.left: monitoringView.left }
            },
            State {
                name: "invisible"
                AnchorChanges { target: viewBox; anchors.right: monitoringView.left }
                AnchorChanges { target: viewBox; anchors.left: undefined }
            }
        ]
    }

    Rectangle {
        id: rightButton
        width: 46
        height: 80
        color: "white"
        border.color: "black"
        border.width: 2
        opacity: 0.9
        radius: 20
        smooth: true

        anchors.horizontalCenter: rightBox.left
        anchors.verticalCenter: rightBox.verticalCenter

        Rectangle {
            id: rightShade
            anchors.fill: parent
            radius: 50
            color: "red"
            smooth: true
            opacity: 0
        }

        MouseArea {
            id: rightMouse
            anchors.fill: parent
            onClicked: rightBox.state = (rightBox.state == "visible") ? "invisible" : "visible"
        }

        states: State {
            when: rightMouse.pressed
            PropertyChanges { target: rightShade; opacity: 0.8 }
        }
    }

    Rectangle {
        id: rightBox
        objectName: "rightBox"
        width: 150
        height: 300
        color: "white"
        border.width: 2
        border.color: "black"
        radius: 10
        smooth: true

        anchors.left: parent.right
        anchors.verticalCenter: parent.verticalCenter

        ListView {
            property string type: "node"
            property string lastCoapResource: ""
            property int lastNodeModelIndex: 0

            id: wellKnownListView
            objectName: "wellKnownListView"
            anchors.fill: parent
            model: NodeModel{}
            delegate: Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: name
                Rectangle {
                    id: rightTextShape
                    color: "cornflowerblue"
                    anchors.fill: parent
                    opacity: 0.5
                    visible: false
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: rightTextShape.visible = true
                    onReleased: rightTextShape.visible = false
                    onClicked: {
                        if (wellKnownListView.type == "node") {
                            wellKnownListView.lastNodeModelIndex = modelIndex
                            nodeClicked(modelIndex)
                        }
                        else if (wellKnownListView.type == "wellknown") {
                            wellKnownListView.lastCoapResource = resourceName
                            coapResourceClicked(resourceName)
                        }
                        else if (wellKnownListView.type == "drawing") {
                            rightBox.state = "invisible"
                            drawingTypeClicked(name)
                        }
                    }
                }
            }

            onTypeChanged: {
                if (type == "node")
                    previous.visible = false
                else if (type == "wellknown")
                    previous.visible = true
                else if (type == "drawing")
                    previous.visible = true
            }
        }

        Image {
            id: previous
            source: "qrc:/icons/previous.png"
            anchors.bottom: rightBox.bottom
            anchors.left: rightBox.left
            visible: false

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (wellKnownListView.type == "wellknown")
                        drawingTypeClicked("")
                    else if (wellKnownListView.type == "drawing")
                        nodeClicked(wellKnownListView.lastNodeModelIndex)
                }
            }
        }

        transitions: [
            Transition {
                from: "*"; to: "visible"
                AnchorAnimation { duration: 800; easing.type: Easing.OutBounce}
            },
            Transition {
                from: "visible"; to: "invisible"
                AnchorAnimation { duration: 150 }
            }
        ]

        states: [
            State {
                name: "visible"
                AnchorChanges { target: rightBox; anchors.left: undefined }
                AnchorChanges { target: rightBox; anchors.right: monitoringView.right }
            },
            State {
                name: "invisible"
                AnchorChanges { target: rightBox; anchors.left: monitoringView.right }
                AnchorChanges { target: rightBox; anchors.right: undefined }
            }
        ]
    }

}
