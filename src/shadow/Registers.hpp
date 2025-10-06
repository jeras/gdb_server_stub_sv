///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow registers
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

namespace shadow {

    //
    enum class RegisterEnum : int {GPR, PC, FPR, VEC, CSR};

    template <
        // register widths are defined as types
        typename XLEN,
        typename FLEN,
        typename VLEN,
        // extensions
        bool extE,  // 16 GPR register file
        bool extF,  // floating point
        bool extV   // vector
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
        XLEN write (const RegisterEnum, const unsigned int, const XLEN);
        XLEN read  (const RegisterEnum, const unsigned int) const;

        // RSP access
        void set (const unsigned int, const XLEN);
        XLEN get (const unsigned int) const;
    };

};
