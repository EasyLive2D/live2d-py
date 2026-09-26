import struct
from typing import List, Any

from .live2d_object_factory import Live2DObjectFactory
from ..DEF import OBJECT_REF
from ..id import Id


class BinaryReader:

    def __init__(self, buf: bytes):
        self.offset8Bit = 0
        self.current8Bit = 0
        self.formatVersion = 0
        self.objects: List[Any] = []
        self.buf = buf
        self.offset = 0

    def readNumber(self):
        b1 = self.readByte()
        if (b1 & 128) == 0:
            return b1 & 255

        b2 = self.readByte()
        if (b2 & 128) == 0:
            return ((b1 & 127) << 7) | (b2 & 127)

        b3 = self.readByte()
        if (b3 & 128) == 0:
            return ((b1 & 127) << 14) | ((b2 & 127) << 7) | (b3 & 255)

        b4 = self.readByte()
        if (b4 & 128) == 0:
            return ((b1 & 127) << 21) | ((b2 & 127) << 14) | ((b3 & 127) << 7) | (b4 & 255)

        raise RuntimeError("number parse error")

    def getFormatVersion(self):
        return self.formatVersion

    def setFormatVersion(self, aH):
        self.formatVersion = aH

    def readType(self):
        return self.readNumber()

    def readDouble(self):
        self.checkBits()
        ret = self.offset
        self.offset += 8
        return struct.unpack('>d', self.buf[ret:ret + 8])[0]

    def readFloat32(self):
        self.checkBits()
        ret = self.offset
        self.offset += 4
        return struct.unpack('>f', self.buf[ret:ret + 4])[0]

    def readInt32(self):
        self.checkBits()
        ret = self.offset
        self.offset += 4
        return struct.unpack('>i', self.buf[ret:ret + 4])[0]

    def readByte(self):
        self.checkBits()
        ret = self.offset
        self.offset += 1
        return self.buf[ret]

    def readUShort(self):
        self.checkBits()
        ret = self.offset
        self.offset += 2
        return struct.unpack('>h', self.buf[ret:ret + 2])[0]

    def readLong(self):
        self.checkBits()
        self.offset += 8
        raise RuntimeError("_L _q read long")

    def readBoolean(self):
        self.checkBits()
        ret = self.offset
        self.offset += 1
        return self.buf[ret] != 0

    def readUTF8String(self):
        self.checkBits()
        aH = self.readType()
        result = self.buf[self.offset:self.offset + aH].decode("utf-8")
        self.offset += aH
        return result

    def readInt32Array(self):
        self.checkBits()
        aI = self.readType()
        aH = [0] * (aI)
        for aJ in range(0, aI, 1):
            aH[aJ] = self.readInt32()

        return aH

    def readFloat32Array(self):
        self.checkBits()
        aI = self.readType()
        aH = [0.0] * (aI)
        for aJ in range(0, aI, 1):
            aH[aJ] = self.readFloat32()

        return aH

    def readFloat64Array(self):
        self.checkBits()
        aI = self.readType()
        aH = [0.0] * (aI)
        for aJ in range(0, aI, 1):
            aH[aJ] = self.readDouble()

        return aH


    def readObject(self, aJ = -1):
        self.checkBits()
        if aJ < 0:
            aJ = self.readType()

        if aJ == OBJECT_REF:
            aH = self.readInt32()
            if 0 <= aH < len(self.objects):
                return self.objects[aH]
            else:
                raise RuntimeError("_sL _4i @_m0")
        else:
            aI = self.readKnownTypeObject(aJ)
            self.objects.append(aI)
            return aI

    def readKnownTypeObject(self, aN):
        if aN == 0:
            return None
        elif aN == 50:
            aK = self.readUTF8String()
            aI = Id.getID(aK)
            return aI
        elif aN == 51:
            aK = self.readUTF8String()
            aI = Id.getID(aK)
            return aI
        elif aN == 134:
            aK = self.readUTF8String()
            aI = Id.getID(aK)
            return aI
        elif aN == 60:
            aK = self.readUTF8String()
            aI = Id.getID(aK)
            return aI
        elif aN >= 48:
            aL = Live2DObjectFactory.create(aN)
            aL.read(self)
            return aL
        elif aN == 1:
            return self.readUTF8String()
        elif aN == 15:
            aH = self.readType()
            aI = [None] * (aH)
            for aJ in range(0, aH, 1):
                aI[aJ] = self.readObject()
            return aI
        elif aN == 23:
            raise RuntimeError("type not implemented")
        elif aN == 16 or aN == 25:
            return self.readInt32Array()
        elif aN == 26:
            return self.readFloat64Array()
        elif aN == 27:
            return self.readFloat32Array()
        raise RuntimeError("type error %d" % aN)

    def readBit(self):
        if self.offset8Bit == 0:
            self.current8Bit = self.readByte()
        elif self.offset8Bit == 8:
            self.current8Bit = self.readByte()
            self.offset8Bit = 0

        ret = ((self.current8Bit >> (7 - self.offset8Bit)) & 1) == 1
        self.offset8Bit += 1
        return ret

    def checkBits(self):
        if self.offset8Bit != 0:
            self.offset8Bit = 0
