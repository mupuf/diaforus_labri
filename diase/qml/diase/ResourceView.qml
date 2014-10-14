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

Item {
    property alias model: gridView.model
    property alias count: gridView.count
    property string type: "tile"

    signal removeResource(int index)
    signal updateResource(variant item, int entityIndex)
    signal itemAdded()

    Component {
        id: tile
        ResourceTile {
            width: gridView.cellWidth - 10
            height: gridView.cellHeight - 10
            nodeName: "node " + NodeId
            nodeId: NodeId
            resourceName: ResourceName
            modelEntryIndex: Index

            onExit: removeResource(modelEntryIndex)
        }
    }

    Component {
        id: graph
        BarGraph {
            id: bargraph
            width: gridView.cellWidth - 10
            height: gridView.cellHeight - 10
            nodeId: NodeId
            resourceName: ResourceName
            modelEntryIndex: Index

            MouseArea {
                id: mouseArea
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 50
                anchors.left: parent.left
                anchors.right: parent.right
                onPressAndHold: {
                    close.visible = !close.visible
                }
            }

            Text {
                id: close
                anchors.bottom: parent.top
                anchors.bottomMargin: 10
                anchors.right: parent.right
                height: 10
                width: 10
                text: "+"
                font.pointSize: 12
                rotation: 45
                visible: false
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        mouseArea.enabled = false
                        close.color = "red"
                    }
                    onExited: {
                        mouseArea.enabled = true
                        close.color = "black"
                    }
                    onClicked: {
                        removeResource(modelEntryIndex)
                    }
                }
            }
        }
    }

    GridView {
        id: gridView
        objectName: "gridView"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        cellWidth: computeCellWidth()
        cellHeight: computeCellHeight()
        model: ResourceModel{}
        delegate: tile

        contentItem.onChildrenChanged: itemAdded()

        function computeCellWidth() {
            return (type == "tile") ? 150 : 500
        }

        function computeCellHeight() {
            return (type ==  "tile") ? 200 : 400
        }
    }

    function getItemAt(index) {
        for (var i = 0; i < gridView.contentItem.children.length; i++)
            if (gridView.contentItem.children[i].modelEntryIndex == index)
                return gridView.contentItem.children[i]
        return null
    }

    onTypeChanged: {
        gridView.delegate = (type == "tile") ? tile : graph
        gridView.cellWidth = gridView.computeCellWidth()
        gridView.cellHeight = gridView.computeCellHeight()
    }
}
