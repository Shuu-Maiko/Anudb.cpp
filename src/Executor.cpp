#include "Executor.h"
#include "BPlusTree.h"
#include "RowSerializer.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

Executor::Executor(DatabaseContext &context) : ctx(context) {
  if (ctx.hasActiveDatabase()) {
    metadata = std::make_unique<MetaDataHandler>(ctx.activeDatabase);
    metadata->open();
    // Use a separate file suffix for the B+Tree data to distinguish from
    // metadata
    pm = std::make_unique<PageManager>(ctx.activeDatabase + ".anudb_data");
  }
}

Executor::~Executor() {
  if (metadata && metadata->isOpened())
    metadata->close();
}

void Executor::execute(Statement *stmt) {
  if (auto s = dynamic_cast<CreateDatabaseStatement *>(stmt))
    executeCreateDatabase(s);
  else if (auto s = dynamic_cast<UseDatabaseStatement *>(stmt))
    executeUseDatabase(s);
  else if (auto s = dynamic_cast<ShowDatabasesStatement *>(stmt))
    executeShowDatabases(s);
  else if (auto s = dynamic_cast<CreateStatement *>(stmt))
    executeCreate(s);
  else if (auto s = dynamic_cast<InsertStatement *>(stmt))
    executeInsert(s);
  else if (auto s = dynamic_cast<UpdateStatement *>(stmt))
    executeUpdate(s);
  else if (auto s = dynamic_cast<DeleteStatement *>(stmt))
    executeDelete(s);
  else if (auto s = dynamic_cast<SelectStatement *>(stmt))
    executeSelect(s);
  else
    std::cerr << "Unsupported statement.\n";
}

void Executor::executeCreateDatabase(CreateDatabaseStatement *stmt) {
  std::string filename = stmt->databseName + ".anudb";
  if (fs::exists(filename)) {
    std::cerr << "Error: Database '" << stmt->databseName
              << "' already exists.\n";
    return;
  }
  try {
    MetaDataHandler temp(stmt->databseName);
    temp.open();
    temp.close();
    std::cout << "Database '" << stmt->databseName
              << "' created successfully.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error creating database: " << e.what() << "\n";
  }
}

void Executor::executeUseDatabase(UseDatabaseStatement *stmt) {
  std::string filename = stmt->databaseName + ".anudb";
  if (!fs::exists(filename)) {
    std::cerr << "Error: Database '" << stmt->databaseName
              << "' does not exist.\n";
    return;
  }

  // Close existing handles before switching
  if (metadata)
    metadata->close();
  pm.reset();

  ctx.activeDatabase = stmt->databaseName;
  try {
    metadata = std::make_unique<MetaDataHandler>(ctx.activeDatabase);
    metadata->open();
    pm = std::make_unique<PageManager>(ctx.activeDatabase + ".anudb_data");
    std::cout << "Using database '" << ctx.activeDatabase << "'.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error opening database: " << e.what() << "\n";
    ctx.clear();
  }
}

void Executor::executeShowDatabases(
    [[maybe_unused]] ShowDatabasesStatement *stmt) {
  bool found = false;
  for (const auto &entry : fs::directory_iterator(".")) {
    if (entry.is_regular_file()) {
      std::string fn = entry.path().filename().string();
      if (fn.size() > 6 && fn.substr(fn.size() - 6) == ".anudb") {
        if (!found) {
          std::cout << "Databases:\n";
          found = true;
        }
        std::cout << "  " << fn.substr(0, fn.size() - 6) << "\n";
      }
    }
  }
  if (!found)
    std::cout << "No databases found.\n";
}

void Executor::executeCreate(CreateStatement *stmt) {
  if (!metadata || !metadata->isOpened()) {
    std::cerr << "Error: No active database selected. Use 'USE <dbname>'.\n";
    return;
  }
  try {
    metadata->createTable(stmt->table, stmt->columns);
    std::cout << "Table '" << stmt->table << "' created successfully.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void Executor::executeInsert(InsertStatement *stmt) {
  if (!metadata || !metadata->isOpened()) {
    std::cerr << "Error: No active database selected.\n";
    return;
  }
  try {
    TableInfo info = metadata->getTable(stmt->table);
    std::vector<uint8_t> rowData =
        RowSerializer::serialize(stmt->values, info.columns);

    BPlusTree tree(*pm, info.metadata.rootPageId);
    int64_t key = -1;
    bool hasPrimaryKey = false;
    for (size_t i = 0; i < info.columns.size(); i++) {
      if (info.columns[i].isPrimary() && info.columns[i].getType() == ColumnType::INT) {
        hasPrimaryKey = true;
        key = std::stoll(stmt->values[i]);
        break;
      }
    }
    if (!hasPrimaryKey) {
      key = static_cast<int64_t>(info.metadata.autoIncrementId++);
    }

    if (!tree.insert(key, rowData.data(), rowData.size())) {
      std::cerr << "Error: Failed to insert into B+Tree (duplicate key?).\n";
      return;
    }

    if (info.metadata.rootPageId == 0) {
      info.metadata.rootPageId = tree.getRootPageId();
    }

    metadata->updateTable(stmt->table, info.metadata);
    pm->sync();
    std::cout << "1 row(s) inserted.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void Executor::executeSelect(SelectStatement *stmt) {
  if (!metadata || !metadata->isOpened()) {
    std::cerr << "Error: No active database selected.\n";
    return;
  }
  try {
    TableInfo info = metadata->getTable(stmt->table);
    if (info.metadata.rootPageId == 0) {
      std::cout << "Empty set.\n";
      return;
    }

    BPlusTree tree(*pm, info.metadata.rootPageId);
    int count = 0;

    // Find column index for WHERE if applicable
    int whereColIdx = -1;
    if (stmt->hasWhere) {
      for (size_t i = 0; i < info.columns.size(); i++) {
        if (info.columns[i].name == stmt->where.column) {
          whereColIdx = i;
          break;
        }
      }
      if (whereColIdx == -1)
        throw std::runtime_error("Column not found in WHERE: " +
                                 stmt->where.column);
    }

    for (const auto &col : info.columns) {
      std::cout << col.name << "\t";
    }
    std::cout << "\n";
    for (size_t i = 0; i < info.columns.size(); i++)
      std::cout << "--------";
    std::cout << "\n";

    bool usedFastPath = false;
    if (stmt->hasWhere && whereColIdx != -1) {
      if (info.columns[whereColIdx].isPrimary() && info.columns[whereColIdx].getType() == ColumnType::INT) {
        int64_t pkValue = std::stoll(stmt->where.value);
        std::vector<uint8_t> outValue;
        if (tree.find(pkValue, outValue)) {
          auto vals = RowSerializer::deserialize(outValue.data(), outValue.size(), info.columns);
          for (const auto &v : vals) {
            std::cout << v << "\t";
          }
          std::cout << "\n";
          count++;
        }
        usedFastPath = true;
      }
    }

    if (!usedFastPath) {
      tree.range(0, 0x7FFFFFFFFFFFFFFF,
                 [&](int64_t /*key*/, const uint8_t *value, uint16_t len) {
                   auto vals =
                       RowSerializer::deserialize(value, len, info.columns);

                   bool match = true;
                   if (stmt->hasWhere) {
                     match = (vals[whereColIdx] == stmt->where.value);
                   }

                   if (match) {
                     for (const auto &v : vals) {
                       std::cout << v << "\t";
                     }
                     std::cout << "\n";
                     count++;
                   }
                 });
    }
    std::cout << "(" << count << " rows fetched)\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void Executor::executeUpdate(UpdateStatement *stmt) {
  if (!metadata || !metadata->isOpened()) {
    std::cerr << "Error: No active database selected.\n";
    return;
  }
  try {
    TableInfo info = metadata->getTable(stmt->table);
    if (info.metadata.rootPageId == 0) {
      std::cout << "0 row(s) updated.\n";
      return;
    }

    BPlusTree tree(*pm, info.metadata.rootPageId);
    int count = 0;

    int updateColIdx = -1;
    for (size_t i = 0; i < info.columns.size(); i++) {
      if (info.columns[i].name == stmt->column) {
        updateColIdx = i;
        break;
      }
    }
    if (updateColIdx == -1)
      throw std::runtime_error("Column not found to update: " + stmt->column);

    int whereColIdx = -1;
    if (stmt->hasWhere) {
      for (size_t i = 0; i < info.columns.size(); i++) {
        if (info.columns[i].name == stmt->where.column) {
          whereColIdx = i;
          break;
        }
      }
      if (whereColIdx == -1)
        throw std::runtime_error("Column not found in WHERE: " +
                                 stmt->where.column);
    }

    std::vector<std::pair<int64_t, std::vector<uint8_t>>> toUpdate;

    bool usedFastPath = false;
    if (stmt->hasWhere && whereColIdx != -1) {
      if (info.columns[whereColIdx].isPrimary() && info.columns[whereColIdx].getType() == ColumnType::INT) {
        int64_t pkValue = std::stoll(stmt->where.value);
        std::vector<uint8_t> outValue;
        if (tree.find(pkValue, outValue)) {
          auto vals = RowSerializer::deserialize(outValue.data(), outValue.size(), info.columns);
          vals[updateColIdx] = stmt->value;
          std::vector<uint8_t> newData = RowSerializer::serialize(vals, info.columns);
          toUpdate.push_back({pkValue, newData});
          count++;
        }
        usedFastPath = true;
      }
    }

    if (!usedFastPath) {
      tree.range(0, 0x7FFFFFFFFFFFFFFF,
                 [&](int64_t key, const uint8_t *value, uint16_t len) {
                   auto vals =
                       RowSerializer::deserialize(value, len, info.columns);
                   bool match = true;
                   if (stmt->hasWhere) {
                     match = (vals[whereColIdx] == stmt->where.value);
                   }

                   if (match) {
                     vals[updateColIdx] = stmt->value;
                     std::vector<uint8_t> newData =
                         RowSerializer::serialize(vals, info.columns);
                     toUpdate.push_back({key, newData});
                     count++;
                   }
                 });
    }

    for (auto &item : toUpdate) {
      tree.remove(item.first);
      tree.insert(item.first, item.second.data(), item.second.size());
    }

    pm->sync();
    std::cout << count << " row(s) updated.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void Executor::executeDelete(DeleteStatement *stmt) {
  if (!metadata || !metadata->isOpened()) {
    std::cerr << "Error: No active database selected.\n";
    return;
  }
  try {
    TableInfo info = metadata->getTable(stmt->table);
    if (info.metadata.rootPageId == 0) {
      std::cout << "0 row(s) deleted.\n";
      return;
    }

    BPlusTree tree(*pm, info.metadata.rootPageId);
    int count = 0;

    int whereColIdx = -1;
    if (stmt->hasWhere) {
      for (size_t i = 0; i < info.columns.size(); i++) {
        if (info.columns[i].name == stmt->where.column) {
          whereColIdx = i;
          break;
        }
      }
      if (whereColIdx == -1)
        throw std::runtime_error("Column not found in WHERE: " +
                                 stmt->where.column);
    }

    std::vector<int64_t> toDelete;

    bool usedFastPath = false;
    if (stmt->hasWhere && whereColIdx != -1) {
      if (info.columns[whereColIdx].isPrimary() && info.columns[whereColIdx].getType() == ColumnType::INT) {
        int64_t pkValue = std::stoll(stmt->where.value);
        std::vector<uint8_t> outValue;
        if (tree.find(pkValue, outValue)) {
          toDelete.push_back(pkValue);
          count++;
        }
        usedFastPath = true;
      }
    }

    if (!usedFastPath) {
      tree.range(0, 0x7FFFFFFFFFFFFFFF,
                 [&](int64_t key, const uint8_t *value, uint16_t len) {
                   auto vals =
                       RowSerializer::deserialize(value, len, info.columns);
                   bool match = true;
                   if (stmt->hasWhere) {
                     match = (vals[whereColIdx] == stmt->where.value);
                   }

                   if (match) {
                     toDelete.push_back(key);
                     count++;
                   }
                 });
    }

    for (int64_t key : toDelete) {
      tree.remove(key);
    }

    metadata->updateTable(stmt->table, info.metadata);
    pm->sync();
    std::cout << count << " row(s) deleted.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}
