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
#include <array>
#include <vector>
#include <print>
#include <stdexcept>
#include <iostream>

// HDLDB includes
#include "Packet.hpp"

namespace rsp {

    void Packet::log (std::string_view packet_data) const {
        std::cout << packet_data;
    }

    std::string_view Packet::rx (bool acknowledge) {
        ssize_t status;
        size_t size = 0;
        do {
            status = recv({m_buffer.data() + size, m_buffer.size() - size}, 0);
            size += status;
        } while (m_buffer[size-3] != static_cast<std::byte>('#'));

        std::string_view packet { reinterpret_cast<char const*>(m_buffer.data()), static_cast<size_t>(size) };
        std::string_view packet_data     = packet.substr(1, size-4);
        std::string_view packet_checksum = packet.substr(size-2, 2);
        uint8_t checksum_pkt = static_cast<uint8_t>(std::stoi(packet_checksum.data(), nullptr, 16));

        log(std::format("REMOTE: <- {}\n", packet_data));

        // calculate payload checksum
        std::span payload { reinterpret_cast<uint8_t const*>(packet_data.data()), packet_data.size() };
        uint8_t checksum_ref { static_cast<uint8_t>(std::accumulate(payload.begin(), payload.end(), 0)) };

        // Verify checksum
        if (checksum_pkt == checksum_ref) {
            if (acknowledge)  send(ACK, 0);
        } else {
            if (acknowledge)  send(NACK, 0);
            throw std::runtime_error { "Sending NACK (due to parity error)." };
        }
        return packet_data;
    }

    void Packet::tx (std::string_view packet_data, bool acknowledge) const {
        log(std::format("REMOTE: -> {}\n", packet_data));

        // calculate payload checksum
        std::span payload { reinterpret_cast<uint8_t const*>(packet_data.data()), packet_data.size() };
        uint8_t checksum { static_cast<uint8_t>(std::accumulate(cbegin(payload), cend(payload), 0)) };

        // format packet
        std::string packet { std::format("${}#{:02x}", packet_data, checksum) };

        // send packet
        ssize_t status;
        size_t size = 0;
        do {
            status = send({ reinterpret_cast<std::byte const*>(packet.data() + size), packet.size() - size }, 0);
            size += status;
        } while (size < packet.size());

        // check acknowledge
        if (acknowledge) {
            std::array<std::byte, 1> ack;
            ssize_t status = recv(ack, 0);
            if (ack == NACK)  throw std::runtime_error { "Received NACK." };
        }
    }
}
