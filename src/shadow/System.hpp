///////////////////////////////////////////////////////////////////////////////
// HDLDB shadow (DUT shadow copy)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

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
//#include "Instruction.hpp"
#include "Core.hpp"
#include "Points.hpp"

namespace shadow {

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    class System {

        CORE m_core;

        MMAP m_mmap;

        POINT m_point;

        // time

        // trace queue
        std::vector<Retired<XLEN, FLEN, VLEN>> m_trace;

    public:
//        // constructor/destructor
//        System () = default;
//        ~System () = default;

//        // memory read/write
//        std::span<std::byte> read (const int, const XLEN, const std::size_t);
//        void                 write(const int, const XLEN, const std::span<std::byte>);
    };

};
