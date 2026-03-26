# REPL, Context & Testing

This document covers the interactive interface, session management, testing, and benchmarking.

---

## REPL (Read-Eval-Print Loop)

Anudb runs as an interactive terminal:

```bash
./anudb
Anudb> CREATE DATABASE mydb;
Anudb> USE mydb;
Anudb [mydb]> CREATE TABLE users (id INT, name TEXT);
Anudb [mydb]> INSERT INTO users VALUES (1, 'Alice');
Anudb [mydb]> SELECT * FROM users;
Anudb [mydb]> .exit
```

### How It Works

```cpp
while (true) {
    print_prompt();
    std::string sql;
    std::getline(std::cin, sql);
    if (sql == ".exit") break;
    try {
        execute(sql);
    } catch (std::runtime_error& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}
```

### Key Points
- **Stateless:** Context is rebuilt after each command
- **Crash-proof:** Exceptions don't kill the REPL
- **Scriptable:** Run from file: `./anudb < script.sql`

---

## DatabaseContext

All state lives in one struct:

```cpp
struct DatabaseContext {
    std::string activeDatabase;
    std::string dataDir;
    MetaDataHandler* metadata;
    PageManager* pageManager;
};
```

Passed explicitly to functions. No globals.

### Active Database

`USE mydb;` switches context:

1. Flush current `PageManager` (msync)
2. Delete old `PageManager`
3. Load new metadata and page manager

Table names are namespaced: `testdb/users.anudb` vs `proddb/users.anudb`.

### Prompt

Shows active database:
- No DB: `Anudb> `
- With DB: `Anudb [mydb]> `

---

## Exit

`.exit` is special — bypasses parser, returns `false` to loop. This lets the `DatabaseContext` destructor run, which flushes dirty pages.

---

## Testing

### No External Frameworks

Uses plain `cassert` and exit codes. No GoogleTest or Catch2. Keeps dependencies minimal.

### Test Suites

| Test | What It Checks |
|---|---|
| `test_bplustree` | Storage engine (insert, delete, split, range scan) |
| `test_serializer` | Binary encoding (strings, schema flags) |
| `billion_bench` | Performance benchmarking |

Additional test (not built by default):
| `test_persistence` | Kill + reload — data survives process death |

### Random Distribution

Tests use `rand()` keys, not sequential (1,2,3...). This forces node splits and cascading changes.

### Memory Leaks

Run with `valgrind`:
```bash
valgrind --leak-check=full ./test_bplustree
```

Should report zero leaks.

---

## Benchmarking

### How It Works

Uses `std::chrono::high_resolution_clock` to measure insert/scan times.

```cpp
auto start = std::chrono::high_resolution_clock::now();
for (int i = 0; i < 1000000; i++) {
    btree.insert(i, row);
}
auto end = std::chrono::high_resolution_clock::now();
```

### Key Metrics
- **1M+ ops/sec** sustained insertion
- **O(log n)** point lookup
- **O(n)** range scan

### Note on Benchmarking

`std::cout` is the bottleneck. Benchmarks disable printing to measure pure throughput.

---

## Running Tests

```bash
# Build
mkdir build && cd build
cmake .. && make

# Run all tests
./test_bplustree
./test_serializer
./test_persistence

# Run with valgrind
valgrind --leak-check=full ./test_bplustree

# Benchmark
./test_bplustree --benchmark
```
