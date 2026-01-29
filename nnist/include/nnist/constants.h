#pragma once
#include <cstddef>
#include <cstdint>
namespace nnist {
    constexpr int RECORD_TYPE_1 = 1;
    constexpr int RECORD_TYPE_2 = 2;
    constexpr int RECORD_TYPE_3 = 3;
    constexpr int RECORD_TYPE_4 = 4;
    constexpr int RECORD_TYPE_5 = 5;
    constexpr int RECORD_TYPE_6 = 6;
    constexpr int RECORD_TYPE_7 = 7;
    constexpr int RECORD_TYPE_8 = 8;
    constexpr int RECORD_TYPE_9 = 9;
    constexpr int RECORD_TYPE_10 = 10;
    constexpr int RECORD_TYPE_11 = 11;
    constexpr int RECORD_TYPE_12 = 12;
    constexpr int RECORD_TYPE_14 = 14;
    constexpr int RECORD_TYPE_17 = 17;

    constexpr uint32_t BYTE_MASK = 0xFFU;
    constexpr uint32_t SHIFT_24 = 24U;
    constexpr uint32_t SHIFT_16 = 16U;
    constexpr uint32_t SHIFT_8 = 8U;

    constexpr size_t BYTE_INDEX_0 = 0U;
    constexpr size_t BYTE_INDEX_1 = 1U;
    constexpr size_t BYTE_INDEX_2 = 2U;
    constexpr size_t BYTE_INDEX_3 = 3U;

    constexpr size_t LEN_FIELD_WIDTH = 6U;
    constexpr size_t TAGGED_LEN_WIDTH = 7U;

    constexpr uint32_t MIN_BINARY_RECORD_SIZE = 8U;
    constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

}  // namespace nnist
