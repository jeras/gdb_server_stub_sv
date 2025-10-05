///////////////////////////////////////////////////////////////////////////////
// HDLDB shadow (DUT shadow copy)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C includes
#include <cstdint>
#include <csignal>

// C++ includes
#include <string>
#include <vector>
#include <array>
#include <span>
#include <map>
#include <bitset>
#include <bit>
#include <utility>

// HDLDB includes
#include "Registers.hpp"
#include "Instruction.hpp"
#include "Point.hpp"
#include "MemoryMap.hpp"

namespace shadow {

    template <typename XLEN, typename FLEN, unsigned int CNUM>
    class Shadow {

        ///////////////////////////////////////
        // type definitions
        ///////////////////////////////////////

        // SoC system architecture
        struct ArchitectureSystem {
            std::array<ArchitectureCore, CNUM> cpu;
            // memory map (shadow memory map)
            std::vector<MemoryBlock> map;
        };

        ////////////////////////////////////////
        // core shadow
        ////////////////////////////////////////

        // shadow state structure
        struct ShadowCore {
            // register files
            RegistersRiscV<XLEN, FLEN> reg;
            // core local memories (array of address map regions)
            std::vector<Memory>  mem;
            // core local memory mapped I/O registers (covers address space not covered by memories)
            std::map<XLEN, byte> i_o;

            // associative array for per thread hardware breakpoints/watchpoint
            Points<XLEN> breakpoints;
            Points<XLEN> watchpoints;

            // instruction counter
            size_t     cnt;
            // current retired instruction
            Retired    ret;
            // signal
            int        signal;
            // reason (point type/kind)
    //        Points<XLEN>::PointType reason;
            int        reason;
        };

        ////////////////////////////////////////
        // system shadow
        ////////////////////////////////////////

        struct ShadowSystem {
            // shadow of individual CPU cores
            std::array<ArchitectureCore, CNUM> archCore;

            // system shared memories (array of address map regions)
            std::vector<Memory>  mem;
            // system shared memory mapped I/O registers (covers address space not covered by memories)
            std::map<XLEN, byte> i_o;

            // associative array for all threads hardware breakpoints/watchpoint
            Points<XLEN> breakpoints;
            Points<XLEN> watchpoints;

            // time


            // trace queue
            vector<Retired> trc;
        };

        ///////////////////////////////////////
        // architecture and shadow configuration array
        ///////////////////////////////////////

        ArchitectureSystem  arch;
        ShadowSystem        shadow;

    public:
        // constructor/destructor
        Shadow (const std::array<ArchitectureCore, CNUM>, ArchitectureSystem);
        ~Shadow ();
        // memory read/write
        std::vector<std::byte> memRead (XLEN, int unsigned);
        void                   memWrite(XLEN, std::vector<std::byte>);
        // breakpoint/watchpoint/catchpoint
        bool matchPoint(Retired);
    };

};
