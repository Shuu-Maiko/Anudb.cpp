#include "Parser.h"
#include <cstdint>
#include <cstring>

// compiletime computation with constexpr cool na
constexpr uint8_t ATTR_FLAG_UNIQUE = 0x01;
constexpr uint8_t ATTR_FLAG_PRIMARY = 0x02;

struct Attribute {
  char name[256]; // Column name
  uint8_t type;   // (0=INT, 1=TEXT, 2=FLOAT)
  uint8_t flags;  // a bitmap : bit0=unique, bit1=primary
  char reserved[22];

  Attribute() : type(0), flags(0) {
    std::memset(name, 0, sizeof(name));
    std::memset(reserved, 0, sizeof(reserved));
  }

  void setUnique(bool val) {
    if (val)
      flags |= ATTR_FLAG_UNIQUE;
    else
      flags &= ~ATTR_FLAG_UNIQUE;
  }

  void setPrimary(bool val) {
    if (val)
      flags |= ATTR_FLAG_PRIMARY;
    else
      flags &= ~ATTR_FLAG_PRIMARY;
  }

  bool isUnique() const { return flags & ATTR_FLAG_UNIQUE; }
  bool isPrimary() const { return flags & ATTR_FLAG_PRIMARY; }

  ColumnType getType() const { return static_cast<ColumnType>(type); }
  void setType(ColumnType t) { type = static_cast<uint8_t>(t); }
};

//  260 bytes
struct TableMetaData {
  char tableName[256];  // Table name
  uint32_t columnCount; // No. of columns

  TableMetaData() : columnCount(0) {
    std::memset(tableName, 0, sizeof(tableName));
  }
};

struct TableInfo {
  TableMetaData metadata;
  std::vector<Attribute> columns;
};

class SchemaManager {
  TableInfo getTable(const std::string &name);

  void createTable(const std::string &name,
                   const std::vector<ColumnDefinition> &columns);
};
