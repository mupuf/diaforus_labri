#! /usr/bin/python
#
# Usage:
#
#   inject-stimulus [host:]port target sensor value
# OR
#   inject-stimulus [host:]port input_csv
#

import dispatcher, socket, struct, sys, time

def usage():
    sys.stderr.write('Usage: %s [host:]port [input_csv] [target sensor value]\n' % sys.argv[0])
    sys.exit(1)

def connect(hostport, socket_type):
    c = hostport.split(':')
    if len(c) == 1:
        host = c[0]
    else:
        host = 'localhost'
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, int(c[-1])))
    s.send(struct.pack('!HB', 1, socket_type))
    return s

def send_stimulus(sock, target, sensor, value):
    payload = struct.pack('!HHBB', 4, target, sensor, value)
    sock.send(payload)


def send_coap_request(sock, node, url, payload):
    # Build the COAP packet
    pkt = [0x51] # V = 1, T = Non-confirmable, and OC = 1
    pkt += [0x02] # method code is POST
    pkt += [0x0, 0x0] # transaction ID
    
    if len(url) < 15:
        pkt += [0x90 + len(url)]
        pkt+= bytearray(url)
    else:
        pkt += [0x9f]
        pkt += [len(url) - 15]
        pkt += bytearray(url)
    pkt += bytearray(payload)
    
    # Send it to the dispatcher
    sock.send(struct.pack('!H', node))
    sock.send(struct.pack('B'*len(pkt), *pkt))

def parse_csv(input_file):
    f = open(input_file, 'r')
    outgoing_stimulus = {}
    
    for line in f.readlines():
        if line[0] == '#' or line[0] == '\n':
            continue
        values = line.replace(' ', '').replace('\n', '').split(',')
        if not outgoing_stimulus.has_key(float(values[0])):
            outgoing_stimulus[float(values[0])] = list()
        outgoing_stimulus[float(values[0])].append(values[1:])
    f.close()
    return outgoing_stimulus

def send_csv_stimulus(stimuli_sock, coap_sock, outgoing_stimulus):
    types = { "PIR" : 0, "SPIRIT" : 1, "SEISMIC" : 2, "COAP" : 3 }
    initial_time = time.time()
    timestamps = sorted(outgoing_stimulus)
    index = 0
    
    while index < len(timestamps):
        curr_time = time.time()
        timestp = timestamps[index]

        if (curr_time - initial_time) >= timestp:
            for values in outgoing_stimulus[timestp]:
                print timestp, "s : send stimuli", values
                if values[1] == "COAP":
                    if len(values) > 4:
                        payload=''
                        for v in values[3:]:
                            payload += (v + ':')
                        send_coap_request(coap_sock, int(values[0]), values[2], payload.rstrip(':'))
                    else:
                        send_coap_request(coap_sock, int(values[0]), values[2], values[3])
                else:
                    sensor = types[ values[1] ] << 4 | int(values[2])
                    if values[1] == "SPIRIT":
                        value = int(values[3])  << 4 | int(values[4])
                    else:
                        value = int(values[3])
                    send_stimulus(stimuli_sock, int(values[0]), sensor, value)
            index += 1

def main():
    if len(sys.argv) == 3:
        outgoing_stimulus = parse_csv(sys.argv[2])
        send_csv_stimulus(connect(sys.argv[1], dispatcher.INCOMING_STIMULI_SOCKET), connect(sys.argv[1], dispatcher.INCOMING_COAP_REQ_SOCKET), outgoing_stimulus)
    else:
        if len(sys.argv) != 5:
            usage()
        send_stimulus(connect(sys.argv[1], dispatcher.INCOMING_STIMULI_SOCKET),
                      int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]))

if __name__ == '__main__':
    main()
