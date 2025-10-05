///////////////////////////////////////////////////////////////////////////////
// HDLDB breakpoint/watchpoint/catchpoint
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// HDLDB includes
#include "Points.hpp"

// C++ includes
#include <map>

namespace shadow {

    // RSP insert breakpoint/watchpoint into dictionary
    template <typename XLEN, typename FLEN>
    int Points<XLEN, FLEN>::insert (
        const PointType type,
        const XLEN      addr,
        const PointKind kind
    ) {
        switch (type) {
            case swbreak:
            case hwbreak:
                m_break.insert(addr) = {type, kind};
                return m_break.size();
            case watch:
            case rwatch:
            case awatch:
                m_watch.insert(addr) = {type, kind};
                return m_watch.size();
        };
    };

    // RSP remove breakpoint/watchpoint from dictionary
    template <typename XLEN, typename FLEN>
    int Points<XLEN, FLEN>::remove (
        const PointType type,
        const XLEN      addr,
        const PointKind kind
    ) {
        switch (type) {
            case swbreak:
            case hwbreak:
                m_break.erase(addr);
                return m_break.size();
            case watch:
            case rwatch:
            case awatch:
                m_watch.erase(addr);
                return m_watch.size();
        };
    };

    // match breakpoint/watchpoint
    template <typename XLEN, typename FLEN>
    int Points<XLEN, FLEN>::match (Retired<XLEN, FLEN> ret) {
        // TODO: this is RISC-V specific, should be moved out
        constexpr std::array<std::byte, 4>   ebreak = {std::byte{0x73}, std::byte{0x00}, std::byte{0x10}, std::byte{0x00}};  // 32'h00100073
        constexpr std::array<std::byte, 2> c_ebreak = {std::byte{0x02}, std::byte{0x90}};                                    // 16'h9002

        XLEN addr;

                                 addr = ret.ifu.adr;
        std::array<std::byte, 4> inst = ret.ifu.rdt;
        // match illegal instruction
        if (ret.ifu.ill) {
            signal = SIGILL;
    //            debug("DEBUG: Triggered illegal instruction at address %h.", addr);
            return true;
        }

        // match software breakpoint
        // match EBREAK/C.EBREAK instruction
        // TODO: there are also explicit SW breakpoints that depend on ILEN
        else if (std::equal(  ebreak.begin(),   ebreak.end(), inst) ||
                 std::equal(c_ebreak.begin(), c_ebreak.end(), inst)) {
            signal = SIGTRAP;
    //            $display("DEBUG: Triggered SW breakpoint at address %h.", addr);
            return true;
        }

        // match hardware breakpoint
        else if (break.contains(addr)) {
            if (m_break[addr].type == m_break::hwbreak) {
                // signal
                signal = SIGTRAP;
                // reason
    //            reason = m_break[addr].type;
                reason = 3;
    //            $display("DEBUG: Triggered HW breakpoint at address %h.", addr);
                return true;
            };
        };

             addr = ret.lsu.adr;
        bool rena = ret.lsu.rdt.size() > 0;
        bool wena = ret.lsu.wdt.size() > 0;

        // match hardware breakpoint
        if (watch.contains(addr)) {
            if (((m_watch[addr].ptype == m_watch::watch ) && wena) ||
                ((m_watch[addr].ptype == m_watch::rwatch) && rena) ||
                ((m_watch[addr].ptype == m_watch::awatch) )) {
                // TODO: check is transfer size matches
                // signal
                signal = SIGTRAP;
                // reason
                reason = m_watch[addr].type;
    //            $display("DEBUG: Triggered HW watchpoint at address %h.", addr);
                return true;
            };
        };
    };

};
