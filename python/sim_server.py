# Simple hardware simulation server.
#
# Lists on a socket port and performs the appropriate exchange to implement
# hardware reading and writing.
#
# The simulation we run here is a dumb and basic as it possibly can be.  For a
# more detailed and realistic hardware simulation, use the simulation server
# provided as part of PandaFPGA.

import argparse
import os
import sys
import socket
import struct


parser = argparse.ArgumentParser(description = 'PandA Hardware simulation')
parser.add_argument(
    '-d', '--daemon', action = 'store_true', help = 'Run as daemon process')
parser.add_argument(
    'config_dir',
    help = '''\
Path to configuration directory.
This field is ignored for the basic simulation server, but is provided for
compatibility with the full server in PandaFPGA.''')
args = parser.parse_args()



# We daemonise the server by double forking, but we leave the controlling
# terminal and other file connections alone.
def daemonise():
    if os.fork():
        # Exit first parent
        sys.exit(0)
    # Do second fork to avoid generating zombies
    if os.fork():
        sys.exit(0)


class SocketFail(Exception):
    pass


# Ensures exactly n bytes are read from sock
def read(sock, n):
    result = ''
    while len(result) < n:
        rx = sock.recv(n - len(result))
        if not rx:
            raise SocketFail('End of input')
        result = result + rx
    return result


# This simulation is as dumb as a brick, it merely provides a basic
# implementation of the communication protocol and otherwise does as little as
# possible.
def run_simulation(conn):
    while True:
        command_word = read(conn, 4)
        command, block, num, reg = struct.unpack('cBBB', command_word)
        if command == 'R':
            # Read one register, always returns zero
            conn.sendall(struct.pack('I', 0))
        elif command == 'W':
            # Write one register, we throw the value away
            value, = struct.unpack('I', read(conn, 4))
        elif command == 'T':
            # Write data array to large table, we throw this away
            length, = struct.unpack('I', read(conn, 4))
            data = read(conn, length * 4)
        elif command == 'D':
            # Retrieve increment of data stream: we never send anything!
            length, = struct.unpack('I', read(conn, 4))
            conn.sendall(struct.pack('i', -1))
        else:
            print 'Unexpected command', repr(command_word)
            raise SocketFail('Unexpected command')


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(('localhost', 9999))
sock.listen(0)

print 'Dummy simulating server ready'
if args.daemon:
    daemonise()

(conn, addr) = sock.accept()
sock.close()

conn.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
try:
    run_simulation(conn)
except (SocketFail, KeyboardInterrupt) as e:
    print 'Simulation closed:', repr(e)
