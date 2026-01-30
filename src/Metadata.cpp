#include "Metadata.h"
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

MetaDataHandler::MetaDataHandler(const std::string &dbName)
    : filename(dbName + ".anudb"), isOpen(false) {}

MetaDataHandler::~MetaDataHandler() {
  if (isOpen) {
    close();
  }
}

void MetaDataHandler::open() {
  if (isOpen) {
    return;
  }

  bool fileExists = fs::exists(filename);

  if (fileExists) {
    fileStream.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream) {
      throw std::runtime_error("Failed to open metadata file: " + filename);
    }
    readHeader();
    buildIndex();
  } else {
    // creating new file
    fileStream.open(filename, std::ios::out | std::ios::binary);
    if (!fileStream) {
      throw std::runtime_error("Failed to create metadata file: " + filename);
    }
    fileStream.close();

    // reopen for read/write
    fileStream.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream) {
      throw std::runtime_error("Failed to reopen metadata file: " + filename);
    }

    header = MetaDataHeader();
    writeHeader();
  }

  isOpen = true;
}

void MetaDataHandler::close() {
  if (fileStream.is_open()) {
    fileStream.close();
  }
  isOpen = false;
  tableIndex.clear();
}

void MetaDataHandler::writeHeader() {
  fileStream.seekp(0, std::ios::beg);
  fileStream.write(reinterpret_cast<const char *>(&header),
                   sizeof(MetaDataHeader));
  fileStream.flush();
}

void MetaDataHandler::readHeader() {
  fileStream.seekg(0, std::ios::beg);
  fileStream.read(reinterpret_cast<char *>(&header), sizeof(MetaDataHeader));

  if (!header.isValid()) {
    throw std::runtime_error("Invalid metadata file: magic bytes mismatch");
  }

  if (header.version != ANUDB_VERSION) {
    throw std::runtime_error("Unsupported metadata version: " +
                             std::to_string(header.version));
  }
}

void MetaDataHandler::buildIndex() {
  tableIndex.clear();

  // start after header
  std::streampos pos = sizeof(MetaDataHeader);
  fileStream.seekg(pos);

  for (uint32_t i = 0; i < header.tableCount; ++i) {
    TableMetaData tableMeta;
    fileStream.read(reinterpret_cast<char *>(&tableMeta),
                    sizeof(TableMetaData));

    if (!fileStream) {
      throw std::runtime_error("Error reading table metadata at index " +
                               std::to_string(i));
    }

    // hash map of tables meta data
    tableIndex[std::string(tableMeta.tableName)] = pos;

    // skip over the attributes
    pos = fileStream.tellg();
    std::streamoff attrSize =
        static_cast<std::streamoff>(tableMeta.columnCount) * sizeof(Attribute);
    pos += attrSize;
    fileStream.seekg(pos);
  }
}

void MetaDataHandler::createTable(
    const std::string &name, const std::vector<ColumnDefinition> &columns) {
  if (!isOpen) {
    throw std::runtime_error("Database not open");
  }

  if (name.length() >= 256) {
    throw std::runtime_error("Table name too long (max 255 characters)");
  }

  if (tableExists(name)) {
    throw std::runtime_error("Table already exists: " + name);
  }

  if (columns.empty()) {
    throw std::runtime_error("Table must have at least one column");
  }

  // seek to end of file
  fileStream.seekp(0, std::ios::end);
  std::streampos tablePos = fileStream.tellp();

  // write table metadata
  TableMetaData tableMeta;
  std::strncpy(tableMeta.tableName, name.c_str(), 255);
  tableMeta.tableName[255] = '\0';
  tableMeta.columnCount = static_cast<uint32_t>(columns.size());

  fileStream.write(reinterpret_cast<const char *>(&tableMeta),
                   sizeof(TableMetaData));

  // write column attributes
  for (const auto &col : columns) {
    if (col.name.length() >= 256) {
      throw std::runtime_error("Column name too long (max 255 characters): " +
                               col.name);
    }

    Attribute attr;
    std::strncpy(attr.name, col.name.c_str(), 255);
    attr.name[255] = '\0';
    attr.setType(col.type);
    attr.setPrimary(col.isPrimary);
    attr.setUnique(col.isUnique);
    fileStream.write(reinterpret_cast<const char *>(&attr), sizeof(Attribute));
  }

  fileStream.flush();

  // update header
  header.tableCount++;
  writeHeader();
  tableIndex[name] = tablePos;
}

bool MetaDataHandler::tableExists(const std::string &name) const {
  return tableIndex.find(name) != tableIndex.end();
}

TableInfo MetaDataHandler::getTable(const std::string &name) {
  if (!isOpen) {
    throw std::runtime_error("Database not open");
  }

  auto it = tableIndex.find(name);
  if (it == tableIndex.end()) {
    throw std::runtime_error("Table not found: " + name);
  }

  // seek to table position
  fileStream.seekg(it->second);

  TableInfo info;

  // table metadata
  fileStream.read(reinterpret_cast<char *>(&info.metadata),
                  sizeof(TableMetaData));

  // columns
  info.columns.resize(info.metadata.columnCount);
  for (uint32_t i = 0; i < info.metadata.columnCount; ++i) {
    fileStream.read(reinterpret_cast<char *>(&info.columns[i]),
                    sizeof(Attribute));
  }

  return info;
}

std::vector<std::string> MetaDataHandler::listTables() const {
  std::vector<std::string> tables;
  tables.reserve(tableIndex.size());

  for (const auto &pair : tableIndex) {
    tables.push_back(pair.first);
  }

  return tables;
}
