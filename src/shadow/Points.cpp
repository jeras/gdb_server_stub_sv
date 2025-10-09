///////////////////////////////////////////////////////////////////////////////
// HDLDB breakpoint/watchpoint/catchpoint
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C includes
#include <signal.h>

// C++ includes
#include <map>

// HDLDB includes
#include "Points.hpp"


namespace shadow {

    // RSP insert breakpoint/watchpoint into dictionary
    template <typename XLEN, typename FLEN, typename VLEN>
    int Points<XLEN, FLEN, VLEN>::insert (
        const PointType type,
        const XLEN      addr,
        const PointKind kind
    ) {
        switch (type) {
            case PointType::swbreak:
            case PointType::hwbreak:
                m_break.insert(addr) = {type, kind};
                return m_break.size();
            case PointType::watch:
            case PointType::rwatch:
            case PointType::awatch:
                m_watch.insert(addr) = {type, kind};
                return m_watch.size();
            default:
        };
    };

    // RSP remove breakpoint/watchpoint from dictionary
    template <typename XLEN, typename FLEN, typename VLEN>
    int Points<XLEN, FLEN, VLEN>::remove (
        const PointType type,
        const XLEN      addr,
        const PointKind kind
    ) {
        switch (type) {
            case PointType::swbreak:
            case PointType::hwbreak:
                m_break.erase(addr);
                return m_break.size();
            case PointType::watch:
            case PointType::rwatch:
            case PointType::awatch:
                m_watch.erase(addr);
                return m_watch.size();
            default:
        };
    };

    // match breakpoint/watchpoint
    template <typename XLEN, typename FLEN, typename VLEN>
    bool Points<XLEN, FLEN, VLEN>::match (Retired<XLEN, FLEN, VLEN> ret) {
        XLEN addr;

                                 addr = ret.ifu.adr;
//        std::array<std::byte, 4> inst = ret.ifu.rdt;
        // match illegal instruction
        if (ret.ifu.ill) {
            m_signal = SIGILL;
    //            debug("DEBUG: Triggered illegal instruction at address %h.", addr);
            return true;
        }

//        // match software breakpoint
//        // match EBREAK/C.EBREAK instruction
//        // TODO: this is RISC-V specific code
//        else if (std::equal(ebreak.begin(), ebreak.end(), inst)) {
//            m_signal = SIGTRAP;
//            m_reason = {PointType::swbreak, 4};
//    //            $display("DEBUG: Triggered SW breakpoint at address %h.", addr);
//            return true;
//        }
//        else if (std::equal(c_ebreak.begin(), c_ebreak.end(), inst)) {
//            m_signal = SIGTRAP;
//            m_reason = {PointType::swbreak, 2};
//    //            $display("DEBUG: Triggered SW breakpoint at address %h.", addr);
//            return true;
//        }

        // match hardware breakpoint
        else if (m_break.contains(addr)) {
            if (m_break[addr].type == PointType::hwbreak) {
                // signal
                m_signal = SIGTRAP;
                // reason
                m_reason = m_break[addr];
    //            $display("DEBUG: Triggered HW breakpoint at address %h.", addr);
                return true;
            };
        };

             addr = ret.lsu.adr;
        bool rena = ret.lsu.rdt.size() > 0;
        bool wena = ret.lsu.wdt.size() > 0;

        // match hardware breakpoint
        if (m_watch.contains(addr)) {
            if (((m_watch[addr].ptype == PointType::watch ) && wena) ||
                ((m_watch[addr].ptype == PointType::rwatch) && rena) ||
                ((m_watch[addr].ptype == PointType::awatch) )) {
                // TODO: check is transfer size matches
                // signal
                m_signal = SIGTRAP;
                // reason
                m_reason = m_watch[addr];
    //            $display("DEBUG: Triggered HW watchpoint at address %h.", addr);
                return true;
            };
        };
    };

};
