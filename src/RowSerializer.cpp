#include "RowSerializer.h"
#include <cstring>
#include <stdexcept>

std::vector<uint8_t>
RowSerializer::serialize(const std::vector<std::string> &values,
                         const std::vector<Attribute> &columns) {
  if (values.size() != columns.size()) {
    throw std::runtime_error("Column count mismatch during insert");
  }
  std::vector<uint8_t> row;
  for (size_t i = 0; i < columns.size(); i++) {

    if (columns[i].type == static_cast<uint8_t>(ColumnType::INT)) {
      int32_t val;
      try {
        val = std::stoi(values[i]);
      } catch (...) {
        throw std::runtime_error("Invalid integer value: " + values[i]);
      }
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&val);
      row.insert(row.end(), ptr, ptr + sizeof(int32_t));
    } else if (columns[i].type == static_cast<uint8_t>(ColumnType::FLOAT)) {
      float val;
      try {
        val = std::stof(values[i]);
      } catch (...) {
        throw std::runtime_error("Invalid float value: " + values[i]);
      }
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&val);
      row.insert(row.end(), ptr, ptr + sizeof(float));
    } else if (columns[i].type == static_cast<uint8_t>(ColumnType::TEXT)) {
      uint16_t len = static_cast<uint16_t>(values[i].length());
      uint8_t *lenPtr = reinterpret_cast<uint8_t *>(&len);
      row.insert(row.end(), lenPtr, lenPtr + sizeof(uint16_t));
      row.insert(row.end(), values[i].begin(), values[i].end());
    }
  }
  return row;
}

std::vector<std::string>
RowSerializer::deserialize(const uint8_t *data, uint16_t length,
                           const std::vector<Attribute> &columns) {
  std::vector<std::string> values;
  size_t offset = 0;
  for (size_t i = 0; i < columns.size(); i++) {
    if (offset >= length)
      break;

    if (columns[i].type == static_cast<uint8_t>(ColumnType::INT)) {
      if (offset + sizeof(int32_t) > length)
        break;
      int32_t val;
      std::memcpy(&val, data + offset, sizeof(int32_t));
      values.push_back(std::to_string(val));
      offset += sizeof(int32_t);
    } else if (columns[i].type == static_cast<uint8_t>(ColumnType::FLOAT)) {
      if (offset + sizeof(float) > length)
        break;
      float val;
      std::memcpy(&val, data + offset, sizeof(float));
      values.push_back(std::to_string(val));
      offset += sizeof(float);
    } else if (columns[i].type == static_cast<uint8_t>(ColumnType::TEXT)) {
      if (offset + sizeof(uint16_t) > length)
        break;
      uint16_t len;
      std::memcpy(&len, data + offset, sizeof(uint16_t));
      offset += sizeof(uint16_t);
      if (offset + len > length)
        break;
      std::string text(reinterpret_cast<const char *>(data + offset), len);
      values.push_back(text);
      offset += len;
    }
  }
  return values;
}
