#include "BinaryReader.hpp"
#include "ISerializable.hpp"
#include "Id.hpp"
#include "Live2DObjectFactory.hpp"
#include "DEF.hpp"
#include <cstring>

namespace live2d {

// -- Helper for big-endian conversion --
namespace {

inline float readBigFloat(const uint8_t* buf, size_t offset) {
    uint32_t raw;
    memcpy(&raw, buf + offset, 4);
    // Big-endian to native
    raw = ((raw & 0x000000FFU) << 24) | ((raw & 0x0000FF00U) << 8) |
          ((raw & 0x00FF0000U) >> 8)  | ((raw & 0xFF000000U) >> 24);
    float result;
    memcpy(&result, &raw, 4);
    return result;
}

inline int32_t readBigInt32(const uint8_t* buf, size_t offset) {
    uint32_t raw;
    memcpy(&raw, buf + offset, 4);
    return static_cast<int32_t>(
        ((raw & 0x000000FFU) << 24) | ((raw & 0x0000FF00U) << 8) |
        ((raw & 0x00FF0000U) >> 8)  | ((raw & 0xFF000000U) >> 24));
}

inline int16_t readBigShort(const uint8_t* buf, size_t offset) {
    uint16_t raw;
    memcpy(&raw, buf + offset, 2);
    return static_cast<int16_t>(
        ((raw & 0x00FFU) << 8) | ((raw & 0xFF00U) >> 8));
}

inline double readBigDouble(const uint8_t* buf, size_t offset) {
    uint64_t raw;
    memcpy(&raw, buf + offset, 8);
    raw = ((raw & 0x00000000000000FFULL) << 56) | ((raw & 0x000000000000FF00ULL) << 40) |
          ((raw & 0x0000000000FF0000ULL) << 24) | ((raw & 0x00000000FF000000ULL) << 8)  |
          ((raw & 0x000000FF00000000ULL) >> 8)  | ((raw & 0x0000FF0000000000ULL) >> 24) |
          ((raw & 0x00FF000000000000ULL) >> 40) | ((raw & 0xFF00000000000000ULL) >> 56);
    double result;
    memcpy(&result, &raw, 8);
    return result;
}

} // anonymous namespace

// -- BinaryReader implementation --

int BinaryReader::readNumber() {
    int b1 = readByte();
    if ((b1 & 128) == 0) return b1 & 255;

    int b2 = readByte();
    if ((b2 & 128) == 0) return ((b1 & 127) << 7) | (b2 & 127);

    int b3 = readByte();
    if ((b3 & 128) == 0) return ((b1 & 127) << 14) | ((b2 & 127) << 7) | (b3 & 255);

    int b4 = readByte();
    if ((b4 & 128) == 0)
        return ((b1 & 127) << 21) | ((b2 & 127) << 14) | ((b3 & 127) << 7) | (b4 & 255);

    throw std::runtime_error("number parse error");
}

double BinaryReader::readDouble() {
    checkBits();
    double result = readBigDouble(mBuf.data(), mOffset);
    mOffset += 8;
    return result;
}

float BinaryReader::readFloat32() {
    checkBits();
    float result = readBigFloat(mBuf.data(), mOffset);
    mOffset += 4;
    return result;
}

int32_t BinaryReader::readInt32() {
    checkBits();
    int32_t result = readBigInt32(mBuf.data(), mOffset);
    mOffset += 4;
    return result;
}

uint8_t BinaryReader::readByte() {
    checkBits();
    return mBuf[mOffset++];
}

int16_t BinaryReader::readUShort() {
    checkBits();
    int16_t result = readBigShort(mBuf.data(), mOffset);
    mOffset += 2;
    return result;
}

bool BinaryReader::readBoolean() {
    checkBits();
    return mBuf[mOffset++] != 0;
}

std::string BinaryReader::readUTF8String() {
    checkBits();
    int length = readType();
    std::string result(reinterpret_cast<const char*>(&mBuf[mOffset]), length);
    mOffset += length;
    return result;
}

std::vector<int32_t> BinaryReader::readInt32Array() {
    checkBits();
    int count = readType();
    std::vector<int32_t> result(count);
    for (int i = 0; i < count; i++) {
        result[i] = readInt32();
    }
    return result;
}

std::vector<float> BinaryReader::readFloat32Array() {
    checkBits();
    int count = readType();
    std::vector<float> result(count);
    for (int i = 0; i < count; i++) {
        result[i] = readFloat32();
    }
    return result;
}

std::vector<double> BinaryReader::readFloat64Array() {
    checkBits();
    int count = readType();
    std::vector<double> result(count);
    for (int i = 0; i < count; i++) {
        result[i] = readDouble();
    }
    return result;
}

bool BinaryReader::readBit() {
    if (mOffset8Bit == 0) {
        mCurrent8Bit = readByte();
    } else if (mOffset8Bit == 8) {
        mCurrent8Bit = readByte();
        mOffset8Bit = 0;
    }

    bool result = ((mCurrent8Bit >> (7 - mOffset8Bit)) & 1) == 1;
    mOffset8Bit++;
    return result;
}

void BinaryReader::checkBits() {
    if (mOffset8Bit != 0) {
        mOffset8Bit = 0;
    }
}

void* BinaryReader::readObjectRaw(int type) {
    return readObjectUntyped(type);
}

void* BinaryReader::readObjectUntyped(int type) {
    checkBits();
    if (type < 0) type = readType();

    if (type == 0) {
        mObjects.push_back(nullptr);
        return nullptr;
    }

    if (type == OBJECT_REF) {
        int index = readInt32();
        if (index < 0 || index >= static_cast<int>(mObjects.size())) {
            throw std::runtime_error("Invalid object reference index: " + std::to_string(index)
                + " (size=" + std::to_string(mObjects.size()) + ")");
        }
        return mObjects[index];
    }

    void* obj = readKnownTypeObject(type);
    mObjects.push_back(obj);
    return obj;
}

void* BinaryReader::readKnownTypeObject(int type) {
    if (type == 0) {
        return nullptr;
    }
    // String-to-Id types: 50, 51, 134, 60
    if (type == 50 || type == 51 || type == 134 || type == 60) {
        std::string idStr = readUTF8String();
        return const_cast<Id*>(&Id::getID(idStr));
    }
    // Factory-created ISerializable subclasses (type >= 48)
    if (type >= 48) {
        auto obj = Live2DObjectFactory::create(type);
        ISerializable* rawPtr = obj.get();
        obj.release(); // Transfer ownership — will be tracked in mObjects
        rawPtr->read(*this);
        return rawPtr;
    }
    // Primitive types
    if (type == 1) {
        // String — return heap-allocated string
        return new std::string(readUTF8String());
    }
    if (type == 15) {
        // Array of objects — read count then each element
        int count = readType();
        auto* arr = new std::vector<void*>();
        arr->reserve(count);
        for (int i = 0; i < count; i++) {
            arr->push_back(readObjectUntyped());
        }
        return arr;
    }
    if (type == 16 || type == 25) {
        auto* arr = new std::vector<int32_t>(readInt32Array());
        return arr;
    }
    if (type == 26) {
        auto* arr = new std::vector<double>(readFloat64Array());
        return arr;
    }
    if (type == 27) {
        auto* arr = new std::vector<float>(readFloat32Array());
        return arr;
    }
    if (type == 23) {
        throw std::runtime_error("type not implemented");
    }

    throw std::runtime_error("type error: " + std::to_string(type));
}

} // namespace live2d
