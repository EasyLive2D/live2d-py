#pragma once

#include "DEF.hpp"

#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <cstring>
#include <type_traits>

namespace live2d {

class ISerializable;

class BinaryReader {
public:
    explicit BinaryReader(std::vector<uint8_t> buf)
        : mBuf(std::move(buf)), mOffset(0), mFormatVersion(0), mOffset8Bit(0), mCurrent8Bit(0) {}

    // -- Basic types (big-endian binary format) --
    int readNumber();
    int readType() { return readNumber(); }
    double readDouble();
    float readFloat32();
    int32_t readInt32();
    uint8_t readByte();
    int16_t readUShort();
    bool readBoolean();
    std::string readUTF8String();

    // -- Array readers --
    std::vector<int32_t> readInt32Array();
    std::vector<float> readFloat32Array();
    std::vector<double> readFloat64Array();

    // -- Bit-level reading --
    bool readBit();
    void checkBits();

    // -- Polymorphic object reader --
    // Handles: T* (ISerializable subclass), const Id*, std::vector<U*> (object arrays)
    // Use readInt32Array/readFloat32Array/readFloat64Array for primitive arrays
    template<typename T>
    T readObject(int type = -1);

    void* readObjectRaw(int type = -1);

    // -- Accessors --
    int getFormatVersion() const { return mFormatVersion; }
    void setFormatVersion(int version) { mFormatVersion = version; }
    size_t getOffset() const { return mOffset; }

private:
    void* readKnownTypeObject(int type);
    void* readObjectUntyped(int type = -1);

    std::vector<uint8_t> mBuf;
    size_t mOffset;
    int mFormatVersion;
    int mOffset8Bit;
    int mCurrent8Bit;
    std::vector<void*> mObjects;

    // Trait to detect std::vector specializations
    template<typename T> struct is_vector : std::false_type {};
    template<typename U, typename A> struct is_vector<std::vector<U, A>> : std::true_type {};

    // Tag-dispatched implementations
    template<typename T>
    T readObjectVector(int type);

    template<typename T>
    T readObjectPtr(int type);

    template<typename T>
    T readObjectDispatch(std::true_type, int type);

    template<typename T>
    T readObjectDispatch(std::false_type, int type);
};

// -- Tag dispatch --

template<typename T>
inline T BinaryReader::readObject(int type) {
    return readObjectDispatch<T>(is_vector<T>{}, type);
}

template<typename T>
inline T BinaryReader::readObjectDispatch(std::true_type /*is_vector*/, int type) {
    return readObjectVector<T>(type);
}

template<typename T>
inline T BinaryReader::readObjectDispatch(std::false_type /*is_vector*/, int type) {
    return readObjectPtr<T>(type);
}

// Helper: reads a primitive-typed array directly (type code already consumed)
template<typename ElemType>
inline std::vector<ElemType> readPrimitiveArray(BinaryReader* /*br*/) {
    throw std::runtime_error("Unsupported primitive array type");
}

template<>
inline std::vector<float> readPrimitiveArray<float>(BinaryReader* br) {
    return br->readFloat32Array();
}
template<>
inline std::vector<int32_t> readPrimitiveArray<int32_t>(BinaryReader* br) {
    return br->readInt32Array();
}
template<>
inline std::vector<double> readPrimitiveArray<double>(BinaryReader* br) {
    return br->readFloat64Array();
}

template<typename T>
inline T BinaryReader::readObjectVector(int type) {
    using ElemType = typename T::value_type;
    checkBits();
    int actualType = (type < 0) ? readType() : type;

    if (actualType == 0) {
        mObjects.push_back(nullptr);
        return T{};
    }

    if constexpr (std::is_pointer_v<ElemType>) {
        // Pointer-element arrays (type 15 or OBJECT_REF)
        if (actualType == OBJECT_REF) {
            int index = readInt32();
            if (index < 0 || index >= static_cast<int>(mObjects.size())) {
                throw std::runtime_error("Invalid object ref index in vector: "
                                         + std::to_string(index));
            }
            auto* storedVec = static_cast<T*>(mObjects[index]);
            return *storedVec;
        }

        if (actualType != 15) {
            throw std::runtime_error(
                "Expected array type (15), got " + std::to_string(actualType));
        }

        int count = readType();
        T* resultPtr = new T();
        resultPtr->reserve(count);
        for (int i = 0; i < count; i++) {
            resultPtr->push_back(readObject<ElemType>());
        }
        mObjects.push_back(resultPtr); // store for OBJECT_REF
        return *resultPtr;
    } else if constexpr (is_vector<ElemType>::value) {
        // Nested vector (e.g. vector<vector<float>>) — stored as type 15
        if (actualType == OBJECT_REF) {
            int index = readInt32();
            if (index < 0 || index >= static_cast<int>(mObjects.size())) {
                throw std::runtime_error("OBJECT_REF invalid for nested vec at "
                                         + std::to_string(index));
            }
            return *static_cast<T*>(mObjects[index]);
        }
        if (actualType != 15) {
            throw std::runtime_error("Expected array type (15) for nested vec, got "
                                     + std::to_string(actualType));
        }
        int count = readType();
        T* resultPtr = new T();
        resultPtr->reserve(count);
        for (int i = 0; i < count; i++) {
            resultPtr->push_back(readObject<ElemType>());
        }
        mObjects.push_back(resultPtr);
        return *resultPtr;
    } else {
        // Primitive-element arrays — use readKnownTypeObject which stores in mObjects
        if (actualType == OBJECT_REF) {
            int index = readInt32();
            if (index < 0 || index >= static_cast<int>(mObjects.size())) {
                throw std::runtime_error("Invalid object ref index for primitive: "
                                         + std::to_string(index));
            }
            return *static_cast<T*>(mObjects[index]);
        }
        void* obj = readKnownTypeObject(actualType);
        mObjects.push_back(obj);
        return *static_cast<T*>(obj);
    }
}

template<typename T>
inline T BinaryReader::readObjectPtr(int type) {
    checkBits();
    int actualType = (type < 0) ? readType() : type;

    if (actualType == 0) {
        mObjects.push_back(nullptr); // Python stores None for OBJECT_REF alignment
        return nullptr;
    }

    if (actualType == OBJECT_REF) {
        int index = readInt32();
        if (index < 0 || index >= static_cast<int>(mObjects.size())) {
            throw std::runtime_error("Invalid object reference index: "
                                     + std::to_string(index)
                                     + " size=" + std::to_string(mObjects.size()));
        }
        return static_cast<T>(mObjects[index]);
    }

    void* obj = readKnownTypeObject(actualType);
    mObjects.push_back(obj);
    return static_cast<T>(obj);
}

} // namespace live2d
