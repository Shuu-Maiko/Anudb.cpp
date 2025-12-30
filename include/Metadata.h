

#include <cstdint>
#include <fstream>
#include <unordered_map>
struct TableMetaData {
  char tableName[32];
  char fileName[32];
  uint32_t columnCount;
  uint32_t reserved; // padding
}; // size = 32+32+4+4 = 72 bytes 8 bytes aligned

class MetaDataHandler {
private:
  std::fstream fileStream;
  std::string filename = "metadata.db";
  std::unordered_map<std::string, std::streampos> indexs;

public:
  MetaDataHandler(); // TODO: Add custom name for metadata file name
  void initilize();
  void returnToBegin();
  TableMetaData getTable(std::string tableName);
};
