///////////////////////////////////////////////////////////////////////////////
// test switch statement with hashed string cases
///////////////////////////////////////////////////////////////////////////////

#include <string_view>
#include <iostream>

constexpr uint64_t hash(std::string_view str) {
    uint64_t hash = 0;
    for (char c : str) {
        hash = (hash * 131) + c;
    }
    return hash;
}

constexpr uint64_t operator"" _hash(const char* str, size_t len) {
    return hash(std::string_view(str, len));
}

void processAction(std::string_view action) {
    switch (hash(action)) {
        case "save"_hash:
            std::cout << "Saving data..." << std::endl;
            break;
        case "load"_hash:
            std::cout << "Loading data..." << std::endl;
            break;
        case "delete"_hash:
            std::cout << "Deleting data..." << std::endl;
            break;
        default:
            std::cout << "Unknown action: " << action << std::endl;
    }
}

int main() {
    processAction("save");
    processAction("delete");
    return 0;
}
