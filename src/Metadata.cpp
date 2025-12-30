#include "Metadata.h"
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

void MetaDataHandler::initilize() {
  if (!fs::exists(filename)) {

    std::fstream touch(filename);
  }

  fileStream.open(filename, std::ios::app | std::ios::binary);

  if (!fileStream) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
}

void MetaDataHandler::returnToBegin() {
  filename.clear();
  fileStream.seekg(0, std::ios::beg);
}

TableMetaData MetaDataHandler::getTable(std::string tableName) {
  TableMetaData temp;
  auto it = indexs.find(tableName);
  if (it == indexs.end())
    throw std::runtime_error("Table not found: " + tableName);
}
