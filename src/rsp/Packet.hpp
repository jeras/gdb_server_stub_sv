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
#include <array>
#include <print>

// HDLDB includes
#include "Socket.h"

namespace rsp {

    class Packet : public Socket {
        // acknowledge presence/absence
        bool m_acknowledge;
        // remote communication log
        bool m_log;

        std::string get () const;
        int put (const std::string) const;
    };
};
