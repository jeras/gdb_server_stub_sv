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

    // RISC-V extensions
    struct ExtensionsRiscV {
        bool E;  // 16 GPR register file
        bool F;  // floating point
        bool V;  // vector
    };

    // RISC-V ISA
    struct IsaRiscV {
        ExtensionsRiscV EXT;
        // list of target CSR as seen by GDB
        std::array<bool, 4096> CSR;
    };

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    class RegistersRiscV {
        // TODO: E extension
        // const auto NGPR = extE ? 16 : 32;

        // register files
        std::array<XLEN,   32> m_gpr;  // GPR (general purpose register file)
                   XLEN        m_pc;   // PC  (program counter)
        std::array<FLEN,   32> m_fpr;  // FPR (floating point register file)
        std::array<VLEN,   32> m_vec;  // CSR (configuration status registers)
        std::array<XLEN, 4096> m_csr;  // CSR (configuration status registers)

        // byte arrays used for debugger register reads
        std::array<XLEN, 32+1> m_reg;

    public:
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
        std::span<std::byte> readAll ();
        void writeOne (const unsigned int index, std::span<std::byte> data);
        std::span<std::byte> readOne (const unsigned int index);
    };

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::writeGpr (const unsigned int index, const XLEN val) {
        return std::exchange(m_gpr[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readGpr (const unsigned int index) const {
        return m_gpr[index];
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    FLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::writeFpr (const unsigned int index, const FLEN val) {
        return std::exchange(m_fpr[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    FLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readFpr (const unsigned int index) const {
        return m_fpr[index];
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    VLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::writeVec (const unsigned int index, const VLEN val) {
        return std::exchange(m_vec[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    VLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readVec (const unsigned int index) const {
        return m_vec[index];
    }


    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::writeCsr (const unsigned int index, const XLEN val) {
        return std::exchange(m_csr[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readCsr (const unsigned int index) const {
        return m_csr[index];
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    void RegistersRiscV<XLEN, FLEN, VLEN, ISA>::writeAll (std::span<std::byte> data) {
//        m_gpr.data() = std::array<XLEN, 32> (sizeof(XLEN)*m_gpr.size())
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    std::span<std::byte> RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readAll () {
        std::copy(m_gpr.begin(), m_gpr.end(), m_reg.data());
        m_reg[32] = m_pc;
        // cast from XLEN to std::byte
        return { reinterpret_cast<std::byte *>(m_reg.data()), m_reg.size() * sizeof(XLEN) };
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    void RegistersRiscV<XLEN, FLEN, VLEN, ISA>::writeOne (const unsigned int index, std::span<std::byte> data) {
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    std::span<std::byte> RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readOne (const unsigned int index) {
        std::vector<std::byte> val {};
        return static_cast<std::span<std::byte>>(val);
    }

}
