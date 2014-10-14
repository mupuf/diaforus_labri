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

function updateContentText(index, keys, values)
{
    var v = values.split("|")

    var array = v[v.length - 1].split(",")
    return "Threshold: " + array[0] + "\n" +
	"Alarm level: " + array[1]
}

// Constructor for this plugin
function initContentGraph(graph)
{
    // Create a Bar named "alarm"
    graph.newBar("Alarm level")
    graph.newGraph("Threshold", "red")
}

// This function is called periodically each time the associated graph must be updated
function updateContentGraph(graph, index, keys, values)
{
    var k = keys.split(",")
    var v = values.split("|")
    var critical_levels = ""

    graph.addKey("Alarm level", k[k.length-1])

    var array = v[v.length - 1].split(",")
    graph.addValue("Threshold", array[0])
    graph.addValue("Alarm level", array[1])
    graph.redraw()
}
