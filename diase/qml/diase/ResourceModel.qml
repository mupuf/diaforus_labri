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

ListModel {
    signal update(variant index, variant keys, variant values)

    function appendResource(nodeId, resourceName, key, value) {
        append( { "NodeId": nodeId, "ResourceName": resourceName, "Keyz": key, "Values": value, "Index": count })
    }

    function setDelegatedPluginPath(index, path) {
        setProperty(index, "delegatedPlugin", path)
    }

    function getDelegatedPluginPath(index) {
        return get(index).delegatedPlugin
    }

    function updateResource(index, key, value) {
        setProperty(index, "Keyz", get(index).Keyz + "," + key)
        setProperty(index, "Values", get(index).Values + "|" + value)
        update(index, get(index).Keyz, get(index).Values)
    }

    function getKeys(index) {
        return get(index).Keyz
    }

    function getValues(index) {
        return get(index).Values
    }
}
