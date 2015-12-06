#!/usr/bin/env python

import socket
import sys
import os
import pprint
pp = pprint.PrettyPrinter()

class ServerException(Exception):
    pass

class SConnection(object):
    def __init__(self, host, port):
        self.__socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.__socket.connect((host, port))
        self.__socketFile = self.__socket.makefile()

    def close(self):
        self.__socket.close()

    def __sendMessage(self, message):
        self.__socket.send(message + "\n")

    def __recvMessage(self):
        chunks = self.__socketFile.readline().strip().split("=")
        messageType = chunks[0]
        messageData = dict(zip(chunks[1::2], chunks[2::2]))
        return (messageType, messageData)

    def __recvList(self):
        messages = []
        while True:
            msg = self.__recvMessage()
            if msg[0] == "EndOfList":
                break
            messages.append(msg)
        return messages

    def __raiseIfErrorMessage(self, messageType, messageData):
        if messageType == "Error":
            msg = "Server returned error"
            if "name" in messageData:
                msg += ": name=" + messageData["name"]
            if "errno" in messageData:
                errno = messageData["errno"]
                msg += " errno=" + errno + " (" + os.strerror(int(errno)) + ")"
            raise ServerException(msg)

    def __getInterfaceConfigs(self, cmd):
        self.__sendMessage(cmd)
        return [x[1] for x in self.__recvList() if x[0] == "IfInfo"]

    def getAllInterfaceConfigs(self):
        return self.__getInterfaceConfigs("show")

    def getInterfaceConfig(self, ifname):
        assert " " not in ifname
        l = self.__getInterfaceConfigs("show " + ifname)
        if len(l) == 1:
            return l[0]
        else:
            return None

    def setIpAndNetmask(self, iface, newipwithmask):
        self.__sendMessage("setip " + iface + " " + newipwithmask)
        messageType, messageData = self.__recvMessage()
        self.__raiseIfErrorMessage(messageType, messageData)
        if messageType != "SetIpOkay":
            raise ServerException("Unexpected response from server: " + messageType)

    def setMac(self, iface, mac):
        self.__sendMessage("setmac " + iface + " " + mac)
        messageType, messageData = self.__recvMessage()
        self.__raiseIfErrorMessage(messageType, messageData)
        if messageType != "SetMacOkay":
            raise ServerException("Unexpected response from server: " + messageType)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: client.py host port [command interface [arg]]")
        exit()
    conn = SConnection(sys.argv[1], int(sys.argv[2]))
    if len(sys.argv) == 3:
        pp.pprint(conn.getAllInterfaceConfigs())
    elif sys.argv[3] == "show":
        pp.pprint(conn.getInterfaceConfig(sys.argv[4]))
    elif sys.argv[3] == "setip":
        conn.setIpAndNetmask(sys.argv[4], sys.argv[5])
    elif sys.argv[3] == "setmac":
        conn.setMac(sys.argv[4], sys.argv[5])
