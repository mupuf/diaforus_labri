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
    var k = keys.split(",")
    var v = values.split("|")
    var last_timestamp = 0
    var last_event = 0
    var id = 0

    var array = k[k.length - 1].split(INTERNAL_MODEL_SEPARATOR)
    for (var i = 0; i < array.length; i++) {
	var timestamp = parseInt(array[i], 10)
	if (timestamp > last_timestamp) {
	    last_timestamp = timestamp
	    id = i
	}
    }
    array = v[v.length - 1].split(INTERNAL_MODEL_SEPARATOR)
    last_event = array[id]
    
    return "last timestamp: " + last_timestamp + "\n" +
	"last event: " + last_event
}

// Constructor for this plugin
function initContentGraph(graph)
{
    // Create a Bar named "event"
    graph.newBar("event")
    // Change the label for y axis to "Percentage"
    graph.setYAxisLabel("Percentage")
}

// This function is called periodically each time the associated graph must be updated
function updateContentGraph(graph, index, keys, values)
{
    var k = keys.split(",")
    var v = values.split("|")
    var critical_levels = ""

   // Add x values
   graph.addKeys("event", k[k.length - 1])
    
    var list = v[v.length - 1].split(":")
    for (var i = 0; i < 16; i++) {
	var level = parseInt(list[i])

	if (level & 0x1) {
	    level = level >> 1;
	    level = level + 1
	    level = level * 100/32
	    level = level + 100
	} else {
	    level = level >> 1;
	}
	
	if (i != 15)
	    critical_levels = critical_levels + level + ':'
	else
	    critical_levels = critical_levels + level
    }
    // Add y values
    graph.addValues("event", critical_levels)
    // Redraw the whole graph
    graph.redraw()
}
