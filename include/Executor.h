#pragma once
#include "Context.h"
#include "Metadata.h"
#include "PageManager.h"
#include "Parser.h"
#include <memory>

class Executor {
private:
  DatabaseContext &ctx;
  std::unique_ptr<MetaDataHandler> metadata;
  std::unique_ptr<PageManager> pm;

public:
  Executor(DatabaseContext &context);
  ~Executor();

  void execute(Statement *stmt);

private:
  void executeCreateDatabase(CreateDatabaseStatement *stmt);
  void executeUseDatabase(UseDatabaseStatement *stmt);
  void executeShowDatabases(ShowDatabasesStatement *stmt);
  void executeCreate(CreateStatement *stmt);
  void executeInsert(InsertStatement *stmt);
  void executeUpdate(UpdateStatement *stmt);
  void executeDelete(DeleteStatement *stmt);
  void executeSelect(SelectStatement *stmt);
};
