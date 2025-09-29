///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow address map
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#include <array>

namespace HdlDb {

    template <typename XLEN>
    struct AddressBlock {
        XLEN base;
        XLEN size;
    };

    // cumulative memory block array size
    template <typename XLEN, std::size_t NUM>
    constexpr XLEN addressBlockSize(
        const std::array<AddressBlock<XLEN>, NUM> blocks
    ) {
        XLEN size = 0;
        for (const auto& block : blocks) {
            size += block.size;
        };
        return size;
    };

    // memory block array alignment check
    template <typename XLEN, std::size_t NUM>
    constexpr bool addressBlockAlignment(
        const std::array<AddressBlock<XLEN>, NUM> blocks
    ) {
        for (const auto& block : mem) {
            // check base alignment
            if (block.base % sizeof(XLEN))  return false;
            // check size alignment
            if (block.size % sizeof(XLEN))  return false;
        };
        return true;
    };

};
