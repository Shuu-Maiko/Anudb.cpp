#pragma once
#include <cstdint>

constexpr uint8_t INTERNAL_NODE = 0x01;
constexpr uint8_t LEAF_NODE = 0x02;
constexpr uint32_t INVALID_PAGE_ID = 0;
constexpr uint16_t MAX_INTERNAL_KEYS = 338;
constexpr uint16_t MAX_LEAF_KEYS = 200;
constexpr uint16_t MIN_INTERNAL_KEYS = MAX_INTERNAL_KEYS / 2;
constexpr uint16_t MIN_LEAF_KEYS = MAX_LEAF_KEYS / 2;
