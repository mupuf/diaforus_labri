#! /usr/bin/python

import select, socket, struct, sys, xml.dom.minidom, time, os 
import datetime, subprocess, shutil, stat, getopt, signal
from subprocess import CalledProcessError

area_node_id = dict()
node_id_list = set()
subprocesses = list()
compilation_errors = list()

def printError(msg):
    print '\033[93m' + "\n" + msg + '\033[0m'

def gen_warning(file, filename, brief):
    file.write("/**\n")
    file.write(" * \\file " + filename + "\n")
    file.write(" * \\brief " + brief + "\n")
    file.write(" *\n")
    file.write(" * NOTE: This file is autogenerated by the script build_network.py.\n")
    file.write(" */\n\n")


def gen_common_config(dom, node):
    network = dom.getElementsByTagName("network")
    build = dom.getElementsByTagName("build")[0]
    is_simulation = node.getAttribute("simulation") == "true"
    node_id = int(node.getAttribute("id"))
    area = int(node.getAttribute("area"))
    energy = node.getElementsByTagName("energy")[0]
    energy_startup = int(energy.getAttribute("startup"))
    energy_alarm = int(energy.getAttribute("alarm"))
    debug = build.getElementsByTagName("debug")[0]
    failure_handling = network[0].getAttribute("failure_handling") == "true"
    debug_wavesim = debug.getAttribute("wavesim") == "true"
    debug_sicslowpan = debug.getAttribute("sicslowpan") == "true"
    debug_uip = debug.getAttribute("uip") == "true"
    debug_rpl = debug.getAttribute("rpl") == "true"
    debug_pubsub = debug.getAttribute("pubsub") == "true"
    debug_coap = debug.getAttribute("coap") == "true"
    debug_reasoning = debug.getAttribute("reasoning") == "true"
    filename = "common_config.h"
    # Check the higher bits of node_id
    if (area_node_id.has_key(area)):
        if (area_node_id[area] != (node_id >> 8)):
            printError("Error: The node " + str(node_id) + "'s MSB (" + str(node_id >> 8) + ") don't match the area's one (" + str(area_node_id[area]) + "). Abort!\n")
            sys.exit(-2)
    else:
        area_node_id[area] = (node_id >> 8)
    # Check the unicity of the node ID
    if (node_id in node_id_list):
        printError("NodeID " + str(node_id) + " already exists. Abort!\n")
        sys.exit(-3)
    node_id_list.add(node_id)
    # Write the output
    try:
        common_file = open("config/"+filename,'w')
    except:
        printError("The file 'config/"+filename+"' cannot be written\n")
        return -1
    gen_warning(common_file, filename, "General-purpose configuration file")
    common_file.write("#ifndef _COMMON_CONFIG_H_\n")
    common_file.write("#define _COMMON_CONFIG_H_\n\n")
    
    common_file.write("	/** \\brief Simulated run. */\n")
    common_file.write("	#define IS_SIMU " + ("1" if is_simulation else "0") + "\n\n")

    common_file.write("	/** \\brief Node identifier. */\n")
    common_file.write("	#define NODE_ID " + str(node_id) + "\n\n")

    common_file.write("	/** \\brief Area identifier */\n")
    common_file.write("	#define AREA_ID " + str(area) + "\n\n")

    common_file.write("	/** \\brief Energy settings. */\n")
    common_file.write("	#define ENERGY_STARTUP " + str(energy_startup) + "\n")
    common_file.write("	#define ENERGY_ALARM " + str(energy_alarm) + "\n\n")

    common_file.write("	/** \\brief Failure handling */\n")
    common_file.write("	#define FAILURE_HANDLING " +("1" if failure_handling else "0") + "\n\n")

    common_file.write("	/** \\brief Modules debugging */\n")
    common_file.write("	#define DEBUG_WAVESIM " + ("1" if debug_wavesim else "0") + "\n")
    common_file.write("	#define DEBUG_SICSLOWPAN " + ("1" if debug_sicslowpan else "0") + "\n")
    common_file.write("	#define DEBUG_UIP " + ("1" if debug_uip else "0") + "\n")
    common_file.write("	#define DEBUG_RPL " + ("1" if debug_rpl else "0") + "\n")
    common_file.write("	#define DEBUG_PUBSUB " + ("1" if debug_pubsub else "0") + "\n")
    common_file.write("	#define DEBUG_COAP " + ("1" if debug_coap else "0") + "\n")
    common_file.write("	#define DEBUG_REASONING " + ("1" if debug_reasoning else "0") + "\n\n")
    common_file.write("#endif\n")
    common_file.close()
    return 0

def gen_network_config(dom, node):
    network = dom.getElementsByTagName("network")
    net = node.getElementsByTagName("rpl")
    pubsub = node.getElementsByTagName("pubsub")
    coap = node.getElementsByTagName("coap")
    gateway = node.getElementsByTagName("gateway")
    filename = "network_config.h"
    
    if (len(net) == 1):
        prefix = net[0].getAttribute("prefix")
        is_root = len(prefix) > 0
    else:
        is_root = False
        prefix = ""
    
    if (len(pubsub) == 1):
        pubsub_active = pubsub[0].getAttribute("enable") == "true"
        pubsub_reliable = network[0].getAttribute("pubsub_reliable") == "true"
        is_broker = pubsub[0].getAttribute("broker") == "true"
    else:
        pubsub_active = False
        pubsub_reliable = False
        is_broker = False

    if (len(coap) == 1):
        coap_active = coap[0].getAttribute("enable") == "true"
    else:
        coap_active = False

    if (len(gateway) == 1):
        is_gateway = True
        port = gateway[0].getAttribute("port")
    else:
        is_gateway = False
        port = 0
    
    try:
        network_file = open("config/"+filename,'w')
    except:
        printError("The file 'config/"+filename+"' cannot be written\n")
        return -1
    
    gen_warning(network_file, filename, "Network configuration file")
    network_file.write("#ifndef _NETWORK_CONFIG_H_\n")
    network_file.write("#define _NETWORK_CONFIG_H_\n")
    
    network_file.write("	/** \\brief This node is the RPL DODAG root. */\n")
    network_file.write("	#define NET_IS_ROOT " + ("1" if is_root else "0") + "\n\n")
    
    network_file.write("	/** \\brief RPL DODAG prefix. */\n")
    if (len(prefix) > 0):
        network_file.write("	#define NET_RPL_PREFIX \"" + prefix + "\"\n\n")
    else:
        network_file.write("	#define NET_RPL_PREFIX NULL\n\n")

    network_file.write("	/** \\brief Middleware active or not */\n")
    network_file.write("	#define PUBSUB_ACTIVE " + ("1" if pubsub_active else "0") + "\n")
    network_file.write("	#define COAP_ACTIVE " + ("1" if coap_active else "0") + "\n")
    network_file.write("	#define ROLE_BROKER " + ("1" if is_broker else "0") + "\n\n")

    network_file.write("	/** \\brief Middleware messages acked */\n")
    network_file.write("	#define RELIABLE_PUBSUB " + ("1" if pubsub_reliable else "0") + "\n\n")

    network_file.write("	/** \\brief If the broker is not the RPL root in the initial configuration (before possible failures):\n")
    network_file.write("	 *  there is the need of the broker discovery process; otherwise the DIO message carries the root\n")
    network_file.write("	 *  (and so the broker) IP address (DODAG_ID field).\n")
    network_file.write("	 */\n")
    network_file.write("#if NET_IS_ROOT && ROLE_BROKER\n")
    network_file.write("	#define BROKER_NOT_ROOT 0\n")
    network_file.write("#elif ROLE_BROKER\n")
    network_file.write("	#define BROKER_NOT_ROOT 1\n")
    network_file.write("#endif\n\n")

    network_file.write("	#define ROLE_GATEWAY " + ("1" if is_gateway else "0") + "\n")
    if (is_gateway and port > 0):
        network_file.write("	#define GATEWAY_PORT " + str(port) + "\n")
    
    network_file.write("#endif\n")
    network_file.close()
    return 0

def gen_rpl_neighbor_config(dom, node):
    network = dom.getElementsByTagName("network")
    net = node.getElementsByTagName("rpl")
    neighbors = node.getElementsByTagName("neighbor")
    neighbors_count = len(neighbors)
    filename = "rpl_neighbor_config.h"

    try:
        config_file = open("config/"+filename,'w')
    except:
        printError("The file 'config/"+filename+"' cannot be written\n")
        return -1

    gen_warning(config_file, filename, "RPL neighbors-restriction configuration file")
    config_file.write("#ifndef _RPL_NEIGHBOR_CONFIG_H_\n")
    config_file.write("#define _RPL_NEIGHBOR_CONFIG_H_\n")

    neighbor_list = ""
    for i in range(0, neighbors_count):
        neighbor_list += neighbors[i].getAttribute("node_id")
        if i < neighbors_count - 1:
            neighbor_list += ","
    config_file.write("	#define NEIGHBOR_LIST \"" + neighbor_list +"\"\n")

    config_file.write("	#define RPL_RESTRICT_NEIGHBORS_COUNT " + str(neighbors_count) + "\n")
    """config_file.write("	node_id_t rpl_restrict_neighbors[] = {\n		")
    for n in neighbors:
        config_file.write(n.getAttribute("node_id") + ", ")
    config_file.write("\n	};\n")"""

    config_file.write("#endif\n\n")

    config_file.close()
    return 0

def gen_sensors_config(dom, node):
    sensors = node.getElementsByTagName("sensors")[0]
    sensors_count = len(sensors.getElementsByTagName("sensor"))
    filename = "sensors_config.h"
    try:
        sensors_file = open("config/"+filename,'w')
    except:
        printError("The file 'config/"+filename+"' cannot be written\n")
        return -1
    gen_warning(sensors_file, filename, "Sensors configuration file")
    sensors_file.write("#ifndef _SENSORS_ROLES_H_\n")
    sensors_file.write("#define _SENSORS_ROLES_H_\n")
    sensors_file.write("\n#include \"sensors.h\"\n")
    sensors_file.write("\n#define SENSOR_COUNT " + str(sensors_count) + "\n")
    sensors_file.write("\nsensor_t sensors[SENSOR_COUNT] __attribute__ (( section (\".slowdata\") )) = {\n")
    modalities = sensors.getElementsByTagName("modality")
    for modality in modalities:
        mod_text = modality.getAttribute("type") + "_MOD"
        mod_sensors = modality.getElementsByTagName("sensor")
        for mod_sensor in mod_sensors:
            sens_id = int(mod_sensor.getAttribute("id"))
            value_type = mod_sensor.getAttribute("type")
            period = int(mod_sensor.getAttribute("period"))
            delay = int(mod_sensor.getAttribute("delay"))
            if (mod_sensor.hasAttribute("reemission_delay")):
                reemission_delay = int(mod_sensor.getAttribute("reemission_delay"))
            else:
                reemission_delay = 0
            sensors_file.write("	{\n")
            sensors_file.write("		.modality = " + mod_text + ",\n")
            sensors_file.write("		.id = " + str(sens_id) + ",\n")
            #connectivity
            i2c = mod_sensor.getElementsByTagName("i2c")
            gpio = mod_sensor.getElementsByTagName("gpio")
            adc = mod_sensor.getElementsByTagName("adc")
            _file = mod_sensor.getElementsByTagName("file")
            if (len(i2c) == 1):
                address = i2c[0].getAttribute("address")
                register = i2c[0].getAttribute("register")
                sensors_file.write("		.connection = CONNECT_I2C,\n")
                sensors_file.write("		.port.i2c.reg = " + register + ",\n")
                sensors_file.write("		.port.i2c.address = " + address + ",\n")
            elif (len(gpio) == 1):
                pin = gpio[0].getAttribute("pin")
                sensors_file.write("		.connection = CONNECT_GPIO,\n")
                sensors_file.write("		.port.gpio = " + pin + ",\n")
            elif (len(adc) == 1):
                pin = adc[0].getAttribute("pin")
                sensors_file.write("		.connection = CONNECT_ADC,\n")
                sensors_file.write("		.port.adc = " + pin + ",\n")
            sensors_file.write("		.periodicity = " + str(period) + ",\n")
            sensors_file.write("		.reemission_delay = " + str(reemission_delay) + ",\n")
            sensors_file.write("		.next_read = " + str(delay) + ",\n")
            sensors_file.write("		.history = { {0}, 0, 0 },\n")
            sensors_file.write("		.stimulus = {0},\n")
            sensors_file.write("		.old_variation = 0,\n")
            #value
            value = mod_sensor.getElementsByTagName("value")[0]
            if (value_type == "digital"):
                value_type_s = "DIGITAL_VALUE"
            elif (value_type == "analog"):
                value_type_s = "ANALOG_VALUE"
            else:
                value_type_s = "UNKNOWN_TYPE"
            sensors_file.write("		.value_type = " + value_type_s + ",\n")
            if (value_type == "digital"):
                invert = value.getAttribute("invert")
                invert_s = "false"
                if (len(invert) > 0 and invert == "true"):
                    invert_s = "true"
                sensors_file.write("		.calibration.invert = " + invert_s + ",\n")
            elif (value_type == "analog"):
                offset = value.getAttribute("offset")
                mult = value.getAttribute("mult")
                div = value.getAttribute("div")
                if (len(offset) == 0):
                    offset = 0
                if (len(mult) == 0):
                    mult = 1
                if (len(div) == 0):
                    div = 1
                offset = int(offset)
                mult = int(mult)
                div = int(div)
                sensors_file.write("		.calibration.analog.offset = " + str(offset) + ",\n")
                sensors_file.write("		.calibration.analog.mult = " + str(mult) + ",\n")
                sensors_file.write("		.calibration.analog.div = " + str(div) + ",\n")
            abs_threshold = value.getAttribute("abs_threshold")
            rel_threshold = value.getAttribute("rel_threshold")
            sensors_file.write("		.abs_threshold = " + abs_threshold + ",\n")
            sensors_file.write("		.rel_threshold = " + rel_threshold + ",\n")
            sensors_file.write("		.normalized_value = 0,\n")
            sensors_file.write("		.last_alert = 0,\n")
            sensors_file.write("	},\n")
    sensors_file.write("};\n")
    
    sensors_file.write("static inline void sensors_init()\n");
    sensors_file.write("{\n");
    sensors_num = 0

    for modality in modalities:
	mod_text = modality.getAttribute("type") + "_MOD"
        mod_sensors = modality.getElementsByTagName("sensor")
        for mod_sensor in mod_sensors:
            sens_id = int(mod_sensor.getAttribute("id"))
            value_type = mod_sensor.getAttribute("type")
            period = int(mod_sensor.getAttribute("period"))
            delay = int(mod_sensor.getAttribute("delay"))
            if (mod_sensor.hasAttribute("reemission_delay")):
                reemission_delay = int(mod_sensor.getAttribute("reemission_delay"))
            else:
                reemission_delay = 0
            sensors_file.write("\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].modality = " + mod_text + ";\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].id = " + str(sens_id) + ";\n")
            #connectivity
            i2c = mod_sensor.getElementsByTagName("i2c")
            gpio = mod_sensor.getElementsByTagName("gpio")
            adc = mod_sensor.getElementsByTagName("adc")
            _file = mod_sensor.getElementsByTagName("file")
            if (len(i2c) == 1):
                address = i2c[0].getAttribute("address")
                register = i2c[0].getAttribute("register")
                sensors_file.write("	sensors["+str(sensors_num)+"].connection = CONNECT_I2C;\n")
                sensors_file.write("	sensors["+str(sensors_num)+"].port.i2c.reg = " + register + ";\n")
                sensors_file.write("	sensors["+str(sensors_num)+"].port.i2c.address = " + address + ";\n")
            elif (len(gpio) == 1):
                pin = gpio[0].getAttribute("pin")
                sensors_file.write("	sensors["+str(sensors_num)+"].connection = CONNECT_GPIO;\n")
                sensors_file.write("	sensors["+str(sensors_num)+"].port.gpio = " + pin + ";\n")
            elif (len(adc) == 1):
                pin = adc[0].getAttribute("pin")
                sensors_file.write("	sensors["+str(sensors_num)+"].connection = CONNECT_ADC;\n")
                sensors_file.write("	sensors["+str(sensors_num)+"].port.adc = " + pin + ";\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].periodicity = " + str(period) + ";\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].reemission_delay = " + str(reemission_delay) + ";\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].next_read = " + str(delay) + ";\n")
            sensors_file.write("	memset(sensors["+str(sensors_num)+"].history.history, 0, 64);\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].history.start = 0;\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].history.end = 0;\n")
            sensors_file.write("	memset(sensors["+str(sensors_num)+"].stimulus, 0, 15 * sizeof(value_t));\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].old_variation = 0;\n")
            #value
            value = mod_sensor.getElementsByTagName("value")[0]
            if (value_type == "digital"):
                value_type_s = "DIGITAL_VALUE"
            elif (value_type == "analog"):
                value_type_s = "ANALOG_VALUE"
            else:
                value_type_s = "UNKNOWN_TYPE"
            sensors_file.write("	sensors["+str(sensors_num)+"].value_type = " + value_type_s + ";\n")
            if (value_type == "digital"):
                invert = value.getAttribute("invert")
                invert_s = "false"
                if (len(invert) > 0 and invert == "true"):
                    invert_s = "true"
                sensors_file.write("	sensors["+str(sensors_num)+"].calibration.invert = " + invert_s + ";\n")
            elif (value_type == "analog"):
                offset = value.getAttribute("offset")
                mult = value.getAttribute("mult")
                div = value.getAttribute("div")
                if (len(offset) == 0):
                    offset = 0
                if (len(mult) == 0):
                    mult = 1
                if (len(div) == 0):
                    div = 1
                offset = int(offset)
                mult = int(mult)
                div = int(div)
                sensors_file.write("	sensors["+str(sensors_num)+"].calibration.analog.offset = " + str(offset) + ";\n")
                sensors_file.write("	sensors["+str(sensors_num)+"].calibration.analog.mult = " + str(mult) + ";\n")
                sensors_file.write("	sensors["+str(sensors_num)+"].calibration.analog.div = " + str(div) + ";\n")
            abs_threshold = value.getAttribute("abs_threshold")
            rel_threshold = value.getAttribute("rel_threshold")
            sensors_file.write("	sensors["+str(sensors_num)+"].abs_threshold = " + abs_threshold + ";\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].rel_threshold = " + rel_threshold + ";\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].normalized_value = 0;\n")
            sensors_file.write("	sensors["+str(sensors_num)+"].last_alert = 0;\n")
            sensors_num += 1
    sensors_file.write("}\n");
    sensors_file.write("#endif\n")
    sensors_file.close()
    return 0

def gen_reasoning_config(dom, node):
    reasoning = node.getElementsByTagName("reasoning")
    reasoning_node = node.getElementsByTagName("reasoning_node")
    log_analyse_period = "1440"
    latencies = { "white" : "150", "green" : "100", "yellow" : "75", "orange" : "50", "red" : "25"}
    filename = "reasoning_config.h"
    if (len(reasoning) == 1):
        min_intrusion_duration = reasoning[0].getAttribute("min_intrusion_duration")
        max_intrusion_duration = reasoning[0].getAttribute("max_intrusion_duration")
        latency_mode = reasoning[0].getAttribute("latency_mode")
        
        if len(reasoning_node) == 1:
            is_reasoning = True
            log_analyse_period = reasoning_node[0].getAttribute("log_analyse_period")
        else:
            is_reasoning = False
    else:
        is_reasoning = False
        average_intrusion_duration = "60"
        latency_mode = "green"
        log_analyse_period = "1440"
    
    try:
        reasoning_file = open("config/"+filename,'w')
    except:
        printError("The file 'config/"+filename+"' cannot be written\n")
        return -1
    gen_warning(reasoning_file, filename, "Reasoning configuration file")
    reasoning_file.write("#ifndef _REASONING_CONFIG_H_\n")
    reasoning_file.write("#define _REASONING_CONFIG_H_\n\n")

    reasoning_file.write("	#define ROLE_REASONING " + ("1" if is_reasoning else "0") + "\n")

    reasoning_file.write("	#define SENSOR_HISTORY_SIZE 64\n")
    reasoning_file.write("	#define SIMULATION_ENTRY_MAX 50\n")
    reasoning_file.write("	#define ALERT_HISTORY_SIZE 16\n")
    reasoning_file.write("	#define SUSPICIOUS_STATE_HISTORY_SIZE 16\n")
    reasoning_file.write("	#define SENSOR_CONTRIBUTION_HISTORY_SIZE 48\n")
    reasoning_file.write("	#define CRITICALITY_THRESHOLD 5\n")
    
    reasoning_file.write("	#define MIN_INTRUSION_DURATION (" + min_intrusion_duration + "*1000)" + "\n")
    reasoning_file.write("	#define MAX_INTRUSION_DURATION (" + max_intrusion_duration + "*1000)" + "\n")
    reasoning_file.write("	#define HISTORY_ANALYZE_PERIOD (" + log_analyse_period + "*60*1000" + ")\n")
    reasoning_file.write("	/* 4 levels : green (1), yellow (2), orange (3) and red (4) */\n")
    reasoning_file.write("	#define LATENCY_MODE " + latencies[latency_mode] + "\n")
    reasoning_file.write("#endif\n")
    reasoning_file.close()
    return 0

def gen_monitored_areas_config(dom, node):
    monitored_areas = node.getElementsByTagName("monitored_area")
    filename = "monitored_areas_config.h"
    try:
        monitored_area_file = open("config/"+filename,'w')
    except:
        printError("The file 'config/"+filename+"' cannot be written\n")
        return -1
    gen_warning(monitored_area_file, filename, "Monitored-area configuration file (may need to be merged with reasoning_config.h)")
    monitored_area_file.write("#ifndef _MONITORED_AREAS_CONFIG_H_\n")
    monitored_area_file.write("#define _MONITORED_AREAS_CONFIG_H_\n\n")
    monitored_area_file.write("#include \"monitored_area.h\"\n")
    monitored_area_file.write("#define MONITORED_AREAS_COUNT " + str(len(monitored_areas)) + "\n")
    monitored_area_file.write("monitored_area_t monitored_areas[] = {\n")
    
    if len(monitored_areas):
        area_id = monitored_areas[0].getAttribute("area")
        crossing_duration = monitored_areas[0].getAttribute("average_crossing_duration") + "000"
        monitored_area_file.write("		{ .area = " + area_id + ", .crossing_duration = " + crossing_duration + ", .value = 0, .previous_time = 0, .time = 0 }\n")
        for area in monitored_areas[1:]:
            area_id = area.getAttribute("area")
            crossing_duration = area.getAttribute("average_crossing_duration")
            monitored_area_file.write("		,{ .area = " + area_id + ", .crossing_duration = " + crossing_duration + ", .value = 0, .previous_time = 0, .time = 0 }")
    monitored_area_file.write("};\n")
    monitored_area_file.write("static inline void monitored_areas_init()\n")
    monitored_area_file.write("{\n");
    area_num = 0
    for area in monitored_areas:
	area_id = area.getAttribute("area")
	crossing_duration = area.getAttribute("average_crossing_duration")
	monitored_area_file.write("monitored_areas["+str(area_num)+"].area = "+area_id+"\n;")
	monitored_area_file.write("monitored_areas["+str(area_num)+"].crossing_duration = "+crossing_duration+"\n;")
	monitored_area_file.write("monitored_areas["+str(area_num)+"].value = 0\n;")
        monitored_area_file.write("monitored_areas["+str(area_num)+"].previous_time = 0\n;")
	monitored_area_file.write("monitored_areas["+str(area_num)+"].time = 0\n\n;")
    
    monitored_area_file.write("}\n");
    monitored_area_file.write("#endif\n")
    monitored_area_file.close()
    return 0

def gen_application_config(dom, node):
    build = dom.getElementsByTagName("build")[0]
    application = build.getElementsByTagName("application")[0]
    application_name = application.getAttribute("name")
    parameters = node.getElementsByTagName("parameter")
    filename = "application_config.h"
    try:
        application_file = open("config/"+filename, 'w')
    except:
        printError("The file 'config/"+filename+"' cannot be written\n")
        return -1
    gen_warning(application_file, filename, "Application configuration file")
    application_file.write("#ifndef _APPLICATION_CONFIG_H_\n")
    application_file.write("#define _APPLICATION_CONFIG_H_\n")
    application_file.write("	#define APPLICATION_" + application_name + "\n\n")
    for param in parameters:
        name = param.getAttribute("name")
        value = param.getAttribute("value")
        if value != "":
            if param.getAttribute("type") == "string":
                value = "\"" + value + "\""
            application_file.write("  #define " + name.upper() + " " + value + "\n")
        else:
            application_file.write("  #define " + name.upper() + "\n")
    application_file.write("#endif\n")
    application_file.close()
    return 0

def compile_node(dom, node, dest_dir, target):
    build = dom.getElementsByTagName("build")[0]
    make = None
    
    for make_node in build.getElementsByTagName("make"):
        if make_node.getAttribute("target") == target:
            make = make_node
    
    make_path = make.getAttribute("path")
    make_args = make.getAttribute("args")
    if (len(make_path) == 0):
        make_path = "make"
    cmd_line = [make_path]
    if (len(make_args) > 0):
        cmd_line += make_args.split(" ")
    build_root = make.getAttribute("root")
    make_output = make.getAttribute("output")
    firmware = node.getElementsByTagName("firmware")[0]
    firmware_name = firmware.getAttribute("name")
    shutil.copytree("config/", "config_save/config." + firmware_name)
    cwd = os.getcwd()
    dest_path = cwd+"/"+dest_dir+"/"+firmware_name
    
    if not os.path.exists(build_root):
        printError("WARNING: build directory " + build_root + " does not exist, ignoring " + dest_path)
        compilation_errors.append(dest_path)
        os.chdir(cwd)
        return
    else:
        os.chdir(build_root)
    try:
        subprocess.check_call(cmd_line + ["clean", "all"])
    except CalledProcessError:
        compilation_errors.append(dest_path)
        os.chdir(cwd)
        return
    shutil.copyfile(make_output, dest_path)
    os.chmod(dest_path, os.stat(make_output)[0])
    os.chdir(cwd)

def build_node(dom, node, dest_dir):
    ret = gen_common_config(dom, node)
    if (ret < 0):
        return ret
    ret = gen_network_config(dom, node)
    if (ret < 0):
        return ret
    ret = gen_reasoning_config(dom, node)
    if (ret < 0):
        return ret
    ret = gen_sensors_config(dom, node)
    if (ret < 0):
        return ret
    ret = gen_monitored_areas_config(dom, node)
    if (ret < 0):
        return ret
    ret = gen_application_config(dom, node)
    if (ret < 0):
        return ret
    ret = gen_rpl_neighbor_config(dom, node)
    if (ret < 0):
        return ret
    target = node.getAttribute("target")
    compile_node(dom, node, dest_dir, target)


def check_dtd(file):
    try:
        subprocess.check_call(["xmllint", "--noout", "--valid", file])
        return
    except OSError:
        printError("WARNING: Cannot validate the XML (xmllint is not accessible). Please install libxml2.\n")
    except CalledProcessError, e:
        printError("WARNING: The network XML is invalid. Please fix the above errors.\n")
    choice = raw_input("Continue anyway (Y/n): ")
    if (choice == "n"):
        sys.exit(-1)

def build_network(file, compile_nodes):
    check_dtd(file)
    dest_dir = os.path.dirname(file)
    dom = xml.dom.minidom.parse(file)
    # make sure config/ exists
    try:
        if (not os.path.isdir("config/")):
            os.mkdir("config/")
    except Exception:
        printError("Error: mkdir(\"config/\") failed\n")
    try:
        shutil.rmtree("config_save/")
    except Exception:
        pass
    # Generate the per-node config
    print "WARNING: Limit compilation to nodes " + str(compile_nodes) + "\n"
    nodes = dom.getElementsByTagName("node")
    for node in nodes:
        if (len(compile_nodes) == 0 or node.getAttribute("id") in compile_nodes):
            build_node(dom, node, dest_dir)
    if len(compilation_errors):
        print "\n========"
        print "The compilation of the following node failed or was ignored:"
        for node in compilation_errors:
            print node
        print "\nPlease selectively compile them using the -n option of build_network.py or check your settings"
        print "========"
    return True

def execute_dispatcher(network_xml, dest_dir, terminal):
    if terminal:
        args = ["xterm", "-e", "/bin/bash", "-c", sys.executable + " dispatcher.py 1234 " + network_xml]
    else:
        args = [sys.executable, "dispatcher.py", "1234", network_xml]
    log = open(dest_dir+"/dispatcher.log", "wb")
    subprocesses.append(subprocess.Popen(args, stdout=log, stderr=log))
    # wait for the service to come up
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    while s.connect_ex(("127.0.0.1", 1234)) != 0:
        time.sleep(0.01)
    s.close()

def execute_node(dom, node, terminal):
    firmware = node.getElementsByTagName("firmware")[0]
    firmare_name = firmware.getAttribute("name")
    log_filename = firmare_name + ".log"
    log = open(log_filename, "wb")
    subprocesses.append(subprocess.Popen("./"+firmare_name, stdout=log, stderr=log))
    if terminal:
        subprocess.Popen(["xterm", "-e", "/bin/bash", "-c", "tail -f " + log_filename, "-T", firmare_name], stdout=None, stderr=None)

def execute_network(file, terminal):
    check_dtd(file)
    dest_dir = os.path.dirname(file)
    dom = xml.dom.minidom.parse(file)
    execute_dispatcher(file, dest_dir, terminal)
    cwd = os.getcwd()
    os.chdir(dest_dir)
    # Launch all the nodes
    nodes = dom.getElementsByTagName("node")
    for node in nodes:
        execute_node(dom, node, terminal)
    print "Wait for suprocesses to terminate"
    try:
        while True:
            pid,exit = os.wait()
            print "Process " + str(pid) + " exited with error code " + str(exit)
    except OSError:
        pass
    print "Done"
    os.chdir(cwd)
    return True

def sig_handler(signum, frame):
    if (signum == signal.SIGINT):
        for p in subprocesses:
            os.kill(p.pid, signal.SIGINT)
        print "Sent the SIGINT signal to " + str(len(subprocesses)) + " subprocesses\n"
        return True

def usage():
    sys.stderr.write("Usage: %s input_xml [-e | --execute-only] [-b | --build-only] [-x | --terminal]\n" % sys.argv[0])
    sys.exit(1)

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hebxn:", ["help", "execute-only", "build-only", "terminal", "compile-nodes"])
    except getopt.GetoptError, err:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    build = True
    execute = True
    terminal = False
    nodes = []
    for o, a in opts:
        if o in ("-e", "--execute-only"):
            build = False
        elif o in ("-b", "--build-only"):
            execute = False
        elif o in ("-x", "--terminal"):
            terminal = True
        elif o in ("-n", "--compile-nodes"):
            nodes = a.split(',')
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        else:
            assert False, "unhandled option"
    if len(args) != 1:
        usage()
    signal.signal(signal.SIGINT, sig_handler)
    file = args[0]
    if build:
        build_network(file, nodes)
    if execute:
        execute_network(file, terminal)

if __name__ == '__main__':
    main()