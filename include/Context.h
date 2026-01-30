#pragma once
#include <string>

struct DatabaseContext {
  std::string activeDatabase;
  bool hasActiveDatabase() const { return !activeDatabase.empty(); }
  void clear() { activeDatabase.clear(); }
};
