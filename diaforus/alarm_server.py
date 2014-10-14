#! /usr/bin/python

import os, select, socket, struct, sys

ANYIPv6 = "::"
C2_LISTEN_PORT = 4242
PUBSUB_LISTEN_PORT = 61616
BROKER_IP = "1180:0000:0000:0000:1063:09ff:fe30:"
SUBSCRIBER_ID = 2
THIS_NODE = 32769 # This is "8001" in decimal

SUBSCRIPTION_ID = 0
L3ACK_TIMEOUT = 5
RELIABLE_PUBSUB_SUPPORT = True

tcp_sock = None
udp_sock = None
tcp_clients = []

class xSimplePredicate():
    OPERATOR_EQ=0
    OPERATOR_NE=1
    OPERATOR_GT=2
    OPERATOR_LT=3
    OPERATOR_GE=4
    OPERATOR_LE=5
    OPERATOR_EX=6
    ATTRIBUTE_NAME_LENGTH=7
    
    def __init__(self, attribute, operator, conditionValue):
        self.attribute = attribute
        self.operator = operator
        self.conditionValue = conditionValue
    
    def serialize(self):
        i = 0
        pkt = []
        while i < len(self.attribute):
            pkt += [ord(self.attribute[i])]
            i = i + 1
        i = len(self.attribute)
        while i < xSimplePredicate.ATTRIBUTE_NAME_LENGTH:
            pkt += [0x00]
            i = i + 1
        pkt += [self.operator]
        pkt += [(self.conditionValue >> 24) & 0xff, (self.conditionValue >> 16) & 0xff, (self.conditionValue >> 8) & 0xff, self.conditionValue & 0xff]
        return pkt

class xMsg():
    MSGTYPE_SUBSCRIBE = 4
    MSGTYPE_NOTIFY = 5
    MSGTYPE_ACK = 10
    
    def __init__(self, payload, msgType = 0, fromNetwork=0):
        if fromNetwork:
            pkt = []
            i = 0
            while i < len(payload):
                pkt += struct.unpack("B", payload[i])
                i = i + 1
            self._msgType = pkt[0]
            self._payloadSize = pkt[1]
            self._genericPayload = pkt[2:]
            
        else:
            self._msgType = msgType
            self._payloadSize = len(payload)
            self._genericPayload = payload
        
    def type(self):
        return self._msgType
    
    def payload(self):
        return self._genericPayload
    
    def payloadSize(self):
        return self._payloadSize
    
    def serialize(self):
        pkt  = [self._msgType]
        pkt += [self._payloadSize]
        return pkt

class xAckMsg():
    MSGTYPE_SUBSCRIBE = 4
    MSGTYPE_NOTIFY = 5
    
    def __init__(self, genericMsg, msgType = 0, id1 = 0, id2 = 0, id3 = 0, idn = 0):
        if not genericMsg is None: 
            payload = genericMsg.payload()
            self.__msgType = payload[0]
            self.__idn = payload[1:2]
            self.__id1 = payload[3]
            self.__id2 = payload[4]
            self.__id3 = payload[5]
        else:
            self.__msgType = msgType
            self.__idn = idn
            self.__id1 = id1
            self.__id2 = id2
            self.__id3 = id3
    
    def msgType(self):
        return self.__msgType
    
    def idn(self):
        return self.__idn
    
    def id1(self):
        return self.__id1
    
    def id2(self):
        return self.__id2
    
    def id3(self):
        return self.__id3
    
    def serialize(self):
        pkt  = [self.__msgType]
        pkt += [self.__idn, 0x00]
        pkt += [self.__id1, self.__id2, self.__id3]
        return struct.pack("B"*len(pkt), *pkt)

class xSubMsg():
    def __init__(self, subscriberId, subscriptionId, predicates):
        self.__subscriberId = subscriberId
        self.__subscriptionId = subscriptionId
        self.__numPredicates = len(predicates)
        self.__predicates = predicates
        self.__acked = False

    def serialize(self):
        tmp = []
        for p in self.__predicates:
            tmp += p.serialize()
        payload  = struct.pack("HBB", self.__subscriberId, self.__subscriptionId, self.__numPredicates)
        payload += struct.pack("B"*len(tmp), *tmp)
        return payload

    def predicates(self):
        return self.__predicates
    
    def subscriptionId(self):
        return self.__subscriptionId
    
    def isAcked(self):
        return self.__acked
    
    def ack(self):
        self.__acked = True

class xNotifyMsg():
    def __init__(self, genericMsg):
        payload = genericMsg.payload()
        self.__numAttributes = payload[0]
        self.__publicationId = payload[1]
        self.__valuesId = payload[2]
        self.__toAck = payload[3]
        self.__publisherId = payload[4]
        self.__subscriptionId = payload[6]
        self.__subIndex = payload[7]
        self.__attributes = []

        # Parsing attributes's name
        attribute_name = ''
        for byte in payload[8:(8 + self.__numAttributes * 7)]:
            if byte == 0:
                if len(attribute_name) == 0:
                    continue
                self.__attributes.append(attribute_name)
                attribute_name = ''
                continue
            attribute_name += chr(byte)
        
        # Parsing attributes's values
        i = 8 + self.__numAttributes * 7
        tmp = ''
        self.__values = []
        valueLen = 0
        for byte in payload[i:]:
            tmp += chr(byte)
            valueLen = valueLen + 1
            if valueLen == 4:
                value = struct.unpack("I", tmp)[0]
                self.__values.append(value)
                tmp = ''
                valueLen = 0
    
    def attributes(self):
        return self.__attributes
    
    def values(self):
        return self.__values
    
    def toAck(self):
        return self.__toAck
    
    def ack(self):
        self.__toAck = False
    
    def subIndex(self):
        return self.__subIndex
    
    def publicationId(self):
        return self.__publicationId
    
    def valuesId(self):
        return self.__valuesId
    
    def publisherId(self):
        return self.__publisherId

def debug(s):
    if os.getenv("ALARM_SERVER_DEBUG") != None:
        sys.stderr.write(s)

def debug_payload(payload):
    tmp = print_to_buffer(payload)
    debug("payload %s\n" % tmp)

def print_flush(msg):
    sys.stdout.write(msg)
    sys.stdout.flush()

def udp_listen():
    global udp_sock
    udp_sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    udp_sock.bind((ANYIPv6, PUBSUB_LISTEN_PORT))

def tcp_listen():
    global tcp_sock
    tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    tcp_sock.bind(('', C2_LISTEN_PORT))
    tcp_sock.listen(5)

def alloc_free_subscription_id():
    global SUBSCRIPTION_ID
    id = SUBSCRIPTION_ID
    SUBSCRIPTION_ID = SUBSCRIPTION_ID + 1
    return id

def print_to_buffer(pkt):
	tmp = ""
	i = 0
	while i < len(pkt):
		tmp += "%02x " % struct.unpack('!B', pkt[i])
		i = i + 1
	return tmp

def sendSubscription(subMsg):
    msg = []
    for p in subMsg.predicates():
        msg += p.serialize()
    genericMsg = xMsg(msg, xMsg.MSGTYPE_SUBSCRIBE)
    xmsg = genericMsg.serialize()

    payload  = struct.pack("B"*len(xmsg), *xmsg)
    payload += subMsg.serialize()

    debug_payload(payload)
    udp_sock.sendto(payload, (BROKER_IP, PUBSUB_LISTEN_PORT))

def serialize_subscription(predicates):
    subscriberId = THIS_NODE
    subscriptionId = alloc_free_subscription_id()
    numPredicates=len(predicates)

    SubMsg = []
    for p in predicates:
        SubMsg += p.serialize()
    genericMsg = xMsg(SubMsg, xMsg.MSGTYPE_SUBSCRIBE)
    xmsg = genericMsg.serialize()
    xsubmsg = xSubMsg(subscriberId, subscriptionId, predicates)
    
    payload  = struct.pack("B"*len(xmsg), *xmsg)
    payload += xsubmsg.serialize()

    print_flush("Sending subscription %d to the broker\n" % subscriptionId)
    debug_payload(payload)
    
    return (xsubmsg, payload)
    
def subscribe_and_register_client(sock, alarm_predicates, fail_predicates):
    
    alarmSubMsg, payload = serialize_subscription(alarm_predicates)
    udp_sock.sendto(payload, (BROKER_IP, PUBSUB_LISTEN_PORT))

    failSubMsg, payload = serialize_subscription(fail_predicates)
    udp_sock.sendto(payload, (BROKER_IP, PUBSUB_LISTEN_PORT))

    tcp_clients.append((sock, alarmSubMsg, failSubMsg))

def mainloop(tcp_listen, udp_listen):
    rlist = [tcp_listen, udp_listen]
    print_flush("Waiting for client\n")
    while True:
        if RELIABLE_PUBSUB_SUPPORT:
            (rready, _, _) = select.select(rlist, [], [], L3ACK_TIMEOUT)
        
            if len(rready) == 0: # L3ACK_TIMEOUT expired
                for pair in tcp_clients:
                    subMsg = pair[1]
                    if not subMsg.isAcked():
                        print_flush("L3 ACK timeout expired for alarm subscription %d, re-sending the subscription\n" % subMsg.subscriptionId())
                        sendSubscription(subMsg)
                    subMsg = pair[2]
                    if not subMsg.isAcked():
                        print_flush("L3 ACK timeout expired for failing subscription %d, re-sending the subscription\n" % subMsg.subscriptionId())
                        sendSubscription(subMsg)
        else:
            (rready, _, _) = select.select(rlist, [], [])
        for fd in rready:
            if fd == tcp_listen:
                (newsock, sender) = fd.accept()
                print_flush("Incoming client %s\n" % str(sender))
                alarm_predicates=[]
                alarm_predicates.append(xSimplePredicate("AlmLvl", xSimplePredicate.OPERATOR_GE, 0))
                alarm_predicates.append(xSimplePredicate("AlmTsp", xSimplePredicate.OPERATOR_GE, 0))
                alarm_predicates.append(xSimplePredicate("AlmAr", xSimplePredicate.OPERATOR_GE, 0))
                alarm_predicates.append(xSimplePredicate("AlmLst", xSimplePredicate.OPERATOR_GE, 0))
                alarm_predicates.append(xSimplePredicate("AlmDrt", xSimplePredicate.OPERATOR_GE, 0))
                
                fail_predicates=[]
                fail_predicates.append(xSimplePredicate("FAIL", xSimplePredicate.OPERATOR_GT, 0))
                fail_predicates.append(xSimplePredicate("NODEID", xSimplePredicate.OPERATOR_GE, 0))

                subscribe_and_register_client(newsock, alarm_predicates, fail_predicates)
            
            elif fd == udp_listen:
                data, addr = fd.recvfrom(1024)
                print_flush("Received message from broker %s\n" % str(addr))
                debug_payload(data)
                
                msg = xMsg(data, 0, 1)
                if msg.type() == xMsg.MSGTYPE_NOTIFY:
                    notifyMsg = xNotifyMsg(msg)
                    if len(tcp_clients):
                        print_flush("Sending notify message to IPv4 world\n")
                        if notifyMsg.attributes()[0] == "AlmLvl":
                            AlmLst = notifyMsg.values()[3]
                            AlmDrt = notifyMsg.values()[4]
                            payload = ''
                            nodesPayload = ''
                            
                            debug("AlmLst %d\n" % AlmLst)
                            debug("AlmDrt %d\n" % AlmDrt)

                            payload += struct.pack("B", 1)
                            for i in range(0, 32):
                                if (AlmLst >> i) & 0x1:
                                    nodesPayload += struct.pack("!H", i)
                            debug("nodes len %d\n" % len(nodesPayload))
                            payload += struct.pack("B", len(nodesPayload) / 2)
                            payload += nodesPayload
                            payload += struct.pack("!I", AlmDrt)
                        
                            for pair in tcp_clients:
                                debug("Sending alarm\n")
                                sock = pair[0]
                                sock.send(payload)
                        elif notifyMsg.attributes()[0] == "FAIL":
                            payload  = ''
                            payload += struct.pack("B", 2)
                            payload += struct.pack("!H", notifyMsg.values()[1])

                            for pair in tcp_clients:
                                debug("Sending failing\n")
                                debug("payload: \n")
                                debug_payload(payload)
                                sock = pair[0]
                                sock.send(payload)
                            
                        if notifyMsg.toAck():
                            print_flush("This notification is critical and requires L3 ACK\n")
                            ackMsg = xAckMsg(None, xAckMsg.MSGTYPE_NOTIFY, notifyMsg.subIndex(), notifyMsg.publicationId(), notifyMsg.valuesId(), notifyMsg.publisherId())
                            genericMsg = xMsg(ackMsg.serialize(), xMsg.MSGTYPE_ACK)
                            xmsg = genericMsg.serialize()
                            payload   = struct.pack("B"*len(xmsg), *xmsg)
                            payload  += ackMsg.serialize()
                            debug("Sending xAckMsg id1 %(id1)s, id2 %(id2)s, id3 %(id3)s, idn %(idn)s\n" % { "id1": ackMsg.id1(), "id2" : ackMsg.id2(), "id3": ackMsg.id3(), "idn" : ackMsg.idn() })
                            debug_payload(payload)
                            udp_sock.sendto(payload, (BROKER_IP, PUBSUB_LISTEN_PORT))
                            notifyMsg.ack()
                elif msg.type() == xMsg.MSGTYPE_ACK:
                    debug_payload(data[5:])
                    ackMsg = xAckMsg(msg)
                    
                    if ackMsg.msgType() == xAckMsg.MSGTYPE_SUBSCRIBE:
                        for pair in tcp_clients:
                            subMsg = pair[1]
                            if subMsg.subscriptionId() == ackMsg.id1():
                                print_flush("L3 ACK for alarm subscription %d\n" % ackMsg.id1())
                                subMsg.ack()
                            subMsg = pair[2]
                            if subMsg.subscriptionId() == ackMsg.id1():
                                print_flush("L3 ACK for failing subscription %d\n" % ackMsg.id1())
                                subMsg.ack()
            
            elif fd in tcp_clients:
                data = fd.recv(1024)
                if data == '':
                    print_flush("Connection closed with client\n")
                    tcp_clients.remove(fd)


def usage():
    sys.stderr.write("Usage: %s port broker_id [--no-reliable-pubsub]\n" % sys.argv[0])
    sys.exit(1)

def main():
    global C2_LISTEN_PORT, RELIABLE_PUBSUB_SUPPORT, BROKER_IP

    if len(sys.argv) < 3:
        usage()
    if len(sys.argv) >= 4 and sys.argv[3] == "--no-reliable-pubsub":
        RELIABLE_PUBSUB_SUPPORT = False
    C2_LISTEN_PORT = int(sys.argv[1])
    BROKER_IP = BROKER_IP + sys.argv[2]
    tcp_listen()
    udp_listen()

    
    mainloop(tcp_sock, udp_sock)

if __name__ == '__main__':
    main()
