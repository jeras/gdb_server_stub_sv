///////////////////////////////////////////////////////////////////////////////
// HDLDB main (stand alone executable)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C includes
#include <cstddef>

// C++ includes
#include <print>
#include <vector>

int main() {
    std::vector<int> values { 11, 22, 33 };
    struct comb_t {
        std::vector<int> values { 11, 22, 33 };
        std::vector<std::byte> data;
    };
    comb_t comb;
    comb.data.assign({static_cast<std::byte>(1), static_cast<std::byte>(2), static_cast<std::byte>(3), static_cast<std::byte>(4)});
    for (size_t i=0; i<comb.data.size(); i++) {
        comb.data[i] = static_cast<std::byte>(i);
    }
    for (size_t i=0; i<comb.data.size(); i++) {
       std::println("data[{}] = {}.", i, static_cast<int>(comb.data[i]));
    }
    comb.data.assign({static_cast<std::byte>(17), static_cast<std::byte>(18), static_cast<std::byte>(19), static_cast<std::byte>(20), static_cast<std::byte>(21)});
    for (auto& element : comb.data) {
       std::println("data = {}.", static_cast<int>(element));
    }
    for (size_t i=0; i<values.size(); i++) {
       std::println("values[{}] = {}.", i, values[i]);
    }
    values.assign({13, 14, 15, 16});
    for (auto& element : values) {
       std::println("values = {}.", element);
    }
//    std::println("values = {:n}.", values);
    return 0;
}

// Initialize max to smallest number.
//double max {
//numeric_limits<double>::infinity() };
//for (size_t i { 0 }; i < doubleVector.size(); ++i) {
//print("Enter score {}: ", i + 1);
//cin >> doubleVector[i];
//if (doubleVector[i] > max) {
//max = doubleVector[i];
//}
//}
//max /= 100.0;
//for (auto& element : doubleVector) {
//element /= max;
//print("{} ", element);
//}