#include "Context.h"
#include "Parser.h"
#include "Tokenizer.h"
#include "Executor.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <cassert>

namespace fs = std::filesystem;

void run_sql(DatabaseContext& ctx, const std::string& sql) {
    std::vector<Token> tokens = Tokenizer::tokenize(sql);
    Parser parser(tokens);
    auto stmt = parser.parse();
    Executor exec(ctx);
    exec.execute(stmt.get());
}

int main() {
    std::string dbName = "persistence_test";
    std::string metaFile = dbName + ".anudb";
    std::string dataFile = dbName + ".anudb_data";
    
    if (fs::exists(metaFile)) fs::remove(metaFile);
    if (fs::exists(dataFile)) fs::remove(dataFile);

    std::cout << "--- Stage 1: Writing Data ---\n";
    {
        DatabaseContext ctx;
        run_sql(ctx, "CREATE DATABASE persistence_test;");
        run_sql(ctx, "USE persistence_test;");
        run_sql(ctx, "CREATE TABLE shelf (id INT, label TEXT);");
        run_sql(ctx, "INSERT INTO shelf VALUES (1, 'PersistenceCheck');");
        std::cout << "Data written and handles closed.\n";
    }

    std::cout << "\n--- Stage 2: Reading Data after 'Restart' ---\n";
    {
        DatabaseContext ctx;
        ctx.activeDatabase = dbName;
        // In reality, SELECT output is printed to stdout, 
        // we'll rely on the visual output here for the user.
        run_sql(ctx, "SELECT * FROM shelf;");
    }

    return 0;
}
