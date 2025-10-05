///////////////////////////////////////////////////////////////////////////////
// HDLDB configuration
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C includes
#include <cstdint>

// C++ includes
#include <string>

// HDLDB includes
#include "System.hpp"
#include "Protocol.hpp"

using XlenHdlDb = std::uint32_t;
using FlenHdlDb = std::uint32_t;
using VlenHdlDb = std::uint32_t;  // TODO: placeholder

using PointHdlDb = shadow::Points<XlenHdlDb, FlenHdlDb>;

using RegsHdlDb = shadow::RegistersRiscV<XlenHdlDb, FlenHdlDb, VlenHdlDb, false, false, false>;

constexpr shadow::AddressMap<XlenHdlDb, 1, 1> AmapCoreHdlDb { { { 0x8000'0000, 0x0001'0000 } }, { { 0x8001'0000, 0x0001'0000 } } };

using MmapCoreHdlDb = shadow::MemoryMap<XlenHdlDb, AmapCoreHdlDb>;

using CoreHdlDb = shadow::Core<XlenHdlDb, FlenHdlDb, RegsHdlDb, MmapCoreHdlDb, PointHdlDb>;

constexpr shadow::AddressMap<XlenHdlDb, 1, 1> AmapSystemHdlDb { { { 0x8002'0000, 0x0001'0000 } }, { { 0x8003'0000, 0x0001'0000 } } };

using MmapSystemHdlDb = shadow::MemoryMap<XlenHdlDb, AmapSystemHdlDb>;

template <typename XLEN, typename FLEN, typename CORE, typename MMAP, typename POINT>

using SystemHdlDb = System<XlenHdlDb, FlenHdlDb, CoreHdlDb, MmapSystemHdlDb, PointHdlDb>;
