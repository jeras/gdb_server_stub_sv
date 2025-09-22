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

// HDLDB includes
#include "Packet.h"

namespace Rsp {

    std::string Packet::get () const {
            output string pkt,
        int status;
        int unsigned len;
        byte   buffer [] = new[512];
        byte   cmd [];
        string str = "";
        byte   checksum = 0;
        string checksum_ref;
        string checksum_str;
        // wait for the start character, ignore the rest

        // TODO: error handling?
        do {
            status = socket_recv(buffer, 0);
  //          $display("DEBUG: rsp_get_packet: buffer = %p", buffer);
            str = {str, string(buffer)};
            len = str.len();
  //          $display("DEBUG: rsp_get_packet: str = %s", str);
        } while (str[len-3] != "#");

        // extract packet data from received string
        pkt = str.substr(1,len-4);
        if (stub_state.remote_log) {
            log << "REMOTE: <- " << pkt << std::eol;
        }

        // calculate packet data checksum
        cmd = new[len-4](byte_array_t(pkt));
        checksum = cmd.sum();

        // Get checksum now
        checksum_ref = str.substr(len-2,len-1);

        // Verify checksum
        checksum_str = $sformatf("%02h", checksum);
        if (checksum_ref != checksum_str) {
            $error("Bad checksum. Got 0x%s but was expecting: 0x%s for packet '%s'", checksum_ref, checksum_str, pkt);
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
        int status;
        byte   ch [] = new[1];
        byte   checksum = 0;

        if (stub_state.remote_log) {
            $display("REMOTE: -> %p", pkt);
        };

        // Send packet start
        rsp_write("$");

        // Send packet data and calculate checksum
        foreach (pkt[i]) {
            checksum += pkt[i];
            rsp_write(string(pkt[i]));
        };

        // Send packet end
        rsp_write("#");

        // Send the checksum
        rsp_write($sformatf("%02h", checksum));

        // Check acknowledge
        if (ack) {
            status = socket_recv(ch, 0);
            if (ch[0] == "+")  return(0);
            else               return(-1);
        };
    };

};
