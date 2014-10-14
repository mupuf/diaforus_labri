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

Flickable {

    property alias source: picture.source

    objectName: "view"
    anchors.fill: parent
    contentWidth: picture.width
    contentHeight: picture.height

    Image {
        id: picture
        x: 0
        y: 0
        asynchronous: true
    }


    transitions: Transition {
        NumberAnimation { properties: "opacity"; duration: 400 }
    }

}
