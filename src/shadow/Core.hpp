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
#include "MemoryMap.hpp"
#include "Points.hpp"

namespace shadow {

    template <typename XLEN, typename FLEN, typename REGS, typename MMAP, typename POINT>
    class Core : REGS, MMAP, POINT {

        // instruction counter
        size_t     cnt = 0;
        // signal
        int        signal = SIGTRAP;
        // reason (point type/kind)
//        Points<XLEN>::PointType reason;
        int        reason;

    public:
        // constructor/destructor
        Core () = default;
        ~Core () = default;
    };

};
