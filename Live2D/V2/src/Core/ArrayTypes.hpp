#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Live2D {
namespace V2 {

template<typename T>
using Array = std::vector<T>;

using Float32Array = std::vector<float>;
using Float64Array = std::vector<double>;
using Int8Array = std::vector<int8_t>;
using Int16Array = std::vector<int16_t>;
using Int32Array = std::vector<int32_t>;

}   // namespace V2
}   // namespace Live2D