#! /usr/bin/python
#
# Dispatcher protocol:
#
# Every packet is preceeded by its size in network order on two bytes
# (even the NODE_ID sending which is then preceeded by \0 \2).
#
# Upon connection:
#   SOCKET_KIND: 8 bits (DATA_SOCKET, DATA_ACK_SOCKET, INCOMING_STIMULI_SOCKET)
#   NODE_ID: 16 bits unless this is an INCOMING_STIMULI_SOCKET
#
# Incoming packet structure:
#   TARGET: 16 bits
#   PAYLOAD: PAYLOAD_SIZE bytes
#
# Outgoing packet structure:
#   PACKET_TYPE:  DATA(0), ACK(1), NACK(2) or STIMULUS(3) on 8 bits
#   PAYLOAD: PAYLOAD_SIZE bytes
# In the case of an ACK or a NACK, the payload is 16 bits wide and
# contains the node_id. In the case of a STIMULUS, the payload contains
# the sensor_id (8 bits) and its value (8 bits).
#
# Incoming stimuli structure:
#  TARGET: 16 bits
#  SENSOR: 8 bits
#  VALUE: 8 bits
#
# Outgoing stimuli structure (in packet):
#  SENSOR: 8 bits
#  VALUE: 8 bits

import datetime, os, select, socket, struct, sys, xml.dom.minidom, time

BROADCAST_ADDRESS = 2 ** 16 - 1

DATA = 0
ACK = 1
NACK = 2
STIMULUS = 3
COAP = 4

DATA_SOCKET = 0
DATA_ACK_SOCKET = 1
INCOMING_STIMULI_SOCKET = 2
INCOMING_COAP_REQ_SOCKET = 3

# sockets on which we send data
nodes_out_data = {}
# sockets on which we receive data and send acks
nodes_in_data_out_ack = {}
# incoming stimuli sockets
incoming_stimuli = []
# incoming coap sockets
incoming_coap_req = []
initial_stimulus_time = 0

class MsgEvent:
    __time__ = 0
    __node_id__ = ""
    __msg_length__ = 0
    __msg__ = ""
    def __init__(self, timestamp, node_id, msg, verbose):
        self.__time__ = timestamp
        self.__node_id__ = node_id
        self.__msg_length = len(msg)
        if (verbose == True):
            self.__msg__ = msg
    def time(self):
        return self.__time__
    def nodeID(self):
        return self.__node_id__
    def msgLength(self):
        return self.__msg_length__
    def msg(self):
        return self.__msg__

class TimestampedValue:
    __time__ = 0
    __value__ = 0
    def __init__(self, time, value):
        self.__time__ = time
        self__value__ = value
    def time(self):
        return self.__time__
    def value(self):
        return self.__value__

class LoggedAttribute:
    __attribute_name__ = ""
    __values__ = []
    __start_time__ = 0
    def __init__(self, attribute_name, start_time = datetime.datetime.now()):
        self.__attribute_name__ = attribute_name
        self.__start_time__ = start_time
    def name(self):
        return self.__attribute_name__
    def startTime(self):
        return self.__start_time__
    def addValue(self, value):
        self.__values__.append(TimestampedValue(datetime.datetime.now() - self.startTime(), value))
    def value(self):
        return self.__values__[-1].value()
    def values(self):
        return self.__values__

class Node:
    # physical properties
    __transmission_rate__ = 9600 # bauds/s

    # Attributes
    __node_name__ = ""
    __help_text__ = ""
    __start_time__ = datetime.datetime.now()
    __volume_in__ = LoggedAttribute("Volume In", __start_time__)
    __volume_out__ = LoggedAttribute("Volume Out", __start_time__)
    __start_up_energy__ = 0
    __transmission_consumption__ = LoggedAttribute("Transmission Consumption", __start_time__)
    def __init__(self, name, help_text, start_up_energy):
        self.__node_name__ = name
        self.__help_text__ = help_text
        self.__start_up_energy__ = start_up_energy
    def newEmission(self, msg):
        msg_len = len(msg)
        self.__volume_out__.addValue(msg_len)
    def newReception(self, msg):
        msg_len = len(msg)
        self.__volume_in__.addValue(msg_len)

class MsgHistory:
    __events__ = []
    __verbose__ = False
    __output_file__ = 0
    __start_time__ = 0
    def __init__(self):
        self.__start_time__ = datetime.datetime.now()
        #self.__output_file__ = open("logs/network_trace_%s" % datetime.datetime.now(), "w")
        try:
            os.mkdir("logs")
        except OSError:
            pass
        self.__output_file__ = open("logs/test_log", "w")
        self.__output_file__.write("timestamp, type, node_id, msg_length, cost\n")
        self.__output_file__.flush()
    def msgSend(self, node_id, msg):
        #event = MsgEvent(datetime.datetime.now() - self.__start_time__, node_id, msg, self.__verbose__)
        #self.__events__.insert(-1, event)
        self.__output_file__.write("%s, send, %s, %i, %f\n" % (datetime.datetime.now() - self.__start_time__, node_id, len(msg), 0.0))
        self.__output_file__.flush()
    def msgRecv(self, node_id, msg):
        #event = MsgEvent(datetime.datetime.now() - self.__start_time__, node_id, msg, self.__verbose__)
        #self.__events__.insert(-1, event)
        self.__output_file__.write("%s, recv, %s, %i, %f\n" % (datetime.datetime.now() - self.__start_time__, node_id, len(msg), 0.0))
        self.__output_file__.flush()
    def isVerbose(self):
        return self.__verbose__
    def setVerbose(self, verbose):
        self.__verbose__ = verbose

msg_history = MsgHistory()

class CommunicationError(Exception): pass

#
# get the text of a node in the DOM
#
def getText(nodelist):
    rc = ""
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc = rc + node.data
    return rc

#
# function to check the reachability of target from sender, using the geographical
# position and the emission range from the xml file the is specified in the command
# line parameter.
#
def check_range(sender, target, nodes):
    sender_node = None;
    target_node = None;
    for node in nodes:
        node_id = int(node.getAttribute("id"))
        if node_id == sender:
            sender_node = node
        if node_id == target:
            target_node = node
        if target_node != None and sender_node != None:
            break
    if sender_node == None or target_node == None:
        return False
    sender_position = sender_node.getElementsByTagName("phy")[0]
    sender_x = int(sender_position.getAttribute("x"))
    sender_y = int(sender_position.getAttribute("y"))
    sender_range = int(sender_position.getAttribute("range"))
    target_position = target_node.getElementsByTagName("phy")[0]
    target_x = int(target_position.getAttribute("x"))
    target_y = int(target_position.getAttribute("y"))
    return (sender_x - target_x)*(sender_x - target_x) + (sender_y - target_y)*(sender_y - target_y) < sender_range * sender_range

def log(s):
    sys.stderr.write(s + "\n")

def reverse_lookup(hash, val):
    for (k, v) in hash.items():
        if v == val:
            return k
    raise CommunicationError

def register_node_id(node_id, s, socket_kind):
    if socket_kind == DATA_SOCKET:
        nodes_out_data[node_id] = s
    elif socket_kind == INCOMING_STIMULI_SOCKET:
        incoming_stimuli.append(s)
    elif socket_kind == INCOMING_COAP_REQ_SOCKET:
        incoming_coap_req.append(s)
    else:
        nodes_in_data_out_ack[node_id] = s

def unregister_node_id(node_id):
    try:
        s = nodes_out_data[node_id]
        del nodes_out_data[node_id]
        s.shutdown(socket.SHUT_RDWR)
    except:
        pass
    try:
        s = nodes_in_data_out_ack[node_id]
        del nodes_in_data_out_ack[node_id]
        s.shutdown(socket.SHUT_RDWR)
    except:
        pass

def send_data(node_id, payload, sender_id, nodes, packet_type = DATA):
    if node_id == BROADCAST_ADDRESS:
        for target in nodes_out_data.keys():
            if target != sender_id and check_range(sender_id, target, nodes):
                send_data(target, payload, sender_id, nodes)
    else:

        try:
            if packet_type == DATA and check_range(sender_id, node_id, nodes):
                log("Sending data to %(dst)d from %(src)d" % {'dst': node_id, 'src': sender_id})
                packet = struct.pack('!B', packet_type) + payload
                try:
                    msg_history.msgRecv(node_id, packet)
                except Exception, e:
                    print "Erreur : %s" % e
                send_packet(nodes_out_data[node_id], packet)
            elif packet_type != DATA:
                send_packet(nodes_in_data_out_ack[node_id], struct.pack('!B', packet_type) + payload)
        except KeyError:
            log("Trying to send to unknown node %d" % node_id)
            raise CommunicationError
        except CommunicationError:
            log("Connection broken while sending to node %d" % node_id)

def recv_all(s, length):
    r = b''
    while len(r) < length:
        try:
            data = s.recv(length - len(r))
        except:
            raise CommunicationError
        if not data:
            raise CommunicationError
        # Print what is received in hex for debugging
        i = 0
        tmp = b''
        while i < len(data):
            tmp += "%02x " % struct.unpack('!B',data[i])[0]
            i = i + 1
        log("rcvd : %s" % tmp)
        r += data
    return r

def recv_packet(s):
    length = struct.unpack('!H', recv_all(s, 2))[0]
    return recv_all(s, length)

def send_packet(s, payload):
    try:
        s.send(struct.pack('!H', len(payload)) + payload)
    except socket.error:
        raise CommunicationError

def send_stimulus(node_id, sensor, data):
    send_data(node_id, struct.pack('BB', sensor, data), None, None, STIMULUS)

def send_coap(node_id, data):
    send_data(node_id, data, None, None, COAP)

def recv_data(s,sender_id,nodes):
    pkt = recv_packet(s)
    target = struct.unpack('!H', pkt[:2])[0]
    payload = pkt[2:]
    try:
        msg_history.msgSend(sender_id, pkt)
    except Exception, e:
        print "Erreur : %s" % e
    log("Received a packet of size %d for %x" % (len(payload), target))
    ack_type = ACK
    try:
        send_data(target, payload, sender_id, nodes)
    except CommunicationError:
        print "Will send NACK"
        ack_type = NACK
    # Send a ACK or NACK in case of a unicast packet
    if target != BROADCAST_ADDRESS:
        send_data(sender_id, struct.pack('!H', target), sender_id, nodes, ack_type)

def recv_stimulus(s):
    try:
        return struct.unpack('!HBB', recv_packet(s))
    except struct.error:
        log("Incorrect incoming stimulus")
        raise CommunicationError

def dump_stimulus(fileName, node_id, sensor, value):
        type_table = { 0: "PIR", 1: "SPIRIT", 2: "SEISMIC" }
        f = open(fileName, "a")
        diffTime = time.time() - initial_stimulus_time
        f.write("%d, %d, %s, %d, %d\n" % (int(diffTime), node_id, type_table[(sensor >> 4)], (sensor & 0xf), value))
        f.close()

def mainloop(listening, rlist, nodes):
    global initial_stimulus_time
    while True:
        (rready, _, _) = select.select(rlist, [], [])
        for fd in rready:
            if fd == listening:
                log("Incoming connection")
                (newsock, sender) = fd.accept()
                try:
                    socket_type = struct.unpack('!B', recv_packet(newsock))[0]
                    if socket_type not in [ INCOMING_STIMULI_SOCKET, INCOMING_COAP_REQ_SOCKET ]:
                        node_id = struct.unpack('!H', recv_packet(newsock))[0]
                    else:
                        node_id = None
                except CommunicationError:
                    log("New node aborted early")
                    continue
                if node_id:
                    log("New node id is %x registering socket kind %x" % (node_id, socket_type))
                else:
                    log("Registering socket kind %x" % socket_type)
                register_node_id(node_id, newsock, socket_type)
                # Only listen to incoming data sockets and stimuli sockets
                if socket_type in [DATA_ACK_SOCKET, INCOMING_STIMULI_SOCKET, INCOMING_COAP_REQ_SOCKET]:
                    rlist.append(newsock)
            elif fd in incoming_stimuli:
                try:
                    node_id, sensor, value = recv_stimulus(fd)
                    log("Stimulus for node %x: sensor %x => %x" % (node_id, sensor, value))
                    if initial_stimulus_time == 0:
                        initial_stimulus_time = time.time() - 1
                    dump_stimulus("dumped_stimulus.log", node_id, sensor, value)
                    try:
                        send_stimulus(node_id, sensor, value)
                    except CommunicationError:
                        log("Communication error when sending stimulus to node %x - Unregistering" % node_id)
                        unregister_node_id(node_id)
                except CommunicationError:
                    log("Communication error from stimuli socket - Unregistering")
                    rlist.remove(fd)
                    incoming_stimuli.remove(fd)
            elif fd in incoming_coap_req:
                data = fd.recv(2)

                if data == '':
                    rlist.remove(fd)
                    incoming_coap_req.remove(fd)
                    continue
                node_id = struct.unpack('!H', data)[0]
                pkt = fd.recv(1024)
                log("COAP request for node %x" % node_id)
                send_coap(node_id, pkt)

            else:
                try:
                    id = reverse_lookup(nodes_in_data_out_ack, fd)
                except:
                    log("Exception on reverse_lookup")
                    continue
                log("Receiving data from node %x" % id)
                try:
                    recv_data(fd,id,nodes)
                except CommunicationError:
                    log("Communication error from node %x - Unregistering" %
                        id)
                    rlist.remove(fd)
                    unregister_node_id(id)

def usage():
    sys.stderr.write("Usage: %s port input_xml\n" % sys.argv[0])
    sys.exit(1)

def main():
    if len(sys.argv) != 3:
        usage()
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('', int(sys.argv[1])))
    s.listen(5)
    rlist = [s]
    file = sys.argv[2]
    dom = xml.dom.minidom.parse(file)
    nodes = dom.getElementsByTagName("node")
    mainloop(s, rlist, nodes)

if __name__ == '__main__':
    main()
