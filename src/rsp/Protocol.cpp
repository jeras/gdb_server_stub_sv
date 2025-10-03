///////////////////////////////////////////////////////////////////////////////
// RSP (remote serial protocol) protocol
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ includes
#include <string>
#include <iostream>
#include <vector>

// HDLDB includes
#include "Protocol.hpp"

namespace rsp {

    template <typename XLEN, typename SHADOW>
    Protocol<XLEN, SHADOW>::Protocol (std::string_view name, SHADOW shadow) :
        Packet(name),
        m_shadow(shadow)
    {
    }

    template <typename XLEN, typename SHADOW>
    Protocol<XLEN, SHADOW>::~Protocol () { };

    template <typename XLEN, typename SHADOW>
    std::string_view Protocol<XLEN, SHADOW>::rx () {
        return Packet::rx(m_state.acknowledge);
    }

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::tx (std::string_view packet) {
        Packet::tx(packet, m_state.acknowledge);
    }

    ////////////////////////////////////////
    // conversion between std::byte and HEX
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    std::vector<std::byte> Protocol<XLEN, SHADOW>::hex2bin (std::string_view hex) const {
        std::vector<std::byte> bin { hex.size()/2 };
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string_view str = hex.substr(i, 2);
            std::byte byte = static_cast<uint8_t>(std::stoi(str.data(), nullptr, 16));
            bin.push_back(byte);
        }
        return bin;
    }

    template <typename XLEN, typename SHADOW>
    std::string Protocol<XLEN, SHADOW>::bin2hex (std::span<std::byte> bin) const {
        return std::format("{::02x}", bin);
    }

    ////////////////////////////////////////
    // RSP memory access (hexadecimal)
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::mem_read (std::string_view packet) {
        // memory address and length
        int code;
        XLEN addr;
        XLEN size;
        switch (sizeof(XLEN)*8) {
            case 32: code = std::sscanf(packet.data(), "m%8x,%8x", &addr, &size);
            case 64: code = std::sscanf(packet.data(), "m%16x,%16x", &addr, &size);
        }

    //    $display("DBG: rsp_mem_read: adr = %08x, len=%08x", adr, len);

        // read from memory
        std::span<std::byte> data;
        if (m_state.dut_memory) {
            data = dut_mem_read(addr, size);
        } else {
            data = m_shadow.mem_read(addr, size);
        }
    //    $display("DBG: rsp_mem_read: pkt = %s", pkt);

        // send response
        tx(bin2hex(data));
    };

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::mem_write   (std::string_view packet) {
        // memory address and length
        int code;
        XLEN adr;
        XLEN len;
        switch (sizeof(XLEN)*8) {
            case 32: code = std::sscanf(packet.data(), "M%8x,%8x:", &adr, &len);
            case 64: code = std::sscanf(packet.data(), "M%16x,%16x:", &adr, &len);
        }
        //    $display("DBG: rsp_mem_write: adr = 'h%08h, len = 'd%0d", adr, len);

        // remove the header from the packet, only data remains
        std::string_view hex { packet.substr(packet.size() - 2*len, 2*len) };
        //    $display("DBG: rsp_mem_write: str = %s", str);

        // write memory
//        dut_mem_write(adr+i,                 dat  ));
        m_shadow.mem_write(adr, hex2bin(hex));

        // send response
        tx("OK");
    };

    ////////////////////////////////////////
    // RSP multiple register access
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::reg_readall (std::string_view packet) {
        // register value
        std::span<XLEN> val;
        // read DUT/shadow
        if (m_state.dut_register) {
            val = dut_reg_read();
        } else {
            val = m_shadow.reg_read();
        }

        // send response
        std::string_view response { bin2hex(val) };
        tx(response);
    };

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::reg_writeall(std::string_view packet) {
        // register value
        std::span<XLEN> val { static_cast<XLEN>(hex2bin(packet.substr(1, packet.size()-1))) };

        // write DUT/shadow
        dut_reg_writeall(val);
        m_shadow.reg_writeall(val);

        // send response
        tx("OK");
    };

    ////////////////////////////////////////
    // RSP single register access
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::reg_readone (std::string_view packet) {
        // register index
        int unsigned idx;
        int status = std::sscanf(packet.data(), "p%x", &idx);

        // read DUT/shadow
        XLEN val;
        if (m_state.dut_register) {
            val = dut_reg_read(idx);
        } else {
            val = m_shadow.reg_readone(idx);
        }

        // send response
        std::string_view response { bin2hex({ &val, sizeof(XLEN)}) };
        tx(response);
    };

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::reg_writeone(std::string_view packet) {
        // register index
        int unsigned idx;
        int status = std::sscanf(packet.data(), "P%x=", &idx);

        // register value
        XLEN val = static_cast<XLEN>(hex2bin(packet.substr(packet.size()-sizeof(XLEN), sizeof(XLEN))));

        // write DUT/shadow
        dut_reg_writeone(idx, val);
        m_shadow.reg_writeone(idx, val);
        // debug
        switch (sizeof(XLEN)*8) {
            case 32: std::cout << std::format("DEBUG: GPR[{0d}] <= 32'h{08x}", idx, val);
            case 64: std::cout << std::format("DEBUG: GPR[{0d}] <= 64'h{016x}", idx, val);
        }

        // send response
        tx("OK");
    };

    ///////////////////////////////////////
    // RSP forward/reverse step/continue
    ///////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::run_step    (std::string_view packet) {};
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::run_continue(std::string_view packet) {};
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::run_backward(std::string_view packet) {};

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::signal      (std::string_view packet) {};
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query       (std::string_view packet) {};
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::verbose     (std::string_view packet) {};

    ////////////////////////////////////////
    // RSP breakpoints/watchpoints
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::point(std::string_view packet) {
        int status;
        typename SHADOW::ptype_t type;
                 XLEN            addr;
        typename SHADOW::pkind_t kind;

        switch (sizeof(XLEN)*8) {
            case 32: status = std::sscanf(packet.data(), "[zZ]%x,%8x,%x", &command, &type, &addr, &kind);
            case 64: status = std::sscanf(packet.data(), "[zZ]%x,%16x,%x", &command, &type, &addr, &kind);
        }

        // insert/remove
        switch (command) {
            case 'z': shadow.point_remove(type, addr, kind);
            case 'Z': shadow.point_insert(type, addr, kind);
        }

        // send  response
        tx.("OK");
    };

    ////////////////////////////////////////
    // RSP extended/reset/detach/kill
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::extended () {
        // set extended mode
        m_state.extended = true;
        // send response
        tx("OK");
    };

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::reset       () {
        // perform DUT RESET sequence
        //dut_reset_assert();
    };

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::detach () {
        // send response (GDB cliend will close the socket connection)
        tx("OK");

        // re-initialize stub state
        //m_state = STUB_STATE_INIT;

        // stop HDL simulation, so the HDL simulator can render waveforms
        //cout << std::format("GDB: detached, stopping HDL simulation from within state {}.", m_shadow.sig.name);
        // TODO: implement DPI call to $finish();
        //$stop();

        // after user continues HDL simulation blocking wait for GDB client to reconnect
        std::cout << "GDB: continuing stopped HDL simulation, waiting for GDB to reconnect.";
        // blocking wait for client (GDB) to accept connection
        accept();
    };

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::kill () {
        // TODO: implement DPI call to $finish();
        // TODO: throw exception, catch it in main and return from main
        std::exit(0);
    };

    ////////////////////////////////////////
    // RSP packet
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::parse (std::string_view packet) {
        switch (packet[0]) {
        //  case "x": mem_bin_read ();
        //  case "X": mem_bin_write();
            case 'm': mem_read    (packet);
            case 'M': mem_write   (packet);
            case 'g': reg_readall (packet);
            case 'G': reg_writeall(packet);
            case 'p': reg_readone (packet);
            case 'P': reg_writeone(packet);
            case 's':
            case 'S': run_step    (packet);
            case 'c':
            case 'C': run_continue(packet);
            case 'b': run_backward(packet);
            case '?': signal      (packet);
            case 'Q':
            case 'q': query       (packet);
            case 'v': verbose     (packet);
            case 'z':
            case 'Z': point       (packet);
            case '!': extended    ();
            case 'R': reset       ();
            case 'D': detach      ();
            case 'k': kill        ();
            // for unsupported commands respond with empty packet
            default: tx("");
        };
    };

};
