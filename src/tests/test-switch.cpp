///////////////////////////////////////////////////////////////////////////////
// test switch statement with hashed string cases
///////////////////////////////////////////////////////////////////////////////

#include <numeric>
#include <string_view>
#include <iostream>

constexpr auto switch_hash(std::string_view str) {
//    return std::hash<std::string_view>{}(str);
    return std::accumulate(str.begin(), str.end(), 0);
}

void processAction(std::string_view action) {
    switch (switch_hash(action)) {
        case switch_hash("save"):
            std::cout << "Saving data..." << std::endl;
            break;
        case switch_hash("load"):
            std::cout << "Loading data..." << std::endl;
            break;
        case switch_hash("delete"):
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
