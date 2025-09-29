///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow memory map
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// HDLDB includes
#include "AddressMap.hpp"
#include "MemoryMap.hpp"

namespace HdlDb {

    // read from shadow memory map
    template <typename XLEN>
    std::vector<std::byte> MemoryMap<XLEN>::read (
        const XLEN        addr,
        const std::size_t size
    ) const {
        std::vector<std::byte> tmp;
        // reading from an address map block
        for (int unsigned blk=0; blk<MMAP.size(); blk++) {
            if ((addr >= MMAP[blk].base) &&
                (addr <  MMAP[blk].base + MMAP[blk].size)) {
//                tmp = {>>{mem[blk] with [adr - MMAP[blk].base +: siz]}};
                return tmp;
            };
        };
        // reading from an unmapped IO region (reads have higher priority)
        // TODO: handle access to nonexistent entries with a warning?
        // TODO: handle access with a size mismatch
        tmp = m_i_o[adr];
        return tmp;
    };

    // write to shadow memory map
    template <typename XLEN>
    void MemoryMap<XLEN>::write (
        const XLEN                   addr,
        const std::vector<std::byte> data
    ) {
        // writing to an address map block
        for (int unsigned blk=0; blk<MMAP.size; blk++) {
            if ((addr >= MMAP[blk].base) &&
                (addr <  MMAP[blk].base + MMAP[blk].size)) {
//                {>>{mem[blk] with [adr - MMAP[blk].base +: dat.size()]}} = dat;
    //            for (int unsigned i=0; i<dat.size(); i++) begin: byt
    //              mem[blk][adr - MMAP[blk].base] = dat[i];
    //            end: byt
                return;
            };
        };
        // writing to an unmapped IO region (reads have higher priority)
        m_i_o[adr] = dat;
    };

};
