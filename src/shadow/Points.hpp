///////////////////////////////////////////////////////////////////////////////
// HDLDB breakpoint/watchpoint/catchpoint
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ includes
#include <map>

// HDLDB includes
#include "Instruction.hpp"

namespace shadow {

    // point type
    enum class PointType : int {
        swbreak   = 0,  // software breakpoint
        hwbreak   = 1,  // hardware breakpoint
        watch     = 2,  // write  watchpoint
        rwatch    = 3,  // read   watchpoint
        awatch    = 4,  // access watchpoint
        replaylog = 5,  // reached replay log edge
        none      = -1  // no reason is given
    };

    using PointKind = unsigned int;

    struct Point {
        PointType type;
        PointKind kind;
    };

    template <typename XLEN, typename FLEN, typename VLEN>
    class Points {
        std::map<XLEN, Point> m_break;
        std::map<XLEN, Point> m_watch;

        int insert (const PointType, const XLEN , const PointKind);
        int remove (const PointType, const XLEN , const PointKind);
        bool match (Retired<XLEN, FLEN, VLEN> ret);
    };

};
