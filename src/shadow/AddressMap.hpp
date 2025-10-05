///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow address map
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#include <array>

namespace shadow {

    template <typename XLEN>
    struct AddressBlock {
        XLEN base;
        XLEN size;
    };

    template <typename XLEN, std::size_t NUM>
    using AddressBlockArray = std::array<AddressBlock<XLEN>, NUM>;

    // cumulative address block array size
    template <typename XLEN, std::size_t NUM>
    constexpr XLEN addressBlockSize (
        const AddressBlockArray<XLEN, NUM> blocks
    ) {
        XLEN size = 0;
        for (const auto& block : blocks) {
            size += block.size;
        };
        return size;
    };

    // address block array alignment check
    template <typename XLEN, std::size_t NUM>
    constexpr bool addressBlockAlignment (
        const AddressBlockArray<XLEN, NUM> blocks
    ) {
        for (const auto& block : blocks) {
            // check base alignment
            if (block.base % sizeof(XLEN))  return false;
            // check size alignment
            if (block.size % sizeof(XLEN))  return false;
        };
        return true;
    };

    // address map
    template <typename XLEN, std::size_t MNUM, std::size_t PNUM>
    struct AddressMap {
        AddressBlockArray<XLEN, MNUM> mem;  // memories
        AddressBlockArray<XLEN, PNUM> i_o;  // I/O peripherals
    };

};
