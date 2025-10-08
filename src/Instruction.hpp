///////////////////////////////////////////////////////////////////////////////
// HDLDB retired instruction
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C includes
#include <cstdint>
#include <cstddef>

// C++ includes
#include <array>
#include <vector>

// TODO: this is RISC-V specific, should be moved out
constexpr std::array<std::byte, 4>   ebreak = {std::byte{0x73}, std::byte{0x00}, std::byte{0x10}, std::byte{0x00}};  // 32'h00100073
constexpr std::array<std::byte, 2> c_ebreak = {std::byte{0x02}, std::byte{0x90}};                                    // 16'h9002

// IFU (instruction fetch unit)
template <typename XLEN>
struct RetiredIfu {
    XLEN                   adr;  // instruction address (current PC)
    XLEN                   pcn;  // next PC
    std::vector<std::byte> rdt;  // read data (current instruction)
    bool                   ill;  // illegal instruction (DUT behavior might not be defined)
    // TODO: add interrupts
};

// GPR (general purpose registers)
template <typename XLEN>
struct RetiredGpr {
    uint8_t                idx;  // GPR index
    std::vector<XLEN>      rdt;  // read  data (current destination register value)
    std::vector<XLEN>      wdt;  // write data (next    destination register value)
};

// FPR (floating point registers)
template <typename FLEN>
struct RetiredFpr {
    uint8_t                idx;  // FPR index
    FLEN                   rdt;  // read  data (current destination register value)
    FLEN                   wdt;  // write data (next    destination register value)
};

// VEC (Vector registers)
template <typename VLEN>
struct RetiredVec {
    uint8_t                idx;  // FPR index
    VLEN                   rdt;  // read  data (current destination register value)
    VLEN                   wdt;  // write data (next    destination register value)
};

// CSR (control status registers)
template <typename XLEN>
struct RetiredCsr {
    uint16_t               idx;  // CSR index
    XLEN                   rdt;  // read  data
    XLEN                   wdt;  // write data
};

// LSU (load store unit)
// (memory/IO access size is encoded in array size)
template <typename XLEN>
struct RetiredLsu {
    XLEN      adr;     // data address
    std::vector<std::byte> rdt;  // read  data
    std::vector<std::byte> wdt;  // write data
};

// instruction retirement history log entry
template <typename XLEN, typename FLEN, typename VLEN>
struct Retired {
    RetiredIfu<XLEN> ifu;
    RetiredGpr<XLEN> gpr;
    RetiredFpr<FLEN> fpr;
    RetiredVec<VLEN> vec;
    RetiredCsr<XLEN> csr;
    RetiredLsu<XLEN> lsu;
};
