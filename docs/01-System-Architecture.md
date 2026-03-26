# System Architecture

This document covers the high-level design, component breakdown, and storage engine fundamentals.

---

## Why Build a Database from Scratch?

Most apps just use SQLite or PostgreSQL. Anudb exists to understand the **raw mechanics** underneath:

- Memory-mapped I/O with OS page cache
- Memory layout determinism
- Algorithmic trade-offs (B+Tree vs HashMap)

---

## Core Components

```
Terminal (REPL) → Tokenizer → Parser → Executor → BPlusTree → PageManager → Disk
                                                              ↓
                                                      RowSerializer
```

| Layer | Module | Purpose |
|---|---|---|
| **Input** | `main.cpp` | Stateless REPL loop |
| **Logical** | `Tokenizer` | Lexical analysis |
| **Logical** | `Parser` | Grammar → AST |
| **Execution** | `Executor` | Routes queries to storage |
| **Physical** | `BPlusTree` | Indexing & storage |
| **Physical** | `PageManager` | Memory-mapped file I/O |
| **Physical** | `RowSerializer` | Binary encoding |
| **Physical** | `MetaDataHandler` | Schema management |

---

## B+Tree vs Other Structures

### Why B+Tree?

| Structure | Point Lookup | Range Scan |
|---|---|---|
| **HashMap** | O(1) | O(n) — must scan all |
| **B-Tree** | O(log n) | O(log n) + scan |
| **B+Tree** | O(log n) | O(log n) + **sequential** |

B+Tree wins because **leaf nodes are linked**:

```
[Leaf1] → [Leaf2] → [Leaf3] → [Leaf4] → ...
```

This enables fast sequential reads for `SELECT * FROM users WHERE age > 25`.

---

## Node Types

### Internal Nodes
- Store routing keys (`int64_t`)
- Store child page pointers (`uint32_t`)
- Direct traffic to the right leaf

### Leaf Nodes
- Store actual row data (`uint8_t*` variable-length)
- Linked to siblings via `nextLeaf` pointer
- No children — just data

---

## File Structure

| Extension | Content |
|---|---|
| `.anudb` | Schema, auto-increment IDs, root pointers |
| `.anudb_data` | Binary B+Tree pages |

This separation means schema can be recovered even if data gets corrupted.

---

## Implicit Clustering

Every table gets an auto-incrementing `autoIncrementId` (stored in `TableMetaData`). This acts as the clustered index key — every row has a unique routing key.

---

## Single-Threaded Design

No locks, no mutexes, no reader/writer contention. Just one thread maxing out throughput for local single-process use. May update with transactions in the future.

---

## Data Flow Example

```sql
SELECT * FROM users WHERE id = 1;
```

1. **Tokenizer** → `['SELECT', '*', 'FROM', 'users', 'WHERE', 'id', '=', '1']`
2. **Parser** → `SelectStatement{columns=['*'], table='users', where={id=1}}`
3. **Executor** → Validates schema, routes to BPlusTree
4. **BPlusTree** → O(log n) traversal to find key=1
5. **RowSerializer** → Decode binary row from leaf
6. **REPL** → Print result
