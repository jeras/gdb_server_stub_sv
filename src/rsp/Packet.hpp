///////////////////////////////////////////////////////////////////////////////
// RSP (remote serial protocol) packet
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

//    ////////////////////////////////////////
//    // RSP character get/put
//    ////////////////////////////////////////
//
//        function automatic void rsp_write (string str);
//            int status;
//            byte buffer [] = new[str.len()](byte_array_t'(str));
//            status = socket_send(buffer, 0);
//        endfunction: rsp_write
//
//    ////////////////////////////////////////
//    // RSP packet get/send
//    ////////////////////////////////////////

// C++ includes
#include <string>

// HDLDB includes
#include "Socket.hpp"

namespace rsp {

    class Packet : public Socket {
        const std::array<std::byte, 1>  ACK { static_cast<std::byte>('+') };
        const std::array<std::byte, 1> NACK { static_cast<std::byte>('-') };

        std::array<std::byte, 512> m_buffer;

        // acknowledge presence/absence
        bool m_acknowledge;
        // remote communication log
        std::string m_log;

        // logging
        void log(std::string_view) const { };

        // handling packets
        std::string_view get ();
        void put (std::string_view) const;
    };
};
