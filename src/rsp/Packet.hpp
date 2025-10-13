///////////////////////////////////////////////////////////////////////////////
// RSP (remote serial protocol) packet
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

// C includes
#include <cstddef>

// C++ includes
#include <string>

// HDLDB includes
#include "Socket.hpp"

namespace rsp {

    class Packet : public Socket {
        const std::array<std::byte, 1>  ACK { static_cast<std::byte>('+') };
        const std::array<std::byte, 1> NACK { static_cast<std::byte>('-') };

        std::array<std::byte, 512> m_buffer;

        // remote communication log
        std::string m_log;

        // logging
        void log(std::string_view) const;

    public:
        // constructor
        Packet(std::string_view name) : Socket(name) { };

        // handling packets
        std::string_view rx (bool acknowledge);
        void tx (std::string_view, bool acknowledge) const;
    };

}
