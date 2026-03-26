#include "BPlusTree.h"
#include "PageManager.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <queue>
#include <vector>

BPlusTree::BPlusTree(PageManager &pm, uint32_t rootPageId)
    : pm_(pm), rootPageId_(rootPageId), keyCount_(0) {
  FileHeader *header = static_cast<FileHeader *>(pm_.getPage(0));
  if (rootPageId_ == 0) {
    if (header->rootPageId != 0) {
      rootPageId_ = header->rootPageId;
      keyCount_ = header->keyCount;
    } else {
      rootPageId_ = allocatePage();

      header = static_cast<FileHeader *>(pm_.getPage(0));
      LeafNode *root = static_cast<LeafNode *>(getPage(rootPageId_));
      root->init();
      header->rootPageId = rootPageId_;
      header->keyCount = 0;
      pm_.sync();
    }
  }
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
  if (!leaf->hasSpaceFor(valueLen) || leaf->keyCount >= MAX_LEAF_KEYS) {
    splitLeaf(leaf, leafPageId);
    leafPageId = findLeafPageId(key);
    leaf = static_cast<LeafNode *>(getPage(leafPageId));
  }
  insertIntoLeaf(leaf, leafPageId, key, value, valueLen);
  keyCount_++;

  FileHeader *header = static_cast<FileHeader *>(pm_.getPage(0));
  header->keyCount = keyCount_;
  return true;
}

void BPlusTree::insertIntoLeaf(LeafNode *leaf,
                               [[maybe_unused]] uint32_t leafPageId,
                               int64_t key, const uint8_t *value,
                               uint16_t valueLen) {
  uint16_t pos = leaf->findPosition(key);
  uint16_t valueOffset = leaf->getNextValueOffset() - valueLen;
  if (pos < leaf->keyCount) {
    std::memmove(&leaf->slots[pos + 1], &leaf->slots[pos],
                 (leaf->keyCount - pos) * sizeof(LeafSlot));
  }
  leaf->slots[pos].key = key;
  leaf->slots[pos].valueOffset = valueOffset;
  leaf->slots[pos].valueLen = valueLen;
  uint8_t *valueDest =
      static_cast<uint8_t *>(static_cast<void *>(leaf)) + valueOffset;
  std::memcpy(valueDest, value, valueLen);
  leaf->keyCount++;
}

void BPlusTree::splitLeaf(LeafNode *leaf, uint32_t leafPageId) {
  uint32_t newLeafPageId = allocatePage(); // This can invalidate 'leaf'

  leaf = static_cast<LeafNode *>(getPage(leafPageId));
  LeafNode *newLeaf = static_cast<LeafNode *>(getPage(newLeafPageId));

  newLeaf->init();
  uint16_t mid = leaf->keyCount / 2;
  for (uint16_t i = mid; i < leaf->keyCount; i++) {
    uint16_t srcIndex = i;
    uint16_t dstIndex = i - mid;
    newLeaf->slots[dstIndex].key = leaf->slots[srcIndex].key;
    newLeaf->slots[dstIndex].valueLen = leaf->slots[srcIndex].valueLen;

    uint16_t valueLen = leaf->slots[srcIndex].valueLen;
    uint16_t newOffset = newLeaf->getNextValueOffset() - valueLen;
    newLeaf->slots[dstIndex].valueOffset = newOffset;

    uint8_t *srcValue = leaf->getValuePtr(srcIndex, leaf);
    uint8_t *dstValue =
        static_cast<uint8_t *>(static_cast<void *>(newLeaf)) + newOffset;
    std::memcpy(dstValue, srcValue, valueLen);
    newLeaf->keyCount++;
  }
  leaf->keyCount = mid;
  newLeaf->nextLeaf = leaf->nextLeaf;
  newLeaf->prevLeaf = leafPageId;
  leaf->nextLeaf = newLeafPageId;
  if (newLeaf->nextLeaf != INVALID_PAGE_ID) {
    LeafNode *oldNext = static_cast<LeafNode *>(getPage(newLeaf->nextLeaf));
    oldNext->prevLeaf = newLeafPageId;
  }
  newLeaf->parent = leaf->parent;
  int64_t promotedKey = newLeaf->slots[0].key;
  insertIntoParent(leafPageId, promotedKey, newLeafPageId);
}

void BPlusTree::insertIntoParent(uint32_t leftPageId, int64_t key,
                                 uint32_t rightPageId) {

  void *leftPage = getPage(leftPageId);
  uint32_t parentPageId;
  if (*static_cast<uint8_t *>(leftPage) == LEAF_NODE) {
    parentPageId = static_cast<LeafNode *>(leftPage)->parent;
  } else {
    parentPageId = static_cast<InternalNode *>(leftPage)->parent;
  }
  if (parentPageId == INVALID_PAGE_ID) {
    uint32_t newRootPageId = allocatePage();

    leftPage = getPage(leftPageId);
    void *rightPage = getPage(rightPageId);

    InternalNode *newRoot = static_cast<InternalNode *>(getPage(newRootPageId));
    newRoot->init();

    newRoot->keyCount = 1;
    newRoot->keys[0] = key;
    newRoot->children[0] = leftPageId;
    newRoot->children[1] = rightPageId;

    if (*static_cast<uint8_t *>(leftPage) == LEAF_NODE) {
      static_cast<LeafNode *>(leftPage)->parent = newRootPageId;
      static_cast<LeafNode *>(rightPage)->parent = newRootPageId;
    } else {
      static_cast<InternalNode *>(leftPage)->parent = newRootPageId;
      static_cast<InternalNode *>(rightPage)->parent = newRootPageId;
    }
    rootPageId_ = newRootPageId;

    FileHeader *header = static_cast<FileHeader *>(pm_.getPage(0));
    header->rootPageId = rootPageId_;

    return;
  }

  InternalNode *parent = static_cast<InternalNode *>(getPage(parentPageId));

  if (!parent->isFull()) {
    insertIntoInternal(parent, parentPageId, key, rightPageId);

  } else {
    splitInternal(parent, parentPageId);

    void *leftNode = getPage(leftPageId);
    if (*static_cast<uint8_t *>(leftNode) == LEAF_NODE) {
      parentPageId = static_cast<LeafNode *>(leftNode)->parent;
    } else {
      parentPageId = static_cast<InternalNode *>(leftNode)->parent;
    }
    parent = static_cast<InternalNode *>(getPage(parentPageId));

    insertIntoInternal(parent, parentPageId, key, rightPageId);
  }
}

void BPlusTree::insertIntoInternal(InternalNode *node, uint32_t nodePageId,
                                   int64_t key, uint32_t rightChild) {
  uint16_t pos = 0;
  while (pos < node->keyCount && node->keys[pos] < key) {
    pos++;
  }
  for (int i = node->keyCount; i > pos; i--) {
    node->keys[i] = node->keys[i - 1];
    node->children[i + 1] = node->children[i];
  }
  node->keys[pos] = key;
  node->children[pos + 1] = rightChild;
  node->keyCount++;
  void *childPage = getPage(rightChild);
  if (*static_cast<uint8_t *>(childPage) == LEAF_NODE) {
    static_cast<LeafNode *>(childPage)->parent = nodePageId;
  } else {
    static_cast<InternalNode *>(childPage)->parent = nodePageId;
  }
}

void BPlusTree::splitInternal(InternalNode *node, uint32_t nodePageId) {
  uint32_t newNodePageId = allocatePage(); // This can invalidate 'node'

  // Refresh pointers
  node = static_cast<InternalNode *>(getPage(nodePageId));
  InternalNode *newNode = static_cast<InternalNode *>(getPage(newNodePageId));

  newNode->init();
  uint16_t mid = node->keyCount / 2;
  int64_t promotedKey = node->keys[mid];
  newNode->keyCount = node->keyCount - mid - 1;
  for (uint16_t i = 0; i < newNode->keyCount; i++) {
    newNode->keys[i] = node->keys[mid + 1 + i];
  }
  for (uint16_t i = 0; i <= newNode->keyCount; i++) {
    newNode->children[i] = node->children[mid + 1 + i];
    void *childPage = getPage(newNode->children[i]);
    if (*static_cast<uint8_t *>(childPage) == LEAF_NODE) {
      static_cast<LeafNode *>(childPage)->parent = newNodePageId;
    } else {
      static_cast<InternalNode *>(childPage)->parent = newNodePageId;
    }
  }
  node->keyCount = mid;
  newNode->parent = node->parent;
  insertIntoParent(nodePageId, promotedKey, newNodePageId);
}

void BPlusTree::range(
    int64_t startKey, int64_t endKey,
    std::function<void(int64_t, const uint8_t *, uint16_t)> callback) {
  if (startKey > endKey) {
    return;
  }
  LeafNode *leaf = findLeaf(startKey);
  while (leaf != nullptr) {
    for (uint16_t i = 0; i < leaf->keyCount; i++) {
      int64_t key = leaf->slots[i].key;
      if (key < startKey) {
        continue;
      }
      if (key > endKey) {
        return;
      }
      const uint8_t *value = leaf->getValuePtr(i, leaf);
      uint16_t valueLen = leaf->slots[i].valueLen;
      callback(key, value, valueLen);
    }
    if (leaf->nextLeaf == INVALID_PAGE_ID) {
      break;
    }
    leaf = static_cast<LeafNode *>(getPage(leaf->nextLeaf));
  }
}

bool BPlusTree::remove(int64_t key) {
  uint32_t leafPageId = findLeafPageId(key);
  LeafNode *leaf = static_cast<LeafNode *>(getPage(leafPageId));
  uint16_t pos = leaf->findPosition(key);

  if (pos >= leaf->keyCount || leaf->slots[pos].key != key) {
    return false; // Key not found
  }

  if (pos < leaf->keyCount - 1) {
    std::memmove(&leaf->slots[pos], &leaf->slots[pos + 1],
                 (leaf->keyCount - pos - 1) * sizeof(LeafSlot));
  }

  leaf->keyCount--;
  keyCount_--;

  FileHeader *header = static_cast<FileHeader *>(pm_.getPage(0));
  header->keyCount = keyCount_;

  return true;
}

void BPlusTree::dump() {
  std::cout << "\n=== B+Tree Dump ===" << std::endl;
  std::cout << "Root page: " << rootPageId_ << "  Total keys: " << keyCount_
            << std::endl;

  if (rootPageId_ == INVALID_PAGE_ID) {
    std::cout << "(empty tree)" << std::endl;
    std::cout << "===================" << std::endl;
    return;
  }

  std::queue<std::pair<uint32_t, int>> q;
  q.push({rootPageId_, 0});
  int currentLevel = -1;

  while (!q.empty()) {
    auto [pageId, level] = q.front();
    q.pop();

    if (level != currentLevel) {
      currentLevel = level;
      std::cout << "\n[Level " << currentLevel << "]" << std::endl;
    }

    void *page = getPage(pageId);
    uint8_t nodeType = *static_cast<uint8_t *>(page);

    if (nodeType == INTERNAL_NODE) {
      InternalNode *node = static_cast<InternalNode *>(page);
      std::cout << "  Internal(page=" << pageId << " parent=" << node->parent
                << " keys=" << node->keyCount << ") [";
      for (uint16_t i = 0; i < node->keyCount; i++) {
        if (i > 0)
          std::cout << " ";
        std::cout << node->keys[i];
      }
      std::cout << "] children=[";
      for (uint16_t i = 0; i <= node->keyCount; i++) {
        if (i > 0)
          std::cout << " ";
        std::cout << node->children[i];
      }
      std::cout << "]" << std::endl;

      for (uint16_t i = 0; i <= node->keyCount; i++) {
        if (node->children[i] != INVALID_PAGE_ID) {
          q.push({node->children[i], level + 1});
        }
      }
    } else if (nodeType == LEAF_NODE) {
      LeafNode *leaf = static_cast<LeafNode *>(page);
      std::cout << "  Leaf(page=" << pageId << " parent=" << leaf->parent
                << " keys=" << (int)leaf->keyCount << " prev=" << leaf->prevLeaf
                << " next=" << leaf->nextLeaf << ") [";
      for (uint16_t i = 0; i < leaf->keyCount; i++) {
        if (i > 0)
          std::cout << " ";
        std::cout << leaf->slots[i].key << "(" << leaf->slots[i].valueLen
                  << "B)";
      }
      std::cout << "]" << std::endl;
    }
  }

  std::cout << "===================" << std::endl;
}
