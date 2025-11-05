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
#include <fstream>
#include <iterator>

// HDLDB includes
#include <rsp.hpp>
//#include "Instruction.hpp"
#include "Core.hpp"
#include "Points.hpp"

namespace shadow {

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    class System {

    public:
        MMAP m_mmap;

        CORE m_core;

        POINT m_point;

        // time, unit is 1fs in VHDL and user defined in Verilog
        std::int64_t time;

        // trace file position
        std::size_t position;

        // trace queue
        std::vector<Retired<XLEN, FLEN, VLEN>> m_trace;

//    public:
//        // constructor/destructor
//        System () = default;
//        ~System () = default;

        // register read/write
        std::span<std::byte> reg_readAll (const rsp::ThreadId threadId);
        void                 reg_writeAll(const rsp::ThreadId threadId, std::span<std::byte> data);
        std::span<std::byte> reg_readOne (const rsp::ThreadId threadId, const unsigned int index);
        void                 reg_writeOne(const rsp::ThreadId threadId, const unsigned int index, const std::span<std::byte> data);

        // memory read/write
        std::span<std::byte> mem_read (const rsp::ThreadId threadId, const XLEN addr, const std::size_t size);
        void                 mem_write(const rsp::ThreadId threadId, const XLEN addr, std::span<std::byte> data);

        // point insert/remove/match
        int pointInsert (const rsp::ThreadId threadId, const rsp::PointType, const XLEN , const rsp::PointKind);
        int pointRemove (const rsp::ThreadId threadId, const rsp::PointType, const XLEN , const rsp::PointKind);
        bool pointMatch (const rsp::ThreadId threadId, Retired<XLEN, FLEN, VLEN> ret);

        // snapshot load
        void snapshotLoad (const std::string& filename);
    };

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    std::span<std::byte> System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::reg_readAll (const rsp::ThreadId threadId) {
        return m_core.readAll();
    }

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    void System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::reg_writeAll (const rsp::ThreadId threadId, const std::span<std::byte> data) {
        m_core.writeAll(data);
    }

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    std::span<std::byte> System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::reg_readOne (const rsp::ThreadId threadId, const unsigned int index) {
        return m_core.readOne(index);
    }

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    void System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::reg_writeOne (const rsp::ThreadId threadId, const unsigned int index, std::span<std::byte> data) {
        m_core.writeOne(index, data);
    }


    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    std::span<std::byte> System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::mem_read (const rsp::ThreadId threadId, const XLEN addr, const std::size_t size) {
        return m_core.read(addr, size);
    }

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    void System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::mem_write (const rsp::ThreadId threadId, const XLEN addr, std::span<std::byte> data) {
        m_core.write(addr, data);
    }


    // point insert/remove/match
    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    int System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::pointInsert (const rsp::ThreadId threadId, const rsp::PointType, const XLEN , const rsp::PointKind) {
        return 0;
    }

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    int System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::pointRemove (const rsp::ThreadId threadId, const rsp::PointType, const XLEN , const rsp::PointKind) {
        return 0;
    }

    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    bool System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::pointMatch (const rsp::ThreadId threadId, Retired<XLEN, FLEN, VLEN> ret) {
        return false;
    }


    template <typename XLEN, typename FLEN, typename VLEN, typename CORE, typename MMAP, typename POINT>
    void System<XLEN, FLEN, VLEN, CORE, MMAP, POINT>::snapshotLoad (const std::string& filename) {
        // open input snapshot file in binary mode
        std::ifstream snapshot { filename, std::ios::binary };
        std::istreambuf_iterator<char> iter { snapshot };
        // copy time
        std::copy_n(iter, sizeof(time), reinterpret_cast<char *>(&time));
        // copy trace file position
        std::copy_n(iter, sizeof(position), reinterpret_cast<char *>(&position));
        // copy registers
        std::copy_n(iter, m_core.m_all.size(), m_core.m_all.data());
        // copy core memory
        std::copy_n(iter, m_core.m_buf.size(), m_core.m_buf.data());
        // copy system memory
        // TODO
    }

}
