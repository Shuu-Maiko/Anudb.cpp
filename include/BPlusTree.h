#pragma once
#include "PageManager.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>
constexpr uint8_t INTERNAL_NODE = 0x01;
constexpr uint8_t LEAF_NODE = 0x02;
constexpr uint32_t INVALID_PAGE_ID = 0;
constexpr uint16_t MAX_INTERNAL_KEYS = 337;
constexpr uint16_t MAX_LEAF_KEYS = 200;
constexpr uint16_t MIN_INTERNAL_KEYS = MAX_INTERNAL_KEYS / 2;
constexpr uint16_t MIN_LEAF_KEYS = MAX_LEAF_KEYS / 2;

#pragma pack(push, 1)

struct InternalNode {
  uint8_t nodeType;
  uint16_t keyCount;
  uint32_t parent;
  uint32_t reserved[9];
  int64_t keys[MAX_INTERNAL_KEYS];
  uint32_t children[MAX_INTERNAL_KEYS + 1];

  void init() {
    nodeType = INTERNAL_NODE;
    keyCount = 0;
    parent = INVALID_PAGE_ID;
    std::memset(reserved, 0, sizeof(reserved));
    std::memset(keys, 0, sizeof(keys));
    std::memset(children, 0, sizeof(children));
  }

  bool isFull() { return keyCount >= MAX_INTERNAL_KEYS; }

  uint16_t findChildIndex(int64_t key) const {
    int l = 0;
    int r = keyCount;
    while (l < r) {
      int mid = (l + r) >> 1;
      if (keys[mid] <= key) {
        l = mid + 1;
      } else {
        r = mid;
      }
    }
    return l;
  }
};

struct LeafSlot {
  int64_t key;
  uint16_t valueOffset;
  uint16_t valueLen;
};

struct LeafNode {

  uint8_t nodeType;
  uint8_t keyCount;
  uint32_t parent;
  uint32_t prevLeaf;
  uint32_t nextLeaf;
  uint8_t reserved[9];
  LeafSlot slots[MAX_LEAF_KEYS];

  void init() {
    nodeType = LEAF_NODE;
    keyCount = 0;
    parent = INVALID_PAGE_ID;
    prevLeaf = INVALID_PAGE_ID;
    nextLeaf = INVALID_PAGE_ID;
    std::memset(reserved, 0, sizeof(reserved));
    std::memset(slots, 0, sizeof(slots));
  }

  bool hasSpaceFor(uint16_t valueLen) const {
    size_t headerSize = 23;
    size_t slotsEnd = headerSize + (keyCount + 1) * sizeof(LeafSlot);
    uint16_t valuesStart = getNextValueOffset() - valueLen;
    return valuesStart >= slotsEnd;
  }

  uint16_t findPosition(int64_t key) const {
    int left = 0;
    int right = keyCount;

    while (left < right) {
      int mid = (left + right) / 2;
      if (slots[mid].key < key) {
        left = mid + 1;
      } else {
        right = mid;
      }
    }
    return left;
  }

  uint8_t *getValuePtr(uint16_t slotIndex, void *pageBase) const {
    return static_cast<uint8_t *>(pageBase) + slots[slotIndex].valueOffset;
  }

  uint16_t getNextValueOffset() const {
    if (keyCount == 0) {
      return PAGE_SIZE;
    }

    uint16_t minOffset = PAGE_SIZE;
    for (uint16_t i = 0; i < keyCount; i++) {
      if (slots[i].valueOffset < minOffset) {
        minOffset = slots[i].valueOffset;
      }
    }
    return minOffset;
  }
};

#pragma pack(pop)

static_assert(sizeof(InternalNode) <= PAGE_SIZE,
              "internalNode not fitting in one page");
static_assert(sizeof(LeafNode) <= PAGE_SIZE,
              "leafNode base must fit in one page!");

class BPlusTree {
public:
  explicit BPlusTree(PageManager &pm, uint32_t rootPageId = 0);
  BPlusTree(const BPlusTree &) = delete;
  BPlusTree &operator=(const BPlusTree &) = delete;

  bool insert(int64_t key, const uint8_t *value, uint16_t valueLen);

  bool find(int64_t key, std::vector<uint8_t> &outValue);

  bool remove(int64_t key);

  void range(
      int64_t startKey, int64_t endKey,
      std::function<void(int64_t key, const uint8_t *value, uint16_t valueLen)>
          callback);

  uint32_t getRootPageId() const { return rootPageId_; }

  size_t size() const { return keyCount_; }

  void dump();

private:
  PageManager &pm_;
  uint32_t rootPageId_;
  size_t keyCount_;
  LeafNode *findLeaf(int64_t key);
  uint32_t findLeafPageId(int64_t key);

  void insertIntoLeaf(LeafNode *leaf, uint32_t leafPageId, int64_t key,
                      const uint8_t *value, uint16_t valueLen);

  void splitLeaf(LeafNode *leaf, uint32_t leafPageId);

  void insertIntoParent(uint32_t leftPageId, int64_t key, uint32_t rightPageId);

  void insertIntoInternal(InternalNode *node, uint32_t nodePageId, int64_t key,
                          uint32_t rightChild);

  void splitInternal(InternalNode *node, uint32_t nodePageId);

  void *getPage(uint32_t pageId) { return pm_.getPage(pageId); }

  uint32_t allocatePage() { return pm_.allocatePage(); }
};
