///////////////////////////////////////////////////////////////////////////////
// HDLDB SoC/DUT definitions
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#include "Shadow.h"

// choice between 8/16/23/64 bit CPU
using XLEN = uint32_t;
using FLEN = XLEN;

// CPU local memory map
const std::array<AddressBlock<XLEN>, 1> soc_mem = {
    {0x8000'0000, 0x0001'0000}  // base, size
};