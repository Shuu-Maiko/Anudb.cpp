#include "RowSerializer.h"
#include <iostream>
#include <cassert>

int main() {
    std::vector<Attribute> columns;
    
    Attribute id;
    std::strncpy(id.name, "id", sizeof(id.name));
    id.setType(ColumnType::INT);
    columns.push_back(id);

    Attribute name;
    std::strncpy(name.name, "name", sizeof(name.name));
    name.setType(ColumnType::TEXT);
    columns.push_back(name);

    Attribute score;
    std::strncpy(score.name, "score", sizeof(score.name));
    score.setType(ColumnType::FLOAT);
    columns.push_back(score);

    std::vector<std::string> values = {"123", "Anu", "98.5"};

    try {
        std::vector<uint8_t> data = RowSerializer::serialize(values, columns);
        std::cout << "Serialized data size: " << data.size() << " bytes\n";

        std::vector<std::string> decoded = RowSerializer::deserialize(data.data(), data.size(), columns);
        
        assert(decoded.size() == 3);
        assert(decoded[0] == "123");
        assert(decoded[1] == "Anu");
        assert(decoded[2] == "98.500000"); // std::to_string for float adds precision

        std::cout << "Test passed!\n";
        for (const auto& v : decoded) {
            std::cout << v << " ";
        }
        std::cout << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
