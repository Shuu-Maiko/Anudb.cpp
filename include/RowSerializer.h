#pragma once
#include "Metadata.h" // For Attribute and ColumnType
#include <cstdint>
#include <string>
#include <vector>

class RowSerializer {
public:
  static std::vector<uint8_t> serialize(const std::vector<std::string> &values,
                                        const std::vector<Attribute> &columns);
  static std::vector<std::string>
  deserialize(const uint8_t *data, uint16_t length,
              const std::vector<Attribute> &columns);
};
