#!/usr/bin/python3
#
# -*- coding: utf-8 -*-
# Discription: Pure python implementation of STK500v1, made to work with Optiboot only.
# Author:      Mathieu Virbel <mat@meltingrocks.com>
# Source:      https://github.com/tito/sk500
# References:  https://github.com/Optiboot/optiboot/wiki/HowOptibootWorks
#
# Updated for hp85disk project by Mike Gore 2020 and Jay Hamlin
#   * Original Source: https://github.com/tito/stk500/raw/master/hexuploader.py
#   * Changed to support atmega1284p
#   * Jay Hamlin updated support for Python 3
#   * Added Baudrate argument
#   * Added code to send "reset" command to hp85disk firmware to force it into optiboot
#   * Fixed intel 02 segment record calculation, was off by a factor of 16
# ===================
# ## Dependencies
#   * python 3
#   * pip3 install pySerial
# 
# ## Uploading firmware
#   Example: python3 flasher.py 115200 /dev/ttyUSB0 gpib.hex
# 
# ## Listing Serial ports
#   Example: python3 listports.py
# ===================
#
# Author notes
# command ascii range: 0x00-0X7F
# data ascii range: 0x00-0xFF
# 
# Commands implemented in optiboot:
# STK_LOAD_ADDRESS    0x55,'U'    Note: 16bit word address, 128kb flash max.
# STK_PROG_PAGE    0x64,'d'    Flash only
# STK_READ_PAGE    0x74,'t'    Flash only
# STK_READ_SIGN    0x75,'u'    Reads compiled-in signature.
# STK_LEAVE_PROGMODE    0x51,'Q'    Starts application code via WDT reset.
# STK_GET_PARAMETER    0x41,'A'    Supports "minor SW version" and "major SW version" Returns 3 for all other parameters.


# Usage
#    python hexuploader.py baudrate port file

import sys
import serial
import struct
from time import sleep

if len (sys.argv) != 4 :
    print("Usage: " + sys.argv[0] + "baudrate port intel_hex_file")
    sys.exit (1)

STK_LOAD_ADDRESS = 0x55
STK_PROG_PAGE = 0x64
STK_READ_PAGE = 0x74
STK_READ_SIGN = 0x75
STK_LEAVE_PROGMODE = 0x51
STK_GET_PARAMETER = 0x41

# get parameter
STK_SW_MAJOR = 0x81
STK_SW_MINOR = 0x82

STK_GET_SYNC = 0x30

Sync_CRC_EOP = 0x20
Resp_STK_INSYNC = 0x14
Resp_STK_OK = 0x10

# Begin of HP85DISK update for atmega1284p
MEM_PARTS_1284P = {
   "eeprom": {
        "mode": 65,
        "delay": 20,
        # "size": 4,
        "indx": 0,
        "paged": False,
        "size": 4096,
        "pagesize": 8,
        "pagecount": 0,
        "minw": 3600,
        "maxw": 3600,
        "readback1": 0xff,
        "readback2": 0xff
    },
    "flash": {
        "mode": 65,
        "delay": 6,
        # "size": 128,
        "indx": 0,
        "paged": True,
        "size": 131072,
        "pagesize": 256,
        "pagecount": 512,
        "minw": 4500,
        "maxw": 4500,
        "readback1": 0xff,
        "readback2": 0xff
    },
    "lfuse": {
        "mode": 0,
        "delay": 0,
        # "size": 0,
        "indx": 0,
        "paged": False,
        "size": 1,
        "pagesize": 0,
        "pagecount": 0,
        "minw": 4500,
        "maxw": 4500,
        "readback1": 0x00,
        "readback2": 0x00
    },
    "hfuse": {
        "mode": 0,
        "delay": 0,
        # "size": 0,
        "indx": 0,
        "paged": False,
        "size": 1,
        "pagesize": 0,
        "pagecount": 0,
        "minw": 4500,
        "maxw": 4500,
        "readback1": 0x00,
        "readback2": 0x00
    },
    "efuse": {
        "mode": 0,
        "delay": 0,
        # "size": 0,
        "indx": 0,
        "paged": False,
        "size": 1,
        "pagesize": 0,
        "pagecount": 0,
        "minw": 4500,
        "maxw": 4500,
        "readback1": 0x00,
        "readback2": 0x00
    },
    "lock": {
        "mode": 0,
        "delay": 0,
        # "size": 0,
        "indx": 0,
        "paged": False,
        "size": 1,
        "pagesize": 0,
        "pagecount": 0,
        "minw": 4500,
        "maxw": 4500,
        "readback1": 0x00,
        "readback2": 0x00
    },
    "calibration": {
        "mode": 0,
        "delay": 0,
        # "size": 0,
        "indx": 0,
        "paged": False,
        "size": 1,
        "pagesize": 0,
        "pagecount": 0,
        "minw": 0,
        "maxw": 0,
        "readback1": 0x00,
        "readback2": 0x00
    },
    "signature": {
        "mode": 0,
        "delay": 0,
        # "size": 0,
        "indx": 0,
        "paged": False,
        "size": 3,
        "pagesize": 0,
        "pagecount": 0,
        "minw": 0,
        "maxw": 0,
        "readback1": 0x00,
        "readback2": 0x00
    }
}
# End of HP85DISK update for atmega1284p


class Uploader(object):
    def __init__(self, baudrate, device, filename):
        self.baudrate = baudrate
        self.device = device
        self.filename = filename
        self._seq = -1
        super(Uploader, self).__init__()

    def upload(self):
        with open(self.filename, "rb") as fd:
            data = fd.read()

        # check it's an hex file
        assert(len(data) >= 11)
        assert(data[0] == 0x3A)

        # HP85DISK Send reset command
        # Notes: https://pyserial.readthedocs.io/en/latest/pyserial_api.html
        try:
            self.con = serial.Serial(
                self.device,
                self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1,
                xonxoff=0,
                dsrdtr=0,
                rtscts=0)
            self.con.write(b'reset\r')
            self.con.flush()
            # The reset would not work without a close first - tried number of obvious things
            # So we close and reopen - which works for some reason
            self.con.close()
            # sleep(1)
            # End of HP85DISK update
            # STK500v2 open code
            self.con = serial.Serial(
                self.device,
                self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1,
                xonxoff=0,
                dsrdtr=0,
                rtscts=0)

            self.get_sync()
            print("Major: {}".format(self.get_parameter(STK_SW_MAJOR)))
            print("Minor: {}".format(self.get_parameter(STK_SW_MINOR)))
            print("Signature: 0x{:02x}{:02x}{:02x}".format(*self.read_sign()))

            memtype = "flash"
            mem = MEM_PARTS_1284P[memtype]
            assert(mem["paged"] == True)
            assert(mem["pagesize"] != 0)

            # convert hex to binary stored in memory
            buf = bytearray(mem["size"])
            prog_size = self.ihex2b(data, buf)
            print ( "program size: " + str(prog_size) + "\n")

            # flash the device
            assert(mem["pagesize"] * mem["pagecount"] == mem["size"])
            for addr in range(0, mem["size"], mem["pagesize"]):
                if addr > prog_size:
                    break
                page = buf[addr:addr + mem["pagesize"]]
                self.load_addr(addr)
                self.prog_page(memtype, page)

            # # read the flash
            # flashbuf = bytearray(mem["size"])
            # for addr in range(0, mem["size"], mem["pagesize"]):
            #     if addr > prog_size:
            #         break
            #     self.load_addr(addr)
            #     flashbuf[addr:addr + mem["pagesize"]] = self.read_page(mem, page)

            self.leave_progmode()

        finally:
            self.con.close()

    def ihex2b(self, data, buf):
        bufsize = len(buf)
        baseaddr = 0
        maxaddr = 0
        offsetaddr = 0
        nextaddr = 0

        for lineno, line in enumerate(data.splitlines()):
            if not line[0] == (0x3A):
                continue
            rec = self.ihex_readrec(line)
            if not rec:
                print("Error reading hex file")
                return False
            if rec["rectype"] == 0:
                # data record
                nextaddr = rec["loadofs"] + baseaddr
                if (nextaddr + rec["reclen"]) > (bufsize + offsetaddr):
                    print("ERROR: address 0x{:04x} out of range at line {}".format(
                        nextaddr + rec["reclen"], lineno + 1
                    ))
                    return -1
                for i in range(rec["reclen"]):
                    buf[nextaddr + i - offsetaddr] = rec["data"][i]
                if nextaddr + rec["reclen"] > maxaddr:
                    maxaddr = nextaddr + rec["reclen"]

            elif rec["rectype"] == 1:
                # end of file record
                print("MAXADDR: 0x{:06x}, OFFSETADDR: 0x{:06x}, BASEADDR: 0x{:06x}\n".format(maxaddr, offsetaddr, baseaddr) )
                return maxaddr - offsetaddr

            elif rec["rectype"] == 2:
                # extended segment address record
                baseaddr = ((rec["data"][0] << 8 ) | (rec["data"][1])) << 4
                print("MAXADDR: 0x{:06x}, OFFSETADDR: 0x{:06x}, BASEADDR: 0x{:06x}\n".format(maxaddr, offsetaddr, baseaddr) )
                print("data[0]: 0x{:02x}, data[1]: 0x{:02x}\n".format(rec["data"][0], rec["data"][1]) )

            elif rec["rectype"] == 3:
                # start segment record
                pass

            elif rec["rectype"] == 4:
                # extended linear address record
                baseaddr = rec["data"][0] << 8 | rec["data"][1] << 16
                if nextaddr == 0:
                    offsetaddr = baseaddr
            elif rec["rectype"] == 5:
                # start linear address record
                pass

            else:
                raise Exception("Unknown rectype {}".format(ihex["rectype"]))

    def ihex_readrec(self, rec):
        ihex = {
            "reclen": None,
            "loadofs": None,
            "rectype": None,
            "data": bytearray(16), # should be 256 as avrdude define
            "cksum": None
        }
        buf = bytearray(8)
        rlen = len(rec)
        offset = 1
        cksum = 0

        # reclen
        if offset + 2 > rlen:
            return
        ihex["reclen"] = int(rec[offset:offset + 2], 16)
        offset += 2

        # load offset
        if offset + 4 > rlen:
            return
        ihex["loadofs"] = int(rec[offset:offset + 4], 16)
        offset += 4

        # record type
        if offset + 2 > rlen:
            return
        ihex["rectype"] = int(rec[offset:offset + 2], 16)
        offset += 2

        # checksum
        cksum = (
            ihex["reclen"] +
            ((ihex["loadofs"] >> 8) & 0x0ff) +
            (ihex["loadofs"] & 0x0ff) +
            ihex["rectype"])

        # data
        for j in range(ihex["reclen"]):
            if offset + 2 > rlen:
                return
            ihex["data"][j] = c = int(rec[offset:offset + 2], 16)
            cksum += c
            offset += 2

        # validate checksum
        if offset + 2 > rlen:
            return
        ihex["cksum"] = int(rec[offset:offset + 2], 16)
        rc = -cksum & 0x000000ff
        if rc < 0:
            print("checksum issue")
            return
        if rc != ihex["cksum"]:
            print("checksum mismatch")
            return
        return ihex

    def load_addr(self, addr):
        print("[STK500] Load address {:06x}".format(addr))
        addr = int(addr / 2)
        pkt = struct.pack(
            "BBBB",
            STK_LOAD_ADDRESS,
            addr & 0xff,
            (addr >> 8) & 0xff,
            Sync_CRC_EOP)
        self.write(pkt)
        if self.readbyte() != Resp_STK_INSYNC:
            raise Exception("load_addr() can't get into sync")
        if self.readbyte() != Resp_STK_OK:
            raise Exception("load_addr() protocol error")

    def prog_page(self, memtype, data):
        # print("[STK500] Prog page")
        assert(memtype == "flash")
        block_size = len(data)
        pkt = struct.pack(
            "BBBB",
            STK_PROG_PAGE,
            (block_size >> 8) & 0xff,
            block_size & 0xff,
            ord("F"),  # because flash, othersize E for eeprom
        )
        pkt += data
        pkt += struct.pack("B", Sync_CRC_EOP)
        self.write(pkt)
        if self.readbyte() != Resp_STK_INSYNC:
            raise Exception("prog_page() can't get into sync")
        if self.readbyte() != Resp_STK_OK:
            raise Exception("prog_page() protocol error")

    def read_page(self, memtype, data):
        print("[STK500] Read page")
        assert(memtype == "flash")
        block_size = len(data)
        pkt = struct.pack(
            "BBBBB",
            STK_READ_PAGE,
            (block_size >> 8) & 0xff,
            block_size & 0xff,
            ord("F"),  # because flash, othersize E for eeprom
            Sync_CRC_EOP
        )
        self.write(pkt)
        if self.readbyte() != Resp_STK_INSYNC:
            raise Exception("read_page() can't get into sync")
        data[:] = self.read(block_size)
        if self.readbyte() != Resp_STK_OK:
            raise Exception("read_page() protocol error")

    def get_sync(self):
        print("[STK500] Get sync")
        pkt = struct.pack("BB", STK_GET_SYNC, Sync_CRC_EOP)
        for i in range(10):
            sleep(.3)
            self.write(pkt)
            self.con.flush()
            try:
                if self.readbyte() != Resp_STK_INSYNC:
                    raise Exception("read_page() can't get into sync")
                if self.readbyte() != Resp_STK_OK:
                    raise Exception("read_page() protocol error")
                print("Connected to bootloader")
                return
            except:
                pass
        raise Exception("STK500: cannot get sync")

    def get_parameter(self, param):
        print("[STK500] Get parameter {:x}".format(param))
        self.write(struct.pack("BBB", STK_GET_PARAMETER, param, Sync_CRC_EOP))
        if self.readbyte() != Resp_STK_INSYNC:
            raise Exception("get_parameter() can't get into sync")
        val = self.readbyte()
        if self.readbyte() != Resp_STK_OK:
            raise Exception("get_parameter() protocol error")
        return val

    def read_sign(self):
        print("[STK500] Read signature")
        self.write(struct.pack("BB", STK_READ_SIGN, Sync_CRC_EOP))
        if self.readbyte() != Resp_STK_INSYNC:
            raise Exception("read_sign() can't get into sync")
        sign = struct.unpack("BBB", self.read(3))
        if self.readbyte() != Resp_STK_OK:
            raise Exception("read_sign() protocol error")
        return sign

    def leave_progmode(self):
        print("[STK500] Leaving programming mode")
        pkt = struct.pack("BB",
            STK_LEAVE_PROGMODE,
            Sync_CRC_EOP)
        self.write(pkt)
        if self.readbyte() != Resp_STK_INSYNC:
            raise Exception("leave_progmode() can't get into sync")
        if self.readbyte() != Resp_STK_OK:
            raise Exception("leave_progmode() protocol error")

    def readbyte(self):
        return struct.unpack("B", self.read(1))[0]

    def read(self, size):
        read = 0
        buf = bytearray(b"\x00" * size)
        while read < size:
            ret = self.con.read(size - read)
            # print("serialread: {!r} ({})".format(ret, size-read))
            if ret == "":
                raise Exception("no data read, timeout?")
            buf[read:read + len(ret)] = ret
            read += len(ret)
        return bytes(buf[:read])

    def write(self, pkt):
        pkt = bytearray(pkt)
        # print("[STK500] Packet {}".format(" ".join(
        #    ["{:02x}".format(x) for x in pkt])))
        self.con.write(pkt)

def upload(baudrate, device, firmware):
    Uploader(baudrate, device, firmware).upload()

if __name__ == "__main__":
    upload(sys.argv[1], sys.argv[2], sys.argv[3])
