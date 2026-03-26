# Storage & Memory

This document covers how Anudb handles memory, paging, file I/O, and binary serialization.

---

## Page Size: 4096 Bytes

All disk operations work on **4KB blocks**. This matches:
- SSD erase block boundaries
- OS page size
- Hardware prefetch alignment

---

## Memory-Mapped I/O

Instead of using raw read/write syscalls with a custom buffer pool, Anudb uses memory-mapped I/O via `mmap()`. The entire file is mapped into virtual memory, and the OS handles page loading/caching:

```
Request Page N
     ↓
Calculate offset: N * 4096
Add to mmap base address
Return pointer (page may be paged in by OS on access)
```

### Memory Mapping

```cpp
void* mmapBase = ::mmap(nullptr, fileSize, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
```

The OS manages caching, but this relies on the OS page cache (not a custom user-space buffer).

---

## Zero-Copy Projection

With mmap, the page is already in memory. Anudb uses `reinterpret_cast` to project typed pointers directly over the mapped region:

```cpp
void* page = getPage(pageId);
LeafNode* leaf = reinterpret_cast<LeafNode*>(page);
```

No extra copies. Direct memory access.

---

## File I/O: Why No std::fstream?

Standard streams have hidden overhead:
- Formatting facets
- Secondary buffers
- State flags

Anudb uses `mmap()` for file access with POSIX `open`/`close` for file management.

### File Header

The file header occupies the entire first page (4096 bytes):

| Offset | Size | Field |
|---|---|---|
| `0x00` | 4 bytes | `ANUDB_MAGIC` ('A','N','U','B') |
| `0x04` | 4 bytes | Schema Version |
| `0x08` | 4 bytes | Page Size |
| `0x0c` | 4 bytes | Page Count |
| `0x10` | 4 bytes | Free List Head |
| `0x14` | 4 bytes | Root Page ID |
| `0x18` | 4 bytes | Schema Page ID |
| `0x1c` | 8 bytes | Key Count |
| ... | 4060 bytes | Reserved |
| `0x1000` | 4KB | Page 0 starts here |

### Magic Byte Validation

On load, Anudb checks `ANUDB_MAGIC`. If missing or wrong → fail fast. No undefined behavior from corrupted reads.

---

## Durability (msync)

Each `INSERT` calls `msync()` before returning. Data is flushed to disk before the prompt returns.

**Trade-off:** Slower writes, but guaranteed durability. `SIGKILL` won't lose data.

---

## File Growth

When a page overflows, Anudb grows the file in chunks (`GROWTH_INCREMENT`), not byte-by-byte. This avoids filesystem fragmentation.

New space is zero-filled before use.

---

## Binary Serialization

### Length-Prefixed Strings

Instead of null-terminated strings (which require O(n) scan), Anudb prefixes strings with `uint16_t` length:

```
[2 bytes: length] → [N bytes: string]
```

O(1) length lookup on deserialization.

### Bitpacked Schema Flags

Each column stores flags in a single byte:

| Bit | Flag | Purpose |
|---|---|---|
| 0 | `isPrimary` | Unique + clustered index |
| 1 | `isUnique` | Pre-check duplicates on insert |
| 2-7 | Reserved | Future use |

---

## Node Operations

### Insert
1. Serialize row to binary
2. Find leaf via B+Tree traversal
3. Insert into slot
4. Split if full (propagate up)

### Update (Delete + Insert)
1. Remove old key from leaf
2. Serialize new row
3. Insert new row
4. Split if overflow

### Delete
1. Find leaf containing key
2. Remove key + payload
3. Shift remaining slots left (slot shifting)

**Note:** Uses `memmove` (not `memcpy`) because source/dest overlap.

---

## Summary

| Concern | Solution |
|---|---|
| Page alignment | 4096 bytes |
| File I/O | Memory-mapped (mmap) |
| Durability | `msync` after each write |
| Strings | Length-prefixed |
| Schema flags | Bitpacked byte |
| Deletes | Slot shifting with `memmove` |
