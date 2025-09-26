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
#include <numeric>

// HDLDB includes
#include "Packet.h"

namespace rsp {

    std::string Packet::get () const {
        std::string packet;
        int status;
        int unsigned len;
        std::array<std::byte, 512> buffer;
        std::vector<std::byte> command;
        std::string str {};
        std::byte   checksum {0};
        std::string checksum_ref;
        std::string checksum_str;

        // TODO: error handling?
        do {
            status = recv(buffer, 0);
  //          std::print("DEBUG: rsp_get_packet: buffer = %p", buffer);
            str += string(buffer);
            len = str.size();
  //          std::print("DEBUG: rsp_get_packet: str = %s", str);
        } while (str[len-3] != "#");

        // extract packet data from received string
        packet = str.substr(1,len-4);

        if (m_log) std::println(m_log, "REMOTE: <- {}", packet);

        // calculate packet data checksum
        command = new[len-4](byte_array_t(packet));
        checksum = command.sum();

        // Get checksum now
        checksum_ref = str.substr(len-2,len-1);

        // Verify checksum
        checksum_str = $sformatf("%02h", checksum);
        if (checksum_ref != checksum_str) {
            $error("Bad checksum. Got 0x%s but was expecting: 0x%s for packet '%s'", checksum_ref, checksum_str, packet);
            if (m_acknowledge) {
                // NACK packet
                rsp_write("-");
            }
            return (-1);
        }
        else {
            if (ack) {
                // ACK packet
                rsp_write("+");
            }
            return 0;
        };
    };

    int Packet::put (
        const std::string packet
    ) const {

        if (m_log) std::println(m_log, "REMOTE: -> {}", packet);

        // calculate checksum
        std::byte checksum { std::accumulate(cbegin(packet), cend(packet), 0) };

        // send entire packet to stream
        std::print(stream, "${}#{}", packet, checksum);

        // check acknowledge
        if (m_acknowledge) {
            int status;
            std::byte ch = new[1];
            status = recv(ch, 0);
            if (ch[0] == "+")  return(0);
            else               return(-1);
        };
    };

};
