#include "BPlusTree.h"
#include "PageManager.h"
#include <cstdint>
#include <cstring>
#include <vector>

BPlusTree::BPlusTree(PageManager &pm, uint32_t rootPageId)
    : pm_(pm), rootPageId_(rootPageId), keyCount_(0) {
  if (rootPageId_ == 0) {
    rootPageId_ = allocatePage();

    LeafNode *root = static_cast<LeafNode *>(getPage(rootPageId_));
    root->init();
    pm_.sync();
  }
  // TODO: For existing tree, could count keys by traversing leaves
}

LeafNode *BPlusTree::findLeaf(int64_t key) {
  uint32_t pageId = rootPageId_;
  while (true) {
    void *page = getPage(pageId);
    uint8_t nodeType = *static_cast<uint8_t *>(page);
    if (nodeType == LEAF_NODE) {
      return static_cast<LeafNode *>(page);
    }
    InternalNode *internal = static_cast<InternalNode *>(page);
    uint16_t childIndex = internal->findChildIndex(key);
    pageId = internal->children[childIndex];
  }
}

uint32_t BPlusTree::findLeafPageId(int64_t key) {
  uint32_t pageId = rootPageId_;
  while (true) {
    void *page = getPage(pageId);
    uint8_t nodeType = *static_cast<uint8_t *>(page);

    if (nodeType == LEAF_NODE) {
      return pageId;
    }

    InternalNode *internal = static_cast<InternalNode *>(page);
    uint16_t childIndex = internal->findChildIndex(key);
    pageId = internal->children[childIndex];
  }
}

bool BPlusTree::find(int64_t key, std::vector<uint8_t> &outValue) {
  outValue.clear();
  LeafNode *leaf = findLeaf(key);
  uint16_t pos = leaf->findPosition(key);
  if (pos < leaf->keyCount && leaf->slots[pos].key == key) {
    // uint16_t offset = leaf->slots[pos].valueOffset;
    uint16_t len = leaf->slots[pos].valueLen;
    const uint8_t *valuePtr = leaf->getValuePtr(pos, leaf);
    outValue.assign(valuePtr, valuePtr + len);
    return true;
  }
  return false;
}

bool BPlusTree::insert(int64_t key, const uint8_t *value, uint16_t valueLen) {
  uint32_t leafPageId = findLeafPageId(key);
  LeafNode *leaf = static_cast<LeafNode *>(getPage(leafPageId));
  uint16_t pos = leaf->findPosition(key);
  if (pos < leaf->keyCount && leaf->slots[pos].key == key) {
    return false;
  }
  if (!leaf->hasSpaceFor(valueLen)) {
    splitLeaf(leaf, leafPageId);
    leafPageId = findLeafPageId(key);
    leaf = static_cast<LeafNode *>(getPage(leafPageId));
  }
  insertIntoLeaf(leaf, leafPageId, key, value, valueLen);
  keyCount_++;
  return true;
}

void BPlusTree::insertIntoLeaf(LeafNode *leaf, uint32_t leafPageId, int64_t key,
                               const uint8_t *value, uint16_t valueLen) {
  uint16_t pos = leaf->findPosition(key);
  uint16_t valueOffset = leaf->getNextValueOffset() - valueLen;
  if (pos < leaf->keyCount) {
    std::memmove(&leaf->slots[pos + 1], &leaf->slots[pos],
                 (leaf->keyCount - pos) * sizeof(LeafSlot));
  }
}
