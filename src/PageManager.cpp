#include "PageManager.h"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

PageManager::PageManager(const std::string &filename)
    : filename(filename), fd(-1), mmapBase(nullptr), mappedSize(0),
      header(nullptr) {
  openOrCreate();
}

PageManager::~PageManager() {
  if (mmapBase != nullptr && mmapBase != MAP_FAILED) {
    msync(mmapBase, mappedSize, MS_SYNC);
  }
  unmapFile();

  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}
void PageManager::openOrCreate() {
  fd = ::open(filename.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    handleError("Failed to open file '" + filename + "'");
    return;
  }

  struct stat st;
  if (::fstat(fd, &st) == -1) {
    handleError("Failed to stat file '" + filename + "'");
    return;
  }

  if (st.st_size == 0) {
    initNewFile();
  } else {
    mapFile(static_cast<size_t>(st.st_size));
    validateExistingFile();
  }
}

void PageManager::initNewFile() {
  if (::ftruncate(fd, PAGE_SIZE) == -1) {
    handleError("Failed to initialize file size");
    return;
  }

  mapFile(PAGE_SIZE);

  if (mmapBase) {
    new (mmapBase) FileHeader();
    if (::msync(mmapBase, PAGE_SIZE, MS_SYNC) == -1) {
      handleError("Failed to sync header to disk");
    }
  }
}

void PageManager::validateExistingFile() {
  if (!header)
    return;

  bool isValid = true;
  std::string errorMsg;

  if (!header->isValid()) {
    errorMsg = "Invalid file format: magic bytes mismatch";
    isValid = false;
  } else if (header->version != ANUDB_VERSION) {
    errorMsg = "Unsupported version: " + std::to_string(header->version);
    isValid = false;
  } else if (header->pageSize != PAGE_SIZE) {
    errorMsg = "Page size mismatch: " + std::to_string(header->pageSize);
    isValid = false;
  }

  if (!isValid) {
    std::cerr << "[PageManager Error] " << errorMsg << std::endl;
    unmapFile();
    close(fd);
    fd = -1;
    throw std::runtime_error(errorMsg);
  }
}

void PageManager::mapFile(size_t size) {
  void *ptr = ::mmap(nullptr,                // Address
                     size,                   // Length
                     PROT_READ | PROT_WRITE, // Prot
                     MAP_SHARED,             // flags
                     fd,                     // file descriptor
                     0                       // Offset
  );

  if (ptr == MAP_FAILED) {
    handleError("mmap failed");
    return;
  }

  mmapBase = ptr;
  mappedSize = size;
  header = static_cast<FileHeader *>(mmapBase);
}

void PageManager::unmapFile() {
  if (mmapBase != nullptr && mmapBase != MAP_FAILED) {
    ::munmap(mmapBase, mappedSize);
    mmapBase = nullptr;
    mappedSize = 0;
    header = nullptr;
  }
}

void PageManager::handleError(const std::string &message) {

  std::cerr << "[PageManager Error] " << message << ": " << std::strerror(errno)
            << std::endl;

  unmapFile();
  if (fd != -1) {
    ::close(fd);
    fd = -1;
  }
  throw std::runtime_error(message);
}

void *PageManager::getPage(uint32_t pageId) {
  if (pageId >= header->pageCount) {
    throw std::out_of_range(
        "Page ID " + std::to_string(pageId) +
        " out of range (max: " + std::to_string(header->pageCount - 1) + ")");
  }
  return static_cast<char *>(mmapBase) + pageId * PAGE_SIZE;
}

uint32_t PageManager::allocatePage() {
  if (header->freeListHead != 0) {
    uint32_t pageId = header->freeListHead;
    FreePage *fp = static_cast<FreePage *>(getPage(pageId));
    header->freeListHead = fp->nextFreePage;
    std::memset(static_cast<void *>(fp), 0, PAGE_SIZE);
    return pageId;
  }
  uint32_t newPageId = header->pageCount;
  size_t requiredSize = (static_cast<size_t>(newPageId) + 1) * PAGE_SIZE;
  if (requiredSize > mappedSize) {
    growFile(newPageId + 1);
  }
  header->pageCount++;
  std::memset(getPage(newPageId), 0, PAGE_SIZE);
  return newPageId;
}

void PageManager::freePage(uint32_t pageId) {
  if (pageId == 0) {
    throw std::runtime_error("Cannot free page 0 (header page )");
  }
  if (pageId >= header->pageCount) {
    throw std::out_of_range("Cannot free page " + std::to_string(pageId) +
                            ": out of range");
  }
  FreePage *fp = static_cast<FreePage *>(getPage(pageId));
  fp->nextFreePage = header->freeListHead;
  header->freeListHead = pageId;
}

void PageManager::growFile(size_t minPageCount) {
  size_t newPageCount =
      ((minPageCount + GROWTH_INCREMENT - 1) / GROWTH_INCREMENT) *
      GROWTH_INCREMENT;
  size_t newSize = newPageCount * PAGE_SIZE;
  if (ftruncate(fd, newSize) == -1) {
    throw std::runtime_error("Failed to grow file: " +
                             std::string(strerror(errno)));
  }
  unmapFile();
  mapFile(newSize);
}

void PageManager::sync() {
  if (mmapBase != nullptr && mmapBase != MAP_FAILED) {
    if (msync(mmapBase, mappedSize, MS_SYNC) == -1) {
      throw std::runtime_error("msync failed: " + std::string(strerror(errno)));
    }
  }
}
