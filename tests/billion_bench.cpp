#include "BPlusTree.h"
#include "Metadata.h"
#include "PageManager.h"
#include "RowSerializer.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

int main() {
    std::string dbName = "billion_db";
    std::cout << "--- Anudb 1-Billion Row Stress Test ---" << std::endl;
    
    // 1. Initialize Metadata and PageManager
    MetaDataHandler meta(dbName);
    meta.open();
    
    std::vector<ColumnDefinition> columns;
    ColumnDefinition idCol;
    idCol.name = "id";
    idCol.type = ColumnType::INT;
    columns.push_back(idCol);
    
    ColumnDefinition nameCol;
    nameCol.name = "name";
    nameCol.type = ColumnType::TEXT;
    columns.push_back(nameCol);
    
    if (!meta.tableExists("billion_table")) {
        meta.createTable("billion_table", columns);
        std::cout << "Table 'billion_table' created." << std::endl;
    }
    
    TableInfo info = meta.getTable("billion_table");
    PageManager pm(dbName + ".anudb_data");
    BPlusTree tree(pm, info.metadata.rootPageId);
    
    long long totalRows = 1000000000; // 1 Billion
    long long batchSize = 1000000;    // Log every 1 million
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::cout << "Starting insertion of " << totalRows << " rows..." << std::endl;
    
    for (long long i = 1; i <= totalRows; ++i) {
        std::vector<std::string> values = {std::to_string(i), "Student_" + std::to_string(i)};
        std::vector<uint8_t> rowData = RowSerializer::serialize(values, info.columns);
        
        if (!tree.insert(i, rowData.data(), rowData.size())) {
            std::cerr << "Insertion failed at row " << i << std::endl;
            return 1;
        }
        
        if (i % batchSize == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = now - start;
            double opsPerSec = i / elapsed.count();
            
            std::cout << "Inserted: " << i << " rows (" << (i * 100.0 / totalRows) << "%) | " 
                      << "Speed: " << (long)opsPerSec << " rows/sec" << std::endl;
            
            // Periodically update root in case of splits
            if (info.metadata.rootPageId != tree.getRootPageId()) {
                info.metadata.rootPageId = tree.getRootPageId();
                meta.updateTable("billion_table", info.metadata);
            }
        }
    }
    
    // Final metadata update
    info.metadata.rootPageId = tree.getRootPageId();
    info.metadata.autoIncrementId = totalRows + 1;
    meta.updateTable("billion_table", info.metadata);
    pm.sync();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalElapsed = end - start;
    
    std::cout << "--- Stress Test Complete ---" << std::endl;
    std::cout << "Total rows: " << totalRows << std::endl;
    std::cout << "Total time: " << totalElapsed.count() << " seconds" << std::endl;
    std::cout << "Average Speed: " << (long)(totalRows / totalElapsed.count()) << " rows/sec" << std::endl;
    std::cout << "Database file: " << dbName << ".anudb_data" << std::endl;
    
    return 0;
}
