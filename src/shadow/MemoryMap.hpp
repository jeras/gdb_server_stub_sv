///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow memory map
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

// C includes
#include <cstddef>

// C++ includes
#include <array>
#include <vector>
#include <span>
#include <map>

// HDLDB includes
#include "AddressMap.hpp"

namespace shadow {

    template <typename XLEN, AddressMap AMAP>
    class MemoryMap {
        // core local memories (array of address map regions)
        std::vector<std::byte> m_mem;
        // core local memory mapped I/O registers (covers address space not covered by memories)
        std::map<XLEN, std::byte> m_i_o;

//        // constructor/destructor
//        MemoryMap () = default;
//        ~MemoryMap () = default;

        // memory read/write
        std::span<std::byte> read  (const XLEN, const std::size_t) const;
        void                 write (const XLEN, const std::span<std::byte>);
    };

        // read from shadow memory map
    template <typename XLEN, AddressMap AMAP>
    std::span<std::byte> MemoryMap<XLEN, AMAP>::read (
        const XLEN        addr,
        const std::size_t size
    ) const {
        // reading from an address map block
        for (int unsigned blk=0; blk<AMAP.mem.size(); blk++) {
            if ((addr >= AMAP.mem[blk].base) &&
                (addr <  AMAP.mem[blk].base + AMAP.mem[blk].size)) {
                return { m_mem.data() + (addr - AMAP.mem[blk].base), size };
            };
        };
        // reading from an unmapped IO region (reads have higher priority)
        // TODO: handle access to nonexistent entries with a warning?
        // TODO: handle access with a size mismatch
        return m_i_o[addr];
    }

    // write to shadow memory map
    template <typename XLEN, AddressMap AMAP>
    void MemoryMap<XLEN, AMAP>::write (
        const XLEN                 addr,
        const std::span<std::byte> data
    ) {
        // writing to an address map block
        for (int unsigned blk=0; blk<AMAP.size; blk++) {
            if ((addr >= AMAP.mem[blk].base) &&
                (addr <  AMAP.mem[blk].base + AMAP.mem[blk].size)) {
                std::copy(m_mem.data() + (addr - AMAP.mem[blk].base), m_mem.data() + (addr - AMAP.mem[blk].base + data.size()), data.data());
                return;
            };
        };
        // writing to an unmapped IO region (reads have higher priority)
        m_i_o[addr] = data;
    }

}
