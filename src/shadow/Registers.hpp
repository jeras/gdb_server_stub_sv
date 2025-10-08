///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow registers
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

// C includes
#include <cstddef>

// C++ includes
#include <array>
#include <vector>
#include <span>

namespace shadow {

    template <
        // register widths are defined as types
        typename XLEN,
        typename FLEN,
        typename VLEN,
        // extensions
        bool EXT_E,  // 16 GPR register file
        bool EXT_F,  // floating point
        bool EXT_V,  // vector
        // list of target CSR as seen by GDB
        std::array<bool, 4096> CSR_LIST
    >

    class RegistersRiscV {
        // TODO: E extension
        // const auto NGPR = extE ? 16 : 32;

    private:
        // register files
        std::array<XLEN,   32> m_gpr;  // GPR (general purpose register file)
                   XLEN        m_pc;   // PC  (program counter)
        std::array<FLEN,   32> m_fpr;  // FPR (floating point register file)
        std::array<VLEN,   32> m_vec;  // CSR (configuration status registers)
        std::array<XLEN, 4096> m_csr;  // CSR (configuration status registers)

        // DUT access
        XLEN writeGpr (const unsigned int, const XLEN);
        XLEN readGpr  (const unsigned int) const;
        FLEN writeFpr (const unsigned int, const FLEN);
        FLEN readFpr  (const unsigned int) const;
        VLEN writeVec (const unsigned int, const VLEN);
        VLEN readVec  (const unsigned int) const;
        XLEN writeCsr (const unsigned int, const XLEN);
        XLEN readCsr  (const unsigned int) const;

        // RSP access
        void writeAll (std::span<std::byte>);
        std::vector<std::byte> readAll () const;
        void writeOne (const unsigned int, std::span<std::byte>);
        std::vector<std::byte> readOne (const unsigned int) const;
    };

};
