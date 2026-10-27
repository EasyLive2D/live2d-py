#pragma once

#include "Log.hpp"
#include "DEF.hpp"
#include "Id.hpp"


#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

namespace Live2D {
namespace V2 {

using namespace Live2D::Common::Log;

class ISerializable;

class BinaryReader {
public:
    explicit BinaryReader(std::vector<uint8_t> buf, Id::IdPool& idPool) :
        mBuf(std::move(buf)), mOffset(0), mFormatVersion(0), mOffset8Bit(0), mCurrent8Bit(0),
        mIdPool(idPool) {}
    ~BinaryReader();

    // -- Basic types (big-endian binary format) --
    int ReadNumber();
    int ReadType() { return ReadNumber(); }
    double ReadDouble();
    float ReadFloat32();
    int32_t ReadInt32();
    uint8_t ReadByte();
    int16_t ReadUShort();
    bool ReadBoolean();
    std::string ReadUTF8String();

    // -- Array readers --
    std::vector<int32_t> ReadInt32Array();
    std::vector<float> ReadFloat32Array();
    std::vector<double> ReadFloat64Array();

    // -- Bit-level reading --
    bool ReadBit();
    void CheckBits();

    // -- Polymorphic object reader --
    // Handles: T* (ISerializable subclass), const Id*, std::vector<U*> (object arrays)
    // Use readInt32Array/readFloat32Array/readFloat64Array for primitive arrays
    template<typename T>
    T ReadObject(int type = -1);

    void* ReadObjectRaw(int type = -1);

    // 手动消费多态对象头后，向对象表补偿一个槽位（保持 OBJECT_REF 索引对齐）
    void AlignObjectTable(void* placeholder) { mObjects.push_back(placeholder); }

    // -- Accessors --
    int GetFormatVersion() const { return mFormatVersion; }
    void SetFormatVersion(int version) { mFormatVersion = version; }
    size_t GetOffset() const { return mOffset; }

    Id* ResolveId(const std::string& idStr) { return mIdPool.Resolve(idStr); }
    Id::IdPool& GetIdPool() const { return mIdPool; }

private:
    void* ReadKnownTypeObject(int type);
    void* ReadObjectUntyped(int type = -1);

    std::vector<uint8_t> mBuf;
    size_t mOffset;
    int mFormatVersion;
    int mOffset8Bit;
    int mCurrent8Bit;
    std::vector<void*> mObjects;
    std::vector<std::function<void()>> mDeleters;
    Id::IdPool& mIdPool;

    // Trait to detect std::vector specializations
    template<typename T>
    struct is_vector : std::false_type {};
    template<typename U, typename A>
    struct is_vector<std::vector<U, A>> : std::true_type {};

    // Tag-dispatched implementations
    template<typename T>
    T ReadObjectVector(int type);

    template<typename T>
    T ReadObjectPtr(int type);

    template<typename T>
    T ReadObjectDispatch(std::true_type, int type);

    template<typename T>
    T ReadObjectDispatch(std::false_type, int type);
};

// -- Tag dispatch --

template<typename T>
inline T BinaryReader::ReadObject(int type) {
    return ReadObjectDispatch<T>(is_vector<T>{}, type);
}

template<typename T>
inline T BinaryReader::ReadObjectDispatch(std::true_type /*is_vector*/, int type) {
    return ReadObjectVector<T>(type);
}

template<typename T>
inline T BinaryReader::ReadObjectDispatch(std::false_type /*is_vector*/, int type) {
    return ReadObjectPtr<T>(type);
}

// Helper: reads a primitive-typed array directly (type code already consumed)
template<typename ElemType>
inline std::vector<ElemType> ReadPrimitiveArray(BinaryReader* /*br*/) {
    LOGE("Unsupported primitive array type");
    return {};
}

template<>
inline std::vector<float> ReadPrimitiveArray<float>(BinaryReader* br) {
    return br->ReadFloat32Array();
}
template<>
inline std::vector<int32_t> ReadPrimitiveArray<int32_t>(BinaryReader* br) {
    return br->ReadInt32Array();
}
template<>
inline std::vector<double> ReadPrimitiveArray<double>(BinaryReader* br) {
    return br->ReadFloat64Array();
}

template<typename T>
inline T BinaryReader::ReadObjectVector(int type) {
    using ElemType = typename T::value_type;
    CheckBits();
    int actualType = (type < 0) ? ReadType() : type;

    if (actualType == 0) {
        LOGD("ReadObjectVector: empty vector");
        mObjects.push_back(nullptr);
        return T{};
    }

    if constexpr (std::is_pointer_v<ElemType>) {
        // Pointer-element arrays (type 15 or OBJECT_REF)
        if (actualType == OBJECT_REF) {
            int index = ReadInt32();
            if (index < 0 || index >= static_cast<int>(mObjects.size())) {
                LOGE("Invalid object ref index in vector: %d", index);
                return T{};
            }
            auto* storedVec = static_cast<T*>(mObjects[index]);
            return *storedVec;
        }

        if (actualType != 15) {
            LOGE("Expected array type (15), got %d", actualType);
            return T{};
        }

        int count = ReadType();
        T* resultPtr = new T();
        mDeleters.push_back([resultPtr]() { delete resultPtr; });
        resultPtr->reserve(count);
        for (int i = 0; i < count; i++) {
            resultPtr->push_back(ReadObject<ElemType>());
        }
        mObjects.push_back(resultPtr);   // store for OBJECT_REF
        return *resultPtr;
    } else if constexpr (is_vector<ElemType>::value) {
        // Nested vector (e.g. vector<vector<float>>) — stored as type 15
        if (actualType == OBJECT_REF) {
            int index = ReadInt32();
            if (index < 0 || index >= static_cast<int>(mObjects.size())) {
                LOGE("OBJECT_REF invalid for nested vec at %d", index);
                return T{};
            }
            return *static_cast<T*>(mObjects[index]);
        }
        if (actualType != 15) {
            LOGE("Expected array type (15) for nested vec, got %d", actualType);
            return T{};
        }
        int count = ReadType();
        T* resultPtr = new T();
        mDeleters.push_back([resultPtr]() { delete resultPtr; });
        resultPtr->reserve(count);
        for (int i = 0; i < count; i++) {
            resultPtr->push_back(ReadObject<ElemType>());
        }
        mObjects.push_back(resultPtr);
        return *resultPtr;
    } else {
        // Primitive-element arrays — use readKnownTypeObject which stores in mObjects
        if (actualType == OBJECT_REF) {
            int index = ReadInt32();
            if (index < 0 || index >= static_cast<int>(mObjects.size())) {
                LOGE("Invalid object ref index for primitive: %d", index);
                return T{};
            }
            return *static_cast<T*>(mObjects[index]);
        }
        void* obj = ReadKnownTypeObject(actualType);
        mObjects.push_back(obj);
        return *static_cast<T*>(obj);
    }
}

template<typename T>
inline T BinaryReader::ReadObjectPtr(int type) {
    CheckBits();
    int actualType = (type < 0) ? ReadType() : type;

    if (actualType == 0) {
        mObjects.push_back(nullptr);   // Python stores None for OBJECT_REF alignment
        return nullptr;
    }

    if (actualType == OBJECT_REF) {
        int index = ReadInt32();
        if (index < 0 || index >= static_cast<int>(mObjects.size())) {
            LOGE("Invalid object reference index: %d size=%zu", index, mObjects.size());
            return nullptr;
        }
        return static_cast<T>(mObjects[index]);
    }

    void* obj = ReadKnownTypeObject(actualType);
    mObjects.push_back(obj);
    return static_cast<T>(obj);
}

}   // namespace V2
}   // namespace Live2D