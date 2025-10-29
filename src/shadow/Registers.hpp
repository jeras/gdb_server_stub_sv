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

        // register files
        std::array<XLEN,   32> m_gpr;  // GPR (general purpose register file)
                   XLEN        m_pc;   // PC  (program counter)
        std::array<FLEN,   32> m_fpr;  // FPR (floating point register file)
        std::array<VLEN,   32> m_vec;  // CSR (configuration status registers)
        std::array<XLEN, 4096> m_csr;  // CSR (configuration status registers)

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
        std::vector<std::byte> readAll () const;
        void writeOne (const unsigned int index, std::span<std::byte> data);
        std::vector<std::byte> readOne (const unsigned int index) const;
    };

        template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::writeGpr (const unsigned int index, const XLEN val) {
        return std::exchange(m_gpr[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::readGpr (const unsigned int index) const {
        return m_gpr[index];
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    FLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::writeFpr (const unsigned int index, const FLEN val) {
        return std::exchange(m_fpr[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    FLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::readFpr (const unsigned int index) const {
        return m_fpr[index];
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    VLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::writeVec (const unsigned int index, const VLEN val) {
        return std::exchange(m_vec[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    VLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::readVec (const unsigned int index) const {
        return m_vec[index];
    }


    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::writeCsr (const unsigned int index, const XLEN val) {
        return std::exchange(m_csr[index], val);
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    XLEN RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::readCsr (const unsigned int index) const {
        return m_csr[index];
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    void RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::writeAll (std::span<std::byte> data) {
//        m_gpr.data() = std::array<XLEN, 32> (sizeof(XLEN)*m_gpr.size())
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    std::vector<std::byte> RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::readAll () const {
        std::vector<std::byte> val ( sizeof(XLEN) * (32+1) );
//                    val.append_range({ static_cast<std::byte>(m_gpr.data()), sizeof(XLEN)*m_gpr.size() });
//        if (EXT_F)  val.append_range({ static_cast<std::byte>(m_fpr.data()), sizeof(FLEN)*m_fpr.size() });
//        if (EXT_V)  val.append_range({ static_cast<std::byte>(m_vec.data()), sizeof(VLEN)*m_vec.size() });
//        for (size_t i=0; i<m_csr.size(); i++) {
//            if (CSR_LIST[i]) {
//                val.append_range({ static_cast<std::byte>(m_csr[i]), sizeof(XLEN) });
//            }
//        }
        return val;
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    void RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::writeOne (const unsigned int index, std::span<std::byte> data) {
    }

    template <typename XLEN, typename FLEN, typename VLEN, bool EXT_E, bool EXT_F, bool EXT_V, std::array<bool, 4096> CSR_LIST>
    std::vector<std::byte> RegistersRiscV<XLEN, FLEN, VLEN, EXT_E, EXT_F, EXT_V, CSR_LIST>::readOne (const unsigned int index) const {
        std::vector<std::byte> val {};
        return val;
    }

}
