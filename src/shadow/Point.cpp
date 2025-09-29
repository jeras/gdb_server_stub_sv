///////////////////////////////////////////////////////////////////////////////
// HDLDB breakpoint/watchpoint/catchpoint
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// HDLDB includes
#include "Point.hpp"

// C++ includes
#include <map>

namespace HdlDb {

    // RSP insert breakpoint/watchpoint into dictionary
    template <typename XLEN>
    int Points<XLEN>::insert (
        const PointType type,
        const XLEN      addr,
        const PointKind kind
    ) {
        switch (type) {
            case swbreak:
            case hwbreak:
                breakpoints.insert(addr) = {type, kind};
                return breakpoints.size();
            case watch:
            case rwatch:
            case awatch:
                watchpoints.insert(addr) = {type, kind};
                return watchpoints.size();
        };
    };

    // RSP remove breakpoint/watchpoint from dictionary
    template <typename XLEN>
    int Points<XLEN>::remove (
        const PointType type,
        const XLEN      addr,
        const PointKind kind
    ) const {
        switch (type) {
            case swbreak:
            case hwbreak:
                breakpoints.erase(addr);
                return breakpoints.size();
            case watch:
            case rwatch:
            case awatch:
                watchpoints.erase(addr);
                return watchpoints.size();
        };
    };

};
