///////////////////////////////////////////////////////////////////////////////
// HDLDB shadow (DUT shadow copy)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#include "Shadow.hpp"

namespace shadow {

    // constructor
    template <typename XLEN, typename FLEN, unsigned int CNUM>
    Shadow<XLEN, FLEN, CNUM>::Shadow (
        const std::array<ArchitectureCore, CNUM>  archCore,
        const            ArchitectureSystem archSystem
    ) {
        // architecture
        archCore = arch;

        // initialize DUT shadow copy
        for (unsigned int core=0; core<arch.size(); core++) {
            // memories
            for (unsigned int i=0; i<arch[core].map.size(); i++) {
                shadow[core].mem.push_back(new Memory [arch[core].mem[i].size]);
            }
            // instruction counter
            shadow[core].cnt = 0;
            // signal
            shadow[core].signal = SIGTRAP;
            // reason (point type/kind)
            // TODO
            // reached beginning of trace
            //rsn;
        };
    };

    // destructor
    template <typename XLEN, typename FLEN, unsigned int CNUM>
    Shadow<XLEN, FLEN, CNUM>::~Shadow () {
        // initialize DUT shadow copy
        for (unsigned int core=0; core<arch.size(); core++) {
            // memories
            for (unsigned int i=0; i<arch[core].map.size(); i++) {
                delete shadow[core].mem[i];
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // breakpoint/watchpoint/catchpoint
    ///////////////////////////////////////////////////////////////////////////////

    template <typename XLEN, typename FLEN, unsigned int CNUM>
    bool Shadow<XLEN, FLEN, CNUM>::matchPoint (
        Retired ret
    ) {
        constexpr std::array<std::byte, 4>   ebreak = {std::byte{0x73}, std::byte{0x00}, std::byte{0x10}, std::byte{0x00}};  // 32'h00100073
        constexpr std::array<std::byte, 2> c_ebreak = {std::byte{0x02}, std::byte{0x90}};                                    // 16'h9002

        XLEN addr;

                                 addr = ret.ifu.adr;
        std::array<std::byte, 4> inst = ret.ifu.rdt;
        // match illegal instruction
        if (ret.ifu.ill) {
            shadow.signal = SIGILL;
    //            debug("DEBUG: Triggered illegal instruction at address %h.", addr);
            return true;
        }

        // match software breakpoint
        // match EBREAK/C.EBREAK instruction
        // TODO: there are also explicit SW breakpoints that depend on ILEN
        else if (std::equal(  ebreak.begin(),   ebreak.end(), inst) ||
                 std::equal(c_ebreak.begin(), c_ebreak.end(), inst)) {
            shadow.signal = SIGTRAP;
    //            $display("DEBUG: Triggered SW breakpoint at address %h.", addr);
            return true;
        }

        // match hardware breakpoint
        else if (shadow.breakpoints.contains(addr)) {
            if (shadow.breakpoints[addr].type == shadow.breakpoints::hwbreak) {
                // signal
                shadow.signal = SIGTRAP;
                // reason
    //            reason = shadow.breakpoints[addr].type;
                shadow.reason = 3;
    //            $display("DEBUG: Triggered HW breakpoint at address %h.", addr);
                return true;
            };
        };

             addr = ret.lsu.adr;
        bool rena = ret.lsu.rdt.size() > 0;
        bool wena = ret.lsu.wdt.size() > 0;

        // match hardware breakpoint
        if (shadow.watchpoints.contains(addr)) {
            if (((shadow.watchpoints[addr].ptype == shadow.watchpoints::watch ) && wena) ||
                ((shadow.watchpoints[addr].ptype == shadow.watchpoints::rwatch) && rena) ||
                ((shadow.watchpoints[addr].ptype == shadow.watchpoints::awatch) )) {
                // TODO: check is transfer size matches
                // signal
                shadow.signal = SIGTRAP;
                // reason
                shadow.reason = shadow.watchpoints[addr].type;
    //            $display("DEBUG: Triggered HW watchpoint at address %h.", addr);
                return true;
            };
        };
    };

};
