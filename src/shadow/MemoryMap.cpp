///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow memory map
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// HDLDB includes
#include "MemoryMap.hpp"

namespace shadow {

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
                return { m_mem.data() + (adr - AMAP.mem[blk].base), size };
            };
        };
        // reading from an unmapped IO region (reads have higher priority)
        // TODO: handle access to nonexistent entries with a warning?
        // TODO: handle access with a size mismatch
        return m_i_o[adr];
    };

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
                std::copy(m_mem.data() + (adr - AMAP.mem[blk].base), m_mem.data() + (adr - AMAP.mem[blk].base + size), data.data());
                return;
            };
        };
        // writing to an unmapped IO region (reads have higher priority)
        m_i_o[adr] = dat;
    };

};
