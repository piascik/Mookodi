"""Driver implementation for the auxiliary systems.

The auxiliary PLC controls motors to move the XY-slides and covers of the
telescope.
"""

import ctypes
import logging
import socket
import sys

from pymodbus.client.sync import ModbusTcpClient
from pymodbus.payload import BinaryPayloadBuilder, BinaryPayloadDecoder
from pymodbus.exceptions import ConnectionException
from lesedi.sdk import ttypes
from lesedi.lib import driver


logger = logging.getLogger(__name__)


class Status(driver.Bitfield):

    _anonymous_ = ('fields',)

    class Fields(ctypes.LittleEndianStructure):
        _fields_ = [
            ('mirror_cover1_closed', ctypes.c_uint8, 1),
            ('mirror_cover1_open', ctypes.c_uint8, 1),
            ('mirror_cover2_closed', ctypes.c_uint8, 1),
            ('mirror_cover2_open', ctypes.c_uint8, 1),
            ('baffle_closed', ctypes.c_uint8, 1),
            ('baffle_open', ctypes.c_uint8, 1),
            ('moving', ctypes.c_uint8, 1),
        ]
    _fields_ = [
        ('fields', Fields),
        ('value', ctypes.c_uint, 7)
    ]


class Driver(object):

    PORT = 502

    COMMAND_ADDRESS = 110

    COVERS_STATUS_ADDRESS = 130
    COVERS_STATUS_LENGTH = 1

    def __init__(self, host, port=PORT):
        self.host = host
        self.port = port

    def get_status(self):
        try:
            with ModbusTcpClient(self.host, self.port) as client:
                result = client.read_holding_registers(
                    self.COVERS_STATUS_ADDRESS, self.COVERS_STATUS_LENGTH)
        except ConnectionException as e:
            raise ttypes.LesediException(str(e))

        if result.isError():
            raise ttypes.LesediException(str(result))

        status = BinaryPayloadDecoder.fromRegisters(
            result.registers, byteorder='>', wordorder='<').decode_16bit_uint()

        return Status(status)

    def _write(self, value):

        builder = BinaryPayloadBuilder(byteorder='>', wordorder='<')
        builder.add_16bit_uint(value)
        payload = builder.to_registers()

        try:
            with ModbusTcpClient(self.host, self.port) as client:
                result = client.write_registers(self.COMMAND_ADDRESS, payload)
        except ConnectionException as e:
            raise ttypes.LesediException(str(e))

        if result.isError():
            raise ttypes.LesediException(str(result))

    def open_covers(self):
        """Opens the mirror covers and baffle in order."""

        # Set the first bit in the command register to one.
        self._write(1)

    def close_covers(self):
        """Closes the mirror covers and baffle in order."""

        # Set the first bit in the command register to zero.
        self._write(0)


def main():
    from pymodbus.server.asynchronous import StartTcpServer
    from pymodbus.datastore import (
        ModbusServerContext,
        ModbusSlaveContext,
        ModbusSparseDataBlock
    )

    logging.basicConfig(level=logging.DEBUG)

    class CallbackDataBlock(ModbusSparseDataBlock):

        def setValues(self, address, values):
            super(CallbackDataBlock, self).setValues(address, values)

            # Set cover status based on the incoming command value.
            if values == [1]:
                s = Status(
                    mirror_cover1_open=1,
                    mirror_cover2_open=1,
                    baffle_open=1
                )
            elif values == [0]:
                s = Status(
                    mirror_cover1_closed=1,
                    mirror_cover2_closed=1,
                    baffle_closed=1
                )

            super(CallbackDataBlock, self).setValues(131, [s.value])

    store = ModbusSlaveContext(
        hr=CallbackDataBlock.create(),  # holding registers
    )

    context = ModbusServerContext(slaves=store, single=True)

    StartTcpServer(context)


if __name__ == '__main__':
    main()
