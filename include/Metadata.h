#pragma once
#include "Parser.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Constants.h"

// File header- (12 bytes)
struct MetaDataHeader {
  char magic[4];
  uint32_t version;    // Schema version
  uint32_t tableCount; // Number of tables

  MetaDataHeader() : version(ANUDB_VERSION), tableCount(0) {
    magic[0] = 'A';
    magic[1] = 'N';
    magic[2] = 'U';
    magic[3] = 'B';
  }

  bool isValid() const {
    return magic[0] == 'A' && magic[1] == 'N' && magic[2] == 'U' &&
           magic[3] == 'B';
  }
};

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
  char tableName[248];      // Table name
  uint32_t columnCount;     // No. of columns
  uint32_t rootPageId;      // Root page of this table's B+Tree
  uint32_t autoIncrementId; // Counter for next inserted row

  TableMetaData() : columnCount(0), rootPageId(0), autoIncrementId(1) {
    std::memset(tableName, 0, sizeof(tableName));
  }
};

struct TableInfo {
  TableMetaData metadata;
  std::vector<Attribute> columns;
};

class MetaDataHandler {
private:
  std::fstream fileStream;
  std::string filename;
  std::unordered_map<std::string, std::streampos> tableIndex;
  MetaDataHeader header;
  bool isOpen = false;

public:
  explicit MetaDataHandler(const std::string &dbName = "default");
  ~MetaDataHandler();

  void open();
  void close();
  bool isOpened() const { return isOpen; }

  // table operations
  void createTable(const std::string &name,
                   const std::vector<ColumnDefinition> &columns);
  bool tableExists(const std::string &name) const;
  TableInfo getTable(const std::string &name);
  void updateTable(const std::string &name, const TableMetaData &newMeta);
  std::vector<std::string> listTables() const;
  uint32_t getTableCount() const { return header.tableCount; }

private:
  void writeHeader();
  void readHeader();
  void buildIndex();
};
