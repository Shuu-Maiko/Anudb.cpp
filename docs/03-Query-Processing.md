# Query Processing

This document covers how SQL text becomes executable operations.

---

## Architecture

```
SQL String → Tokenizer → Parser → AST → Executor → Storage
```

**Key principle:** The parser doesn't know about disk, schemas, or files. It just produces AST. Validation happens later in the Executor.

This makes the parser easily testable in isolation.

---

## Tokenizer (Lexer)

Scans the input string character-by-character. No regex — just `std::isalpha` checks.

Input:
```sql
SELECT * FROM users WHERE id = 1;
```

Output:
```
[SELECT] [*] [FROM] [users] [WHERE] [id] [=] [1] [;]
```

---

## Parser: Recursive Descent

Hand-rolled parser. No Bison/Flex. This keeps the codebase readable — no generated C macros.

### Grammar Constraints
- No subqueries
- No JOINs
- No nested expressions

This bounds parsing to deterministic O(n) time.

---

## AST: Polymorphic Statements

All statements inherit from a base `Statement` class:

| Statement | Attributes |
|---|---|
| `SelectStatement` | columns, table, where |
| `InsertStatement` | table, values |
| `UpdateStatement` | table, set, where |
| `DeleteStatement` | table, where |
| `CreateStatement` | table, columns |

Each has its own `execute()` method.

---

## Unified WHERE Clause

Instead of duplicating predicate logic, all statements share one `WhereClause` struct:

```cpp
struct WhereClause {
    std::string column;
    Value value;
    Op op;  // =, >, <, etc.
};
```

`SELECT`, `UPDATE`, and `DELETE` all use it.

---

## Executor: Schema Validation

Before any operation, the Executor validates:

1. Table exists
2. Columns exist and types match
3. Primary key constraints

If validation fails → throw early, before touching disk.

---

## Query Execution Flow

### SELECT with WHERE
1. Find root page
2. Traverse B+Tree to leaf
3. If no WHERE → scan all leaf nodes (range scan via `nextLeaf`)
4. If WHERE → evaluate each row, add matches to result

### INSERT
1. Validate schema
2. Serialize values to binary
3. Generate autoIncrementId (auto-increment)
4. Insert into B+Tree
5. `msync` to disk

### UPDATE
1. Find existing row
2. Delete old record (B+Tree remove)
3. Serialize new values
4. Insert new record (may trigger split)

### DELETE
1. Find leaf containing key
2. Remove key + payload
3. Shift remaining slots

---

## String Parsing

The tokenizer strips quotes from string literals:

Input: `'Alice'`
Token: `Alice`

No substring allocation. Just preserves the inner sequence.

---

## Error Handling

Parser errors throw `std::runtime_error`. The REPL catches them and prints the message, but **doesn't crash**:

```cpp
try {
    executeQuery(sql);
} catch (const std::runtime_error& e) {
    std::cout << "Error: " << e.what() << "\n";
    // Context remains valid, prompt again
}
```

---

## Performance Note

The fastest operations are memory-bound. `std::cout` is often the bottleneck — printing takes longer than the actual query execution.

Benchmarks silence `std::cout` to measure pure throughput.
