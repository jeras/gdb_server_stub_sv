///////////////////////////////////////////////////////////////////////////////
// remote serial protocol
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

// C++ includes
#include <numeric>
#include <string>
#include <string_view>
#include <utility>

namespace rsp {

    // thread ID
    struct ThreadId {
        int pid;
        int tid; 
    };

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

    // point kind
    using PointKind = unsigned int;

    constexpr auto lit2hash (std::string_view str) {
        return std::accumulate(str.begin(), str.end(), 0);
    }
//constexpr auto switch_hash(std::string_view str) {
////    return std::hash<std::string_view>{}(str);
//    return std::accumulate(str.begin(), str.end(), 0);
//}

//    template <typename XLEN, typename SHADOW>
//    unsigned int constexpr Protocol<XLEN, SHADOW>::lit2hash(char const* input) {
//        return *input ? static_cast<unsigned int>(*input) + 33 * lit2hash(input + 1) : 5381;
//    }



}
