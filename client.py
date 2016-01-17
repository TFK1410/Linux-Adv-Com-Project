#!/usr/bin/env python

import socket
import os

class ServerException(Exception):
    "Raised when server returns error"
    pass

class SConnection(object):
    "Connection to network configuration server"

    def __init__(self, host, port):
        """Connect to network configuration server

        host - Hostname of server, as string
        port - Port, as int
        """
        self.__socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.__socket.connect((host, port))
        self.__socketFile = self.__socket.makefile()

    def close(self):
        "Close the connection"
        self.__socket.close()

    def __sendMessage(self, message):
        self.__socket.send(message + "\n")

    def __recvMessage(self):
        line = self.__socketFile.readline()
        if not line:
            raise ServerException("Unexpected EOF reached when reading from server")
        chunks = line.strip().split("=")
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
        """Get list of all interface configurations

        Configuration of each interface is in same format as returned by getInterfaceConfig
        """
        return self.__getInterfaceConfigs("show")

    def getInterfaceConfig(self, ifname):
        """Get configuration for single interface

        Returns dict with following keys:
            'ifname'  : Name of interface; eg. 'eth0',
            'ipv4'    : IP adress of interface; eg. '10.0.0.1',
            'ipv6'    : IPv6 address; eg. '0000:0000:0000:0000:0000:0000:0000:0000',
            'mac'     : MAC adress; eg. '9a:c3:e6:39:b2:96',
            'netmask' : Network mask; eg '255.255.0.0',
            'status'  : Status of interface; 'UP' or 'DOWN'

        Returns None if theres no interface with given name
        """
        assert " " not in ifname
        l = self.__getInterfaceConfigs("show " + ifname)
        if len(l) == 1:
            return l[0]
        else:
            return None

    def setIpAndNetmask(self, iface, newipwithmask):
        """Set IP address and netmask for interface

        iface - name of interface to configure
        newipwithnetmask - IP with netmask in "a.b.c.d/e" format
        """
        self.__sendMessage("setip " + iface + " " + newipwithmask)
        messageType, messageData = self.__recvMessage()
        self.__raiseIfErrorMessage(messageType, messageData)
        if messageType != "SetIpOkay":
            raise ServerException("Unexpected response from server: " + messageType)

    def setMac(self, iface, mac):
        """Set MAC address and netmask for interface

        iface - name of interface to configure
        mac - New MAC address in "aa:bb:cc:dd:ee:ff" format
        """
        self.__sendMessage("setmac " + iface + " " + mac)
        messageType, messageData = self.__recvMessage()
        self.__raiseIfErrorMessage(messageType, messageData)
        if messageType != "SetMacOkay":
            raise ServerException("Unexpected response from server: " + messageType)

CLI_USAGE = """
Usage: client.py host port [command [iface [argument]]]

If command is omitted it defaults to "show"

Command can be:

  show
    Show information about specified interface
    or all interfaces if iface is omitted

  show iface
    Show information about specified interface

  setip iface a.b.c.d/m
    Set ip for iface to a.b.c.d and network mask to m

  setmac iface aa:bb:cc:dd:ee:ff
    Set MAC for iface to aa:bb:cc:dd:ee:ff


Commands for scripting (output is just value):

  showip iface
    Show just IP Address for iface

  shownetmask iface
    Show just network mask for iface

  showip6 iface
    Show just IPv6 Address for iface

  showmac iface
    Show just MAC Address for iface

  showstatus iface
    Show just status for iface, "UP" or "DOWN"


Note: client.py can be used also as python module
"""

if __name__ == "__main__":
    import sys
    import pprint
    pp = pprint.PrettyPrinter()

    # Check argument count
    if len(sys.argv) < 3:
        print(CLI_USAGE)
        exit(1)

    # Connect to server
    conn = SConnection(sys.argv[1], int(sys.argv[2]))

    # Read command and arguments
    if len(sys.argv) <= 3:
        command = "show"
        args = []
    else:
        command = sys.argv[3]
        args = sys.argv[4:]

    # Execute command
    if command == "show":
        if args:
            pp.pprint(conn.getInterfaceConfig(args[0]))
        else:
            pp.pprint(conn.getAllInterfaceConfigs())
    elif command == "setip":
        conn.setIpAndNetmask(args[0], args[1])
    elif command == "setmac":
        conn.setMac(args[0], args[1])
    elif command == "showip":
        print(conn.getInterfaceConfig(args[0])['ipv4'])
    elif command == "shownetmask":
        print(conn.getInterfaceConfig(args[0])['netmask'])
    elif command == "showip6":
        print(conn.getInterfaceConfig(args[0])['ipv6'])
    elif command == "showmac":
        print(conn.getInterfaceConfig(args[0])['mac'])
    elif command == "showmac":
        print(conn.getInterfaceConfig(args[0])['mac'])
    elif command == "showstatus":
        print(conn.getInterfaceConfig(args[0])['status'])
    else:
        print("Unrecognized command")
        exit(1)
