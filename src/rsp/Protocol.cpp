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

    ///////////////////////////////////////
    // RSP signal
    ///////////////////////////////////////

//    // in response to '?'
//    function automatic int rsp_signal();
//        string pkt;
//        int status;
//
//        // read packet
//        status = rsp_get_packet(pkt);
//
//        // reply with current signal
//        status = rsp_stop_reply();
//        return(status);
//    endfunction: rsp_signal
//
//    // TODO: Send a exception packet "T <value>"
//    function automatic int rsp_stop_reply (
//        // register
//        input int unsigned idx = -1,
//        input [XLEN-1:0]   val = 'x,
//        // thread
//        input int thr = -1,
//        // core
//        input int unsigned core = -1
//    );
//        string str;
//        // reply with signal/register/thread/core/reason
//        str = $sformatf("T%02h;", shd.sig);
//        // register
//        if (idx != -1) {
//            case (XLEN)
//                32: str = {str, $sformatf("%0h:%08h;", idx, val)};
//                64: str = {str, $sformatf("%0h:%016h;", idx, val)};
//            endcase
//        end
//        // thread
//        if (thr != -1) {
//            str = {str, $sformatf("thread:%s;", sformat_thread(1, thr))};
//        end
//        // core
//        if (core != -1) {
//            str = {str, $sformatf("core:%h;", core)};
//        end
//        // reason
//        case (shd.rsn.ptype)
//            watch, rwatch, awatch: {
//                str = {str, $sformatf("%s:%h;", shd.rsn.ptype.name, shd.ret.lsu.adr)};
//            end
//            swbreak, hwbreak: {
//                str = {str, $sformatf("%s:;", shd.rsn.ptype.name)};
//            end
//            replaylog: {
//                str = {str, $sformatf("%s:%s;", shd.rsn.ptype.name, shd.cnt == 0 ? "begin" : "end")};
//            end
//        endcase
//        // remove the trailing semicolon
//        str = str.substr(0, str.len()-2);
//        return(rsp_send_packet(str));
//    //    // reply with signal (current signal by default)
//    //    return(rsp_send_packet($sformatf("S%02h", sig)));
//    endfunction: rsp_stop_reply
//
//    // send ERROR number reply (GDB only)
//    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
//    function automatic int rsp_error_number_reply (
//        input byte val = 0
//    );
//        return(rsp_send_packet($sformatf("E%02h", val)));
//    endfunction: rsp_error_number_reply
//
//    // send ERROR text reply (GDB only)
//    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
//    function automatic int rsp_error_text_reply (
//        input string str = ""
//    );
//        return(rsp_send_packet($sformatf("E.%s", rsp_ascii2hex(str))));
//    endfunction: rsp_error_text_reply
//
//    // send ERROR LLDB reply
//    // https://lldb.llvm.org/resources/lldbgdbremote.html#qenableerrorstrings
//    function automatic int rsp_error_lldb_reply (
//        input byte   val = 0,
//        input string str = ""
//    );
//        return(rsp_send_packet($sformatf("E%02h;%s", val, rsp_ascii2hex(str))));
//    endfunction: rsp_error_lldb_reply
//
//    // send message to GDB console output
//    function automatic int rsp_console_output (
//        input string str
//    );
//        return(rsp_send_packet($sformatf("O%s", rsp_ascii2hex(str))));
//    endfunction: rsp_console_output

    ///////////////////////////////////////
    // RSP query (monitor, )
    ///////////////////////////////////////

    // send message to GDB console output
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::monitor_reply (std::string_view str) {}
        tx(bin2hex(str))));
    }

    // GDB monitor commands
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query_monitor (std::string_view str) {
        switch (str)
            case "help":
                monitor_reply("HELP: Available monitor commands:\n"
                              "* 'set remote log on/off',\n"
                              "* 'set waveform dump on/off',\n"
                              "* 'set register=dut/shadow' (reading registers from dut/shadow, default is shadow),\n"
                              "* 'set memory=dut/shadow' (reading memories from dut/shadow, default is shadow),\n"
                              "* 'reset assert' (assert reset for a few clock periods),\n"
                              "* 'reset release' (synchronously release reset).");
            case "set remote log on":
                m_state.remote_log = 1'b1;
                monitor_reply("Enabled remote logging to STDOUT.\n");
            case "set remote log off":
                m_state.remote_log = 1'b0;
                monitor_reply("Disabled remote logging.\n");
            case "set waveform dump on":
//                $dumpon;
                monitor_reply("Enabled waveform dumping.\n");
            case "set waveform dump off":
//                $dumpoff;
                monitor_reply("Disabled waveform dumping.\n");
            case "set register=dut":
                m_state.dut_register = true;
                monitor_reply("Reading registers directly from DUT.\n");
            case "set register=shadow":
                m_state.dut_register = false;
                monitor_reply("Reading registers from shadow copy.\n");
            case "set memory=dut":
                m_state.dut_memory = true;
                monitor_reply("Reading memory directly from DUT.\n");
            case "set memory=shadow":
                m_state.dut_memory = false;
                monitor_reply("Reading memory from shadow copy.\n");
            case "reset assert":
//                dut_reset_assert;
                // TODO: rethink whether to reset the shadow or keep it
                //shd = new();
                monitor_reply("DUT reset asserted.\n");
            case "reset release":
//                dut_reset_release;
                monitor_reply("DUT reset released.\n");
            default:
                monitor_reply("'monitor' command was not recognized.\n");
        }
    }

        // GDB supported features
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query_supported (std::string_view str) {
        int code;
        int status;
        string_queue_t features;
        string feature;
        string value;

        // parse features supported by the GDB client
        features = split(str, byte'(";"));
        foreach (features[i]) {
            int unsigned len = features[i].len();
            // add feature and its value (+/-/?)
            value = string'(features[i][len-1]);
            if (value inside {"+", "-", "?"}) {
                feature = features[i].substr(0, len-2);
            } else {
                features[i] = char2space(features[i], byte'("="));
                code = $sscanf(str, "%s %s", feature, value);
            }
            features_gdb[feature] = value;
        }
        $display("DEBUG: features_gdb = %p", features_gdb);

        // reply with stub features
        str = "";
        foreach (features_stub[feature]) {
            if (features_stub[feature] inside {"+", "-", "?"}) {
                str = {str, $sformatf("%s%s;", feature, features_stub[feature])};
            } else {
                str = {str, $sformatf("%s=%s;", feature, features_stub[feature])};
            }
        }
        // remove the trailing semicolon
        str = str.substr(0, str.len()-2);
        status = rsp_send_packet(str);

        return(0);
    }


    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::format_thread (
        input int process,
        input int thread
    ) {
        switch (features_stub["multiprocess"]) {
            case "+": return($sformatf("p%h,%h", process, thread));
            case "-": return($sformatf("%0h", thread));
        }
    }

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::parse_thread (
        input string str
    ) {
        int code;
        int process;
        int thread;
        switch (features_stub["multiprocess"]) {
            case "+": code = $sscanf(str, "p%h,%h;", process, thread);
            case "-": code = $sscanf(str, "%h", thread);
        }
        return(thread);
    }

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query (std::string_view packet);
        string str;
        int status;

        // parse various query packets
        if (std::sscanf(packet, "qSupported:%s", str) > 0) {
            $display("DEBUG: qSupported = %p", str);
            status = rsp_query_supported(str);
        } else
        // parse various monitor packets
        if (std::sscanf(packet, "qRcmd,%s", str) > 0) {
            rsp_query_monitor(rsp_hex2ascii(str));
        } else
        // start no acknowledge mode
        if (packet == "QStartNoAckMode") {
            status = rsp_send_packet("OK");
            m_state.acknowledge = false;
        } else
        // start no acknowledge mode
        if (packet == "QEnableErrorStrings") {
            status = rsp_send_packet("OK");
            // TODO
//            m_state.acknowledge = false;
        } else
        // query first thread info
        if (packet == "qfThreadInfo") {
            str = "m";
            foreach (THREADS[thr]) {
                str = {str, sformat_thread(1, thr+1), ","};
            end
            // remove the trailing comma
            str = str.substr(0, str.len()-2);
            status = rsp_send_packet(str);
        } else
        // query subsequent thread info
        if (packet == "qsThreadInfo") {
            // last thread
            status = rsp_send_packet("l");
        } else
        // query extra info for given thread
        if (std::sscanf(packet, "qThreadExtraInfo,%s", str) > 0) {
            int thr;
            thr = sscan_thread(str);
            $display("DEBUG: qThreadExtraInfo: str = %p, thread = %0d, THREADS[%0d-1] = %s", str, thr, thr, THREADS[thr-1]);
            status = rsp_send_packet(rsp_ascii2hex(THREADS[thr-1]));
        } else
        // query first thread info
        if (packet == "qC") {
            int thr = 1;
            str = {"QC", sformat_thread(1, thr)};
            status = rsp_send_packet(str);
        } else
        // query whether the remote server attached to an existing process or created a new process
        if (packet == "qAttached") {
            // respond as "attached"
            status = rsp_send_packet("1");
        } else
        // not supported, send empty response packet
        {
            status = rsp_send_packet("");
        }
    }

    ////////////////////////////////////////
    // RSP verbose
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::verbose (std::string_view packet) {
        str::string str;
        string tmp;
        int code;

        // interrupt signal
        if (packet == "vCtrlC") {
//            shd.sig = SIGINT;
//            tx("OK");
            tx("");
        } else
        // list actions supported by the ‘vCont?’ packet
        if (packet == "vCont?") {
            tx("vCont;c:C;s:S");
        }
        // parse 'vCont' packet
        if (std::sscanf(packet.data(), "vCont;%s", str) > 0) {
            string_queue_t features;
            string action;
            string thread;
            // parse action list
            features = split(str, byte'(";"));
            foreach (features[i]) {
                // parse action/thread pair
                features[i] = char2space(features[i], byte'(":"));
                code = $sscanf(features[i], "%s %s", action, thread);
                // TODO
            }
        } else
        // not supported, send empty response packet
        {
            tx("");
        }
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
