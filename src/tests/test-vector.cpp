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
#include <numeric>
#include <print>
#include <string>
#include <vector>
#include <span>


void test_accumulate () {
    uint8_t sum;
    std::vector<uint8_t> data_vector {0,1,2,3,4,5,6,7,8,9};
    sum = std::accumulate(data_vector.begin(), data_vector.end(), 0);
    std::println("sum(data_vector) = {}", sum);
//    std::string data_string { "HELLO" };
    std::string data_string { "Hello from client to server!" };
    std::span<uint8_t> data_span { reinterpret_cast<uint8_t *>(data_string.data()), data_string.size() };
    sum = std::accumulate(data_span.begin(), data_span.end(), 0);
    std::print("sum(data_span) = {} = {:02x} = ", sum, sum);
    for (auto& element : data_span) {
       std::print("{}+", element);
    }
}

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

    test_accumulate();
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