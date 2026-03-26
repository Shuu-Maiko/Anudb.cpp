#include "BPlusTree.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

void testBPlusTree() {
  const char *testFile = "test_btree.anudb";
  std::remove(testFile);
  std::cout << "Test 1: Creating new B+Tree..." << std::endl;
  {
    PageManager pm(testFile);
    BPlusTree tree(pm);

    assert(tree.size() == 0);
    std::cout << "  ✓ Empty tree created" << std::endl;
  }

  std::cout << "Test 2: Insert and find..." << std::endl;
  {
    PageManager pm(testFile);
    BPlusTree tree(pm);

    for (int i = 0; i < 100; i++) {
      int64_t key = i * 10;
      std::string value = "value_" + std::to_string(i);

      bool inserted =
          tree.insert(key, reinterpret_cast<const uint8_t *>(value.c_str()),
                      value.length() + 1);
      assert(inserted);
    }

    for (int i = 0; i < 100; i++) {
      int64_t key = i * 10;
      std::vector<uint8_t> value;

      bool found = tree.find(key, value);
      assert(found);

      std::string expected = "value_" + std::to_string(i);
      assert(std::strcmp(reinterpret_cast<const char *>(value.data()),
                         expected.c_str()) == 0);
    }

    pm.sync();
    std::cout << "  ✓ 100 key-value pairs inserted and found" << std::endl;
  }

  std::cout << "Test 3: Duplicate key handling..." << std::endl;
  {
    PageManager pm(testFile);
    BPlusTree tree(pm);

    const char *value = "test";
    assert(tree.insert(999, reinterpret_cast<const uint8_t *>(value), 5));
    assert(!tree.insert(999, reinterpret_cast<const uint8_t *>(value), 5));

    std::cout << "  ✓ Duplicate keys rejected correctly" << std::endl;
  }

  std::cout << "Test 4: Range scan..." << std::endl;
  {
    PageManager pm(testFile);
    BPlusTree tree(pm);

    std::vector<int64_t> scannedKeys;
    tree.range(50, 150, [&](int64_t key, const uint8_t *, uint16_t) {
      scannedKeys.push_back(key);
    });

    std::cout << "  Scanned " << scannedKeys.size() << " keys: ";
    for (auto k : scannedKeys)
      std::cout << k << " ";
    std::cout << std::endl;
    assert(scannedKeys.size() == 11);
    assert(scannedKeys.front() == 50);
    assert(scannedKeys.back() == 150);

    // Verify sorted
    assert(std::is_sorted(scannedKeys.begin(), scannedKeys.end()));

    std::cout << "  ✓ Range scan returned correct keys" << std::endl;
  }

  // Test 5: Random order insertion (stress test)
  std::cout << "Test 5: Random order insertion..." << std::endl;
  {
    std::remove(testFile);
    PageManager pm(testFile);
    BPlusTree tree(pm);

    // Generate random keys
    std::vector<int64_t> keys;
    for (int i = 0; i < 1000; i++) {
      keys.push_back(i);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    // Insert in random order
    for (int64_t key : keys) {
      std::string value = "v" + std::to_string(key);
      tree.insert(key, reinterpret_cast<const uint8_t *>(value.c_str()),
                  value.length() + 1);
    }

    // Verify all can be found
    for (int64_t key : keys) {
      std::vector<uint8_t> value;
      std::cout << " Key : " << key << std::endl;
      if (!tree.find(key, value)) {
        assert(false);
      }
    }

    std::cout << "  ✓ 1000 random insertions work correctly" << std::endl;
  }

  std::remove(testFile);

  std::cout << "\\n✅ All B+Tree tests passed!" << std::endl;
}

void benchmarkBPlusTree() {
  const char *testFile = "bench_btree.anudb";
  std::remove(testFile);

  const long long int N = 100000000; // 1 million

  PageManager pm(testFile);
  BPlusTree tree(pm);

  // Benchmark insert
  auto start = std::chrono::high_resolution_clock::now();

  for (long long int i = 0; i < N; i++) {
    std::string val = std::to_string(i);
    tree.insert(i, reinterpret_cast<const uint8_t *>(val.c_str()),
                val.length() + 1);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto insertTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  std::cout << "Insert " << N << " keys: " << insertTime << " ms"
            << " (" << (N * 1000.0 / insertTime) << " ops/sec)" << std::endl;

  // Benchmark find
  start = std::chrono::high_resolution_clock::now();

  std::vector<uint8_t> value;
  for (int i = 0; i < N; i++) {
    tree.find(i, value);
  }

  end = std::chrono::high_resolution_clock::now();
  auto findTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  std::cout << "Find " << N << " keys: " << findTime << " ms"
            << " (" << (N * 1000.0 / findTime) << " ops/sec)" << std::endl;

  // Benchmark range scan (10K keys)
  start = std::chrono::high_resolution_clock::now();

  int count = 0;
  tree.range(100000, 110000,
             [&](int64_t, const uint8_t *, uint16_t) { count++; });

  end = std::chrono::high_resolution_clock::now();
  auto rangeTime =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Range scan (10K keys): " << rangeTime << " μs"
            << " (found " << count << " keys)" << std::endl;
  tree.dump();
  std::remove(testFile);
}

int main() {
    testBPlusTree();
    benchmarkBPlusTree();
    return 0;
}
