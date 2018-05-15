#!/usr/bin/env python3

import socket
import struct

class EV3():
    def __init__(self, host: str):
        self._socket = socket.socket(
            socket.AF_BLUETOOTH,
            socket.SOCK_STREAM,
            socket.BTPROTO_RFCOMM
        )
        self._socket.connect((host, 1))

    def __del__(self):
        if isinstance(self._socket, socket.socket):
            self._socket.close()

    def send_direct_cmd(self, ops: bytes, local_mem: int=0, global_mem: int=0) -> bytes:
        cmd = b''.join([
            struct.pack('<h', len(ops) + 5),
            struct.pack('<h', 42),
            DIRECT_COMMAND_REPLY,
            struct.pack('<h', local_mem*1024 + global_mem),
            ops
        ])
        self._socket.send(cmd)
        print_hex('Sent', cmd)
        reply = self._socket.recv(5 + global_mem)
        print_hex('Recv', reply)
        return reply

def print_hex(desc: str, data: bytes) -> None:
    print(desc + ' 0x|' + ':'.join('{:02X}'.format(byte) for byte in data) + '|')

DIRECT_COMMAND_REPLY = b'\x00'
opNop = b'\x01'
my_ev3 = EV3('00:16:53:3F:71:F0')
ops_nothing = opNop
my_ev3.send_direct_cmd(ops_nothing)

