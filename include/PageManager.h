#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
constexpr size_t PAGE_SIZE = 4096;
constexpr char ANUDB_MAGIC[4] = {'A', 'N', 'U', 'B'};
constexpr uint32_t ANUDB_VERSION = 1;
constexpr size_t GROWTH_INCREMENT = 256; // 256 pages are 1 MB
#pragma pack(push, 1)
struct FileHeader {
  char magic[4]; // ANUB
  uint32_t version;
  uint32_t pageSize; // 4KB
  uint32_t pageCount;
  uint32_t freeListHead; // head of free page list
  uint32_t rootPageId;   // of bptree
  uint32_t schemaPageId; // page location of schema
  uint64_t keyCount;
  char reserved[4060];
  FileHeader()
      : version(ANUDB_VERSION), pageSize(PAGE_SIZE), pageCount(1),
        freeListHead(0), rootPageId(0), schemaPageId(0) {
    std::memcpy(magic, ANUDB_MAGIC, sizeof(magic));
    std::memset(reserved, 0, sizeof(reserved));
  }
  bool isValid() const {
    if (std::memcmp(magic, ANUDB_MAGIC, sizeof(magic)) != 0) {
      return false;
    }
    if (version != ANUDB_VERSION) {
      return false;
    }
    if (pageSize != PAGE_SIZE) {
      return false;
    }
    return true;
  }
};

// compile time check
static_assert(sizeof(FileHeader) == PAGE_SIZE,
              "FileHeader must be exactly 4096 bytes");

struct FreePage {
  uint32_t nextFreePage;
  char padding[4092];
  FreePage() : nextFreePage(0) {}
};

static_assert(sizeof(FreePage) == PAGE_SIZE,
              "FreePage must be exactly 4096 bytes");

#pragma pack(pop)

class PageManager {
public:
  explicit PageManager(const std::string &filename);
  ~PageManager();

  // prevent copying file descriptor issues
  PageManager(const PageManager &) = delete;
  PageManager &operator=(const PageManager &) = delete;

  void *getPage(uint32_t pageId);
  uint32_t allocatePage();
  void freePage(uint32_t pageId);
  void sync();
  uint32_t getPageCount() const { return header->pageCount; }
  uint32_t getFreeListHead() const { return header->freeListHead; }
  const std::string &getFilename() const { return filename; }

private:
  std::string filename;
  int fd = -1;              // File descriptor
  void *mmapBase = nullptr; // mmapped base address
  size_t mappedSize = 0;
  FileHeader *header = nullptr; // points to mmapBase (page 0)

  void openOrCreate();
  void mapFile(size_t size);
  void unmapFile();
  void growFile(size_t minPageCount);
  void initNewFile();
  void validateExistingFile();
  void handleError(const std::string &message);
};
