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
#include <bitset>

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

    // calculating the lengths for each register file
    template <IsaRiscV ISA> constexpr std::size_t lenGpr { ISA.EXT.E ? 16 : 32 };  // GPR
    template <IsaRiscV ISA> constexpr std::size_t lenPc  {              1      };  // PC
    template <IsaRiscV ISA> constexpr std::size_t lenFpr { ISA.EXT.F ? 32 :  0 };  // FPR
    template <IsaRiscV ISA> constexpr std::size_t lenVec { ISA.EXT.V ? 32 :  0 };  // VEC
//  template <IsaRiscV ISA> constexpr std::size_t lenCsr { ISA.CSR.count()     };  // CSR
    template <IsaRiscV ISA> constexpr std::size_t lenCsr { std::count(ISA.CSR.begin(), ISA.CSR.end(), true) };  // CSR

    // calculating the size of std::byte array for debugger g/G packets
    template <typename LEN, IsaRiscV ISA> constexpr std::size_t sizeGpr { lenGpr<ISA> * sizeof(LEN) };  // GPR
    template <typename LEN, IsaRiscV ISA> constexpr std::size_t sizePc  { lenPc <ISA> * sizeof(LEN) };  // PC
    template <typename LEN, IsaRiscV ISA> constexpr std::size_t sizeFpr { lenFpr<ISA> * sizeof(LEN) };  // FPR
    template <typename LEN, IsaRiscV ISA> constexpr std::size_t sizeVec { lenVec<ISA> * sizeof(LEN) };  // VEC
    template <typename LEN, IsaRiscV ISA> constexpr std::size_t sizeCsr { lenCsr<ISA> * sizeof(LEN) };  // CSR

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    constexpr static std::size_t sizeAll {
        sizeGpr<XLEN, ISA> + 
        sizePc <XLEN, ISA> + 
        sizeFpr<FLEN, ISA> + 
        sizeVec<VLEN, ISA> + 
        sizeCsr<VLEN, ISA>
    };

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    class RegistersRiscV {

        // byte arrays used for debugger g/G packets
        std::array<std::byte, sizeAll<XLEN, FLEN, VLEN, ISA>> m_all;

        // register files
        std::span<XLEN> m_gpr { reinterpret_cast<XLEN *>(m_all.data() + 0                                                                               ), lenGpr<ISA> };
//                XLEN& m_pc  { reinterpret_cast<XLEN *>(m_all.data() + sizeGpr<XLEN, ISA>                                                              )              };
                  XLEN  m_pc  { 0 };
        std::span<FLEN> m_fpr { reinterpret_cast<FLEN *>(m_all.data() + sizeGpr<XLEN, ISA> + sizePc<XLEN, ISA>                                          ), lenFpr<ISA> };
        std::span<VLEN> m_vec { reinterpret_cast<VLEN *>(m_all.data() + sizeGpr<XLEN, ISA> + sizePc<XLEN, ISA> + sizeFpr<FLEN, ISA>                     ), lenVec<ISA> };
        std::span<XLEN> m_csr { reinterpret_cast<XLEN *>(m_all.data() + sizeGpr<XLEN, ISA> + sizePc<XLEN, ISA> + sizeFpr<FLEN, ISA> + sizeVec<VLEN, ISA>), lenCsr<ISA> };

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
        std::copy(data.begin(), data.end(), m_all.data());
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    std::span<std::byte> RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readAll () {
        return { m_all.data(), m_all.size() };
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    void RegistersRiscV<XLEN, FLEN, VLEN, ISA>::writeOne (const unsigned int index, std::span<std::byte> data) {
    }

    template <typename XLEN, typename FLEN, typename VLEN, IsaRiscV ISA>
    std::span<std::byte> RegistersRiscV<XLEN, FLEN, VLEN, ISA>::readOne (const unsigned int index) {
        int idx = index;
        if (idx < lenGpr<ISA>)  return { reinterpret_cast<std::byte *>(m_gpr[idx]), sizeof(XLEN) };  idx -= lenGpr<ISA>;
        if (idx < 1          )  return { reinterpret_cast<std::byte *>(m_pc      ), sizeof(XLEN) };  idx -= 1          ;
        if (idx < lenFpr<ISA>)  return { reinterpret_cast<std::byte *>(m_fpr[idx]), sizeof(FLEN) };  idx -= lenFpr<ISA>;
        if (idx < lenVec<ISA>)  return { reinterpret_cast<std::byte *>(m_vec[idx]), sizeof(VLEN) };  idx -= lenVec<ISA>;
        if (idx < lenCsr<ISA>)  return { reinterpret_cast<std::byte *>(m_csr[idx]), sizeof(XLEN) };  idx -= lenCsr<ISA>;
        return { std::span<std::byte>{ } };
    }

}
