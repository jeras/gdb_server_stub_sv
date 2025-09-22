///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow memory map
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C includes
#include <cstddef>

// C++ includes
#include <array>
#include <vector>
#include <map>

// HDLDB includes
#include "AddressMap.h"

namespace HdlDb {

    template <typename XLEN>
    class MemoryMap {
        // core local memories (array of address map regions)
        std::vector<std::byte> m_mem;
        // core local memory mapped I/O registers (covers address space not covered by memories)
        std::map<XLEN, std::byte> m_i_o;

        // constructor/destructor
        MemoryMap (
            const std::array<ArchitectureCore, CNUM>,
            const            ArchitectureSystem
        );
        ~MemoryMap ();

        // memory read/write
        std::vector<std::byte> read  (const XLEN, const std::size_t) const;
        void                   write (const XLEN, const std::vector<std::byte>);
    };

};
