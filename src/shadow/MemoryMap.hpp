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
        std::array<std::byte, addressBlockSize(AMAP.mem)> m_buf;
//        std::array<std::byte, 0x1'0000> m_mem;
        std::span<std::byte> m_mem { m_buf };
        // core local memory mapped I/O registers (covers address space not covered by memories)
        std::map<XLEN, XLEN> m_i_o;

//        // constructor/destructor
//        MemoryMap () = default;
//        ~MemoryMap () = default;

        // mapping from CPU address space to shadow memory offset
        XLEN offset (XLEN addr) const;
    public:
        // memory load/store from CPU
        template <typename TYPE>
        TYPE load  (const XLEN addr);
        template <typename TYPE>
        void store (const XLEN addr, TYPE data);

        // memory read/write from debugger
        std::span<std::byte> read  (const XLEN addr, const std::size_t size) const;
        void                 write (const XLEN addr, const std::span<std::byte> data);
    };

    // mapping from CPU address space to shadow memory offset
    template <typename XLEN, AddressMap AMAP>
    XLEN MemoryMap<XLEN, AMAP>::offset (XLEN addr) const {
        for (int unsigned blk=0; blk<AMAP.mem.size(); blk++) {
            if ((addr >= AMAP.mem[blk].base) &&
                (addr <  AMAP.mem[blk].base + AMAP.mem[blk].size)) {
                return addr-AMAP.mem[blk].base;
            };
        };
        return { };
    }

    // memory load/store from CPU
    template <typename XLEN, AddressMap AMAP>
    template <typename TYPE>
    TYPE MemoryMap<XLEN, AMAP>::load (const XLEN addr) {
        return static_cast<TYPE>(m_mem.data() + offset(addr));
    }

    template <typename XLEN, AddressMap AMAP>
    template <typename TYPE>
    void MemoryMap<XLEN, AMAP>::store (const XLEN addr, TYPE data) {
        static_cast<TYPE>(m_mem.data() + offset(addr)) = data;
    }

    // read from shadow memory map
    template <typename XLEN, AddressMap AMAP>
    std::span<std::byte> MemoryMap<XLEN, AMAP>::read (
        const XLEN        addr,
        const std::size_t size
    ) const {
        // reading from an address map block
        return m_mem.subspan(offset(addr), size);
        // reading from an unmapped IO region (reads have higher priority)
        // TODO: handle access to nonexistent entries with a warning?
        // TODO: handle access with a size mismatch
//        return { static_cast<std::byte *>(m_i_o[addr]), sizeof(XLEN) };
        return { };
    }

    // write to shadow memory map
    template <typename XLEN, AddressMap AMAP>
    void MemoryMap<XLEN, AMAP>::write (
        const XLEN                 addr,
              std::span<std::byte> data
    ) {
        // writing to an address map block
        std::copy_n(data.data(), data.size(), m_mem.data() + offset(addr));
        // writing to an unmapped IO region (reads have higher priority)
        // TODO
//        m_i_o[addr] = data;
    }

}
