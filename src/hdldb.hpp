///////////////////////////////////////////////////////////////////////////////
// HDLDB configuration
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

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

constexpr std::array<bool, 4096> CsrListHdlDb { false };

using PointHdlDb = shadow::Points<XlenHdlDb, FlenHdlDb, VlenHdlDb>;

using RegsHdlDb = shadow::RegistersRiscV<XlenHdlDb, FlenHdlDb, VlenHdlDb, false, false, false, CsrListHdlDb>;

constexpr shadow::AddressBlock<XlenHdlDb> memCore0HdlDb { 0x8000'0000, 0x0001'0000 };
constexpr shadow::AddressBlock<XlenHdlDb> i_oCore0HdlDb { 0x8001'0000, 0x0001'0000 };

constexpr shadow::AddressBlockArray<XlenHdlDb, 1> memCoreHdlDb { memCore0HdlDb };
constexpr shadow::AddressBlockArray<XlenHdlDb, 1> i_oCoreHdlDb { i_oCore0HdlDb };

constexpr shadow::AddressMap<XlenHdlDb, 1, 1> AmapCoreHdlDb { memCoreHdlDb, i_oCoreHdlDb };

using MmapCoreHdlDb = shadow::MemoryMap<XlenHdlDb, AmapCoreHdlDb>;

using CoreHdlDb = shadow::Core<RegsHdlDb, MmapCoreHdlDb, PointHdlDb>;

constexpr shadow::AddressBlock<XlenHdlDb> memSystem0HdlDb { 0x8002'0000, 0x0001'0000 };
constexpr shadow::AddressBlock<XlenHdlDb> i_oSystem0HdlDb { 0x8003'0000, 0x0001'0000 };

constexpr shadow::AddressBlockArray<XlenHdlDb, 1> memSystemHdlDb { memSystem0HdlDb };
constexpr shadow::AddressBlockArray<XlenHdlDb, 1> i_oSystemHdlDb { i_oSystem0HdlDb };

constexpr shadow::AddressMap<XlenHdlDb, 1, 1> AmapSystemHdlDb { memSystemHdlDb, i_oSystemHdlDb };

using MmapSystemHdlDb = shadow::MemoryMap<XlenHdlDb, AmapSystemHdlDb>;

using SystemHdlDb = shadow::System<XlenHdlDb, FlenHdlDb, VlenHdlDb, CoreHdlDb, MmapSystemHdlDb, PointHdlDb>;

using ProtocolHdlDb = rsp::Protocol<XlenHdlDb, SystemHdlDb>;