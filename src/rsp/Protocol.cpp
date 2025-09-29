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

    Protocol::Protocol (std::string_view name) {
        m_packet = new Packet(name);
    }

    std::string_view Protocol::rx () {
        return m_packet.rx(m_state.acknowledge);
    }

    void Protocol::tx (std::string_view packet) {
        m_packet.tx(packet, m_state.acknowledge);
    }

    ////////////////////////////////////////
    // conversion between std::byte and HEX
    ////////////////////////////////////////

    std::span<std::byte> hex2bin (std::string_view hex) {
        std::vector<std::byte> bin { hex.size()/2 };
    }

    std::string bin2hex (std::span<std::byte> bin) {
        return std::format("{::02x}", bin);
    }

    ////////////////////////////////////////
    // RSP memory access (hexadecimal)
    ////////////////////////////////////////

    template <typename XLEN>
    void Protocol::mem_read (std::string_view packet) {
        int code;
        int status;
        SIZE_T adr;
        SIZE_T len;
        byte val;

        // memory address and length
        case (XLEN)
            32: code = $sscanf(pkt, "m%8h,%8h", adr, len);
            64: code = $sscanf(pkt, "m%16h,%16h", adr, len);
        endcase

    //    $display("DBG: rsp_mem_read: adr = %08x, len=%08x", adr, len);

        // read memory
        pkt = {len{"XX"}};
        for (SIZE_T i=0; i<len; i++) begin
            string tmp = "XX";
            if (stub_state.memory) begin
                val =     dut_mem_read(adr+i);
            end else begin
                val = shd.mem_read(adr+i, 1)[0];
            end
            tmp = $sformatf("%02h", val);
            pkt[i*2+0] = tmp[0];
            pkt[i*2+1] = tmp[1];
        end

    //    $display("DBG: rsp_mem_read: pkt = %s", pkt);

        // send response
        tx(response);
    };

    template <typename XLEN>
    void Protocol::mem_write   (std::string_view packet) {
        int code;
        string str;
        int status;
        SIZE_T adr;
        SIZE_T len;
        byte   dat;

        // memory address and length
        case (XLEN)
            32: code = $sscanf(pkt, "M%8h,%8h:", adr, len);
            64: code = $sscanf(pkt, "M%16h,%16h:", adr, len);
        endcase
        //    $display("DBG: rsp_mem_write: adr = 'h%08h, len = 'd%0d", adr, len);

            // remove the header from the packet, only data remains
            str = pkt.substr(pkt.len() - 2*len, pkt.len() - 1);
        //    $display("DBG: rsp_mem_write: str = %s", str);

        // write memory
        for (SIZE_T i=0; i<len; i++) begin
        //    $display("DBG: rsp_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, dut_mem_read(adr+i));
            status = $sscanf(str.substr(i*2, i*2+1), "%h", dat);
        //    $display("DBG: rsp_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, dut_mem_read(adr+i));
            // TODO handle memory access errors
            // NOTE: memory writes are always done to both DUT and shadow
            void'(    dut_mem_write(adr+i,                 dat  ));
            void'(shd.mem_write(adr+i, byte_array_t'('{dat})));
        end

        // send response
        tx("OK");
    };

    ////////////////////////////////////////
    // RSP multiple register access
    ////////////////////////////////////////

    template <typename XLEN>
    void Protocol::reg_readall (std::string_view packet) {
        int status;
        logic [XLEN-1:0] val;  // 4-state so GDB can iterpret 'x

        pkt = "";
        for (int unsigned i=0; i<REGN; i++) begin
            // swap byte order since they are sent LSB first
            if (stub_state.register) begin
                val = {<<8{dut_reg_read(i)}};
            end else begin
                val = {<<8{shd.reg_read(i)}};
            end
            case (XLEN)
                32: response = {pkt, $sformatf("%08h", val)};
                64: response = {pkt, $sformatf("%016h", val)};
            endcase
        end

        // send response
        tx(response);
    };

    template <typename XLEN>
    void Protocol::reg_writeall(std::string_view packet) {
        string pkt;
        int unsigned len = XLEN/8*2;
        bit [XLEN-1:0] val;

        // remove command
        pkt = pkt.substr(1, pkt.len()-1);

        // GPR
        for (int unsigned i=0; i<REGN; i++) {
            case (XLEN)
                32: status = $sscanf(pkt.substr(i*len, i*len+len-1), "%8h", val);
                64: status = $sscanf(pkt.substr(i*len, i*len+len-1), "%16h", val);
            endcase
            // swap byte order since they are sent LSB first
            // NOTE: register writes are always done to both DUT and shadow
            dut_reg_write(i, {<<8{val}});
            shd.reg_write(i, {<<8{val}});
        }

        // send response
        tx("OK");
    };

    ////////////////////////////////////////
    // RSP single register access
    ////////////////////////////////////////

    template <typename XLEN>
    void Protocol::reg_readone (std::string_view packet) {
        int status;
        int unsigned idx;
        logic [XLEN-1:0] val;  // 4-state so GDB can iterpret 'x

        // register index
        status = $sscanf(pkt, "p%h", idx);

        // swap byte order since they are sent LSB first
        if (m_state.register) {
//          val = {<<8{dut_reg_read(idx)}};
        } else {
            val = shd.reg_read(idx);
        }
        switch (XLEN) {
            case 32: response = {pkt, $sformatf("%08h", val)};
            case 64: response = {pkt, $sformatf("%016h", val)};
        }

        // send response
        tx(response);
    };

    template <typename XLEN>
    void Protocol::reg_writeone(std::string_view packet) {
        int status;
        int unsigned idx;
        bit [XLEN-1:0] val;

        // register index and value
        case (XLEN)
            32: status = $sscanf(pkt, "P%h=%8h", idx, val);
            64: status = $sscanf(pkt, "P%h=%16h", idx, val);
        endcase

        // swap byte order since they are sent LSB first
        // NOTE: register writes are always done to both DUT and shadow
        dut_reg_write(idx, {<<8{val}});
        shd.reg_write(idx, {<<8{val}});
    //    case (XLEN)
    //        32: $display("DEBUG: GPR[%0d] <= 32'h%08h", idx, val);
    //        64: $display("DEBUG: GPR[%0d] <= 64'h%016h", idx, val);
    //    endcase

        // send response
        tx("OK");
    };

    ///////////////////////////////////////
    // RSP forward/reverse step/continue
    ///////////////////////////////////////

    void Protocol::run_step    (std::string_view packet) {};
    void Protocol::run_continue(std::string_view packet) {};
    void Protocol::run_backward(std::string_view packet) {};

    void Protocol::signal      (std::string_view packet) {};
    void Protocol::query       (std::string_view packet) {};
    void Protocol::verbose     (std::string_view packet) {};

    ////////////////////////////////////////
    // RSP breakpoints/watchpoints
    ////////////////////////////////////////

    template <typename XLEN>
    void Protocol::point_remove(std::string_view packet) {
        int status;
        shadow::ptype_t type;
                XLEN    addr;
        shadow::pkind_t kind;

        // breakpoint/watchpoint
        switch (XLEN) {
            case 32: status = sscanf(packet, "z%h,%8h,%h", &type, &addr, &kind);
            case 64: status = sscanf(packet, "z%h,%16h,%h", &type, &addr, &kind);
        }

        shadow.point_remove(type, addr, kind);
        tx.("OK");
    };

    template <typename XLEN>
    void Protocol::point_insert(std::string_view packet) {
        int status;
        shadow::ptype_t type;
                XLEN    addr;
        shadow::pkind_t kind;

        // breakpoint/watchpoint
        switch (XLEN) {
            case 32: status = sscanf(packet, "Z%h,%8h,%h", &type, &addr, &kind);
            case 64: status = sscanf(packet, "Z%h,%16h,%h", &type, &addr, &kind);
        }

        shadow.point_insert(type, addr, kind);
        tx.("OK");
    };

    ////////////////////////////////////////
    // RSP extended/reset/detach/kill
    ////////////////////////////////////////

    void Protocol::extended () {
        // set extended mode
        m_state.extended = true;
        // send response
        tx("OK");
    };

    void Protocol::reset       () {
        // perform DUT RESET sequence
        //dut_reset_assert();
    };

    void Protocol::detach () {
        // send response (GDB cliend will close the socket connection)
        tx("OK");

        // re-initialize stub state
        //stub_state = STUB_STATE_INIT;

        // stop HDL simulation, so the HDL simulator can render waveforms
        //cout << std::format("GDB: detached, stopping HDL simulation from within state {}.", shadow.sig.name);
        // TODO: implement DPI call to $finish();
        //$stop();

        // after user continues HDL simulation blocking wait for GDB client to reconnect
        std::cout << "GDB: continuing stopped HDL simulation, waiting for GDB to reconnect.";
        // blocking wait for client (GDB) to accept connection
        accept();
    };

    void Protocol::kill () {
        // TODO: implement DPI call to $finish();
        // TODO: throw exception, catch it in main and return from main
        std::exit(0);
    };

    ////////////////////////////////////////
    // RSP packet
    ////////////////////////////////////////

    void Protocol::parse (std::string_view packet) {
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
            case 'z': point_remove(packet);
            case 'Z': point_insert(packet);
            case '!': extended    ();
            case 'R': reset       ();
            case 'D': detach      ();
            case 'k': kill        ();
            // for unsupported commands respond with empty packet
            default: tx("");
        };
    };

};
