///////////////////////////////////////////////////////////////////////////////
// RSP (remote serial protocol) protocol
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#pragma once

// C++ includes
#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <print>
#include <sstream>
#include <spanstream> // TODO: learn to use this

// HDLDB includes
#include "Packet.hpp"
#include <Points.hpp>

namespace rsp {

    template <typename XLEN, typename SHADOW>
    class Protocol : Packet {

        // server state
        struct State {
            bool acknowledge;
            bool extended;
            bool dut_register;
            bool dut_memory;
            bool remote_log;
        };

        State m_state;

        // supported features
        std::map<std::string, std::string> m_features_server {
            {"swbreak"        , "+"},
            {"hwbreak"        , "+"},
            {"error-message"  , "+"},  // GDB (LLDB asks with QEnableErrorStrings)
            {"binary-upload"  , "-"},  // TODO: for now it is broken
            {"multiprocess"   , "-"},
            {"ReverseStep"    , "+"},
            {"ReverseContinue", "+"},
            {"QStartNoAckMode", "+"}
        };
        std::map<std::string, std::string> m_features_client { };

        SHADOW m_shadow;

    public:
        // constructor/destructor
        Protocol (std::string_view name, SHADOW shadow);
        Protocol (std::uint16_t port, SHADOW shadow);
        ~Protocol ();

        std::string_view rx ();
        void tx (std::string_view);

        std::vector<std::byte> hex2bin (std::string_view hex) const;
        std::string bin2hex (std::span<std::byte> bin) const;
        std::string bin2hex (std::string_view str) const;
        constexpr size_t lit2hash (std::string_view str) const;

        // packet parsers
        void mem_read    (std::string_view packet);
        void mem_write   (std::string_view packet);
        void reg_readall (std::string_view packet);
        void reg_writeall(std::string_view packet);
        void reg_readone (std::string_view packet);
        void reg_writeone(std::string_view packet);
        void point       (std::string_view packet);

        void run_step    (std::string_view packet);
        void run_continue(std::string_view packet);
        void run_backward(std::string_view packet);
        void signal      (std::string_view packet);
        void reset       ();

        void query       (std::string_view packet);
        void verbose     (std::string_view packet);
        void extended    ();
        void detach      ();
        void kill        ();

        // packet parser
        void parse       (std::string_view packet);

        // main loop
        void loop ();

        void rsp_signal          ();
        void stop_reply          (int index = -1, XLEN value = 0,  // register
                                  int thread = -1, int core = -1);  // thread/core
        void error_number_reply  (std::uint8_t value);
        void error_text_reply    (std::string_view text = "");
        void error_lldb_reply    (std::uint8_t value = 0, std::string_view text = "");
        void console_output      (std::string_view text);

        // helpers
        void query_supported     (std::string_view);
        void query_monitor       (std::string_view);
        void query_monitor_reply (std::string_view);

        std::string format_thread (int, int);
        int parse_thread  (std::string_view);

        // dummy placeholders
        // DUT
        void                 dut_reset_assert() {};
        std::span<std::byte> dut_mem_read (XLEN) { return {}; };
        void                 dut_mem_write (XLEN, std::span<std::byte>) {};
        XLEN                 dut_reg_read (int unsigned) { return 0; };
        void                 dut_reg_write (int unsigned, XLEN) {};
    };

    template <typename XLEN, typename SHADOW>
    Protocol<XLEN, SHADOW>::Protocol (std::string_view name, SHADOW shadow) :
        Packet(name),
        m_shadow(shadow)
    { }

    template <typename XLEN, typename SHADOW>
    Protocol<XLEN, SHADOW>::Protocol (std::uint16_t port, SHADOW shadow) :
        Packet(port),
        m_shadow(shadow)
    { }

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
            std::byte byte = static_cast<std::byte>(std::stoi(str.data(), nullptr, 16));
            bin.push_back(byte);
        }
        return bin;
    }

    template <typename XLEN, typename SHADOW>
    std::string Protocol<XLEN, SHADOW>::bin2hex (std::span<std::byte> bin) const {
        std::ostringstream hex;
        for (auto& element: bin) {
            hex << std::format("{:02x}", static_cast<uint8_t>(element));
        }
        return hex.str();
    }

    template <typename XLEN, typename SHADOW>
    std::string Protocol<XLEN, SHADOW>::bin2hex (std::string_view bin) const {
        std::ostringstream hex;
        for (auto& element: bin) {
            hex << std::format("{:02x}", element);
        }
        return hex.str();
    }

    template <typename XLEN, typename SHADOW>
    constexpr size_t Protocol<XLEN, SHADOW>::lit2hash (std::string_view str) const {
        return std::hash<std::string_view>{}(str);
    }

    ///////////////////////////////////////
    // RSP signal
    ///////////////////////////////////////

    // in response to '?'
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::rsp_signal() {
        // reply with current signal
        stop_reply();
    }

    // TODO: Send a exception packet "T <value>"
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::stop_reply (int index, XLEN value,  // register
                                             int thread, int core)  // thread/core
    {
        // reply with signal/register/thread/core/reason
        std::string str { std::format("T{:02x}", m_shadow.signal) };
        // register
        if (index != -1) {
            switch (sizeof(XLEN)*8) {
                case 32: str += std::format(";{:0x}:P{:08x}", index, value);
                case 64: str += std::format(";{:0x}:P{:016x}", index, value);
            }
        }
        // thread
        if (thread != -1) {
            str += std::format(";thread:{}", format_thread(1, thread));
        }
        // core
        if (core != -1) {
            str += std::format(";core:{:x}", core);
        }
        // reason
        switch (m_shadow.reason.ptype) {
            case shadow::PointType::watch:
                str += std::format(";{}:{:x}", "watch", m_shadow.ret.lsu.adr);
                break;
            case shadow::PointType::rwatch:
                str += std::format(";{}:{:x}", "rwatch", m_shadow.ret.lsu.adr);
                break;
            case shadow::PointType::awatch:
                str += std::format(";{}:{:x}", "awatch", m_shadow.ret.lsu.adr);
                break;
            case shadow::PointType::swbreak:
                str += std::format(";{}:", "swbreak");
                break;
            case shadow::PointType::hwbreak:
                str += std::format(";{}:", "hwbreak");
                break;
            case shadow::PointType::replaylog:
                str += std::format(";{}:{}", "replaylog", m_shadow.cnt == 0 ? "begin" : "end");
                break;
        }
        tx(str);
    //    // reply with signal (current signal by default)
    //    tx(std::format("S{:02x}", m_shadow.signal));
    }

    // send ERROR number reply (GDB only)
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::error_number_reply (std::uint8_t value) {
        tx(std::format("E{:02x}", value));
    }

    // send ERROR text reply (GDB only)
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::error_text_reply (std::string_view text) {
        tx(std::format("E.{}", bin2hex(text)));
    }

    // send ERROR LLDB reply
    // https://lldb.llvm.org/resources/lldbgdbremote.html#qenableerrorstrings
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::error_lldb_reply (std::uint8_t value, std::string_view text) {
        tx(std::format("E{:02x};{}", value, bin2hex(text)));
    }

    // send message to GDB console output
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::console_output (std::string_view text) {
        tx(std::format("O{}", bin2hex(text)));
    }

    ///////////////////////////////////////
    // RSP query (monitor, )
    ///////////////////////////////////////

    // send message to GDB console output
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query_monitor_reply (std::string_view str) {
        tx(bin2hex(str));
    }

    // GDB monitor commands
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query_monitor (std::string_view str) {
        switch (std::hash<std::string_view>{}(str)) {
            case lit2hash("help"):
                query_monitor_reply("HELP: Available monitor commands:\n"
                    "* 'set remote log on/off',\n"
                    "* 'set waveform dump on/off',\n"
                    "* 'set register=dut/shadow' (reading registers from dut/shadow, default is shadow),\n"
                    "* 'set memory=dut/shadow' (reading memories from dut/shadow, default is shadow),\n"
                    "* 'reset assert' (assert reset for a few clock periods),\n"
                    "* 'reset release' (synchronously release reset).");
                break;
            case lit2hash("set remote log on"):
                m_state.remote_log = true;
                query_monitor_reply("Enabled remote logging to STDOUT.\n");
                break;
            case lit2hash("set remote log off"):
                m_state.remote_log = false;
                query_monitor_reply("Disabled remote logging.\n");
                break;
            case lit2hash("set waveform dump on"):
//                $dumpon;
                query_monitor_reply("Enabled waveform dumping.\n");
                break;
            case lit2hash("set waveform dump off"):
//                $dumpoff;
                query_monitor_reply("Disabled waveform dumping.\n");
                break;
            case lit2hash("set register=dut"):
                m_state.dut_register = true;
                query_monitor_reply("Reading registers directly from DUT.\n");
                break;
            case lit2hash("set register=shadow"):
                m_state.dut_register = false;
                query_monitor_reply("Reading registers from shadow copy.\n");
                break;
            case lit2hash("set memory=dut"):
                m_state.dut_memory = true;
                query_monitor_reply("Reading memory directly from DUT.\n");
                break;
            case lit2hash("set memory=shadow"):
                m_state.dut_memory = false;
                query_monitor_reply("Reading memory from shadow copy.\n");
                break;
            case lit2hash("reset assert"):
//                dut_reset_assert;
                // TODO: rethink whether to reset the shadow or keep it
                //shd = new();
                query_monitor_reply("DUT reset asserted.\n");
                break;
            case lit2hash("reset release"):
//                dut_reset_release;
                query_monitor_reply("DUT reset released.\n");
                break;
            default:
                query_monitor_reply("'monitor' command was not recognized.\n");
        }
    }

    // GDB supported features
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query_supported (std::string_view str) {
//        int code;
//
//        // parse features supported by the GDB client
//        for (const auto feature : std::views::split(str, ";"sv)) {
//            int unsigned len = features[i].len();
//            // add feature and its value (+/-/?)
//            char value = feature.back();
//            if (value inside {"+", "-", "?"}) {
//                features_gdb[features[i].substr(0, len-2)] = value;
//            } else {
//                features[i] = char2space(features[i], '=');
//                int code = std::sscanf(str, "%s %s", &feature, &value);
//                features_gdb[features[i].substr(0, len-2)] = value;
//            }
//        }
//        std::println("DEBUG: features_gdb = {}", features_gdb);
//
//        // reply with stub features
//        std::vector<std::string> response { };
//        foreach (m_features_server[feature]) {
//            if (m_features_server[feature] inside {"+", "-", "?"}) {
//                response = std::format("{ }{ };", feature, m_features_server[feature]);
//            } else {
//                response = std::format("{ }={ };", feature, m_features_server[feature])};
//            }
//        }
//        // remove the trailing semicolon
//        tx(response | std::views::join_with(';'));
    }


    template <typename XLEN, typename SHADOW>
    std::string Protocol<XLEN, SHADOW>::format_thread (int process, int thread) {
        switch (m_features_server["multiprocess"][0]) {
            case '+': return std::format("p{:x},{:x}", process, thread);
            case '-': return std::format("{:0x}", thread);
        }
    }

    template <typename XLEN, typename SHADOW>
    int Protocol<XLEN, SHADOW>::parse_thread (std::string_view str) {
        int code;
        int process;
        int thread;
        switch (m_features_server["multiprocess"][0]) {
            case '+': code = std::sscanf(str.data(), "p%x,%x;", &process, &thread);
            case '-': code = std::sscanf(str.data(), "%x", &thread);
        }
        return(thread);
    }

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::query (std::string_view packet) {
        std::string str;

        // parse various query packets
        if (std::sscanf(packet.data(), "qSupported:%s", str.data()) > 0) {
            std::println("DEBUG: qSupported = {}", str.data());
            query_supported(str);
        } else
        // parse various monitor packets
        if (std::sscanf(packet.data(), "qRcmd,%s", str.data()) > 0) {
            query_monitor(hex2bin(str));
        } else
        // start no acknowledge mode
        if (packet == "QStartNoAckMode") {
            tx("OK");
            m_state.acknowledge = false;
        } else
        // start no acknowledge mode
        if (packet == "QEnableErrorStrings") {
            tx("OK");
            // TODO
//            m_state.acknowledge = false;
        } else
        // query first thread info
        if (packet == "qfThreadInfo") {
            str = "m";
//            foreach (THREADS[thr]) {
//                str = {str, sformat_thread(1, thr+1), ","};
//            end
            // remove the trailing comma
//            str = str.substr(0, str.size()-2);
            tx(str);
        } else
        // query subsequent thread info
        if (packet == "qsThreadInfo") {
            // last thread
            tx("l");
        } else
        // query extra info for given thread
        if (std::sscanf(packet.data(), "qThreadExtraInfo,%s", str) > 0) {
//            int thread = sscan_thread(str);
//            std::println("DEBUG: qThreadExtraInfo: str = {}, thread = {:0d}, THREADS[{:0d}-1] = {}", str, thread, thread, THREADS[thread-1]);
//            tx(bin2hex(THREADS[thr-1]));
        } else
        // query first thread info
        if (packet == "qC") {
            int thr = 1;
            tx("QC" + format_thread(1, thr));
        } else
        // query whether the remote server attached to an existing process or created a new process
        if (packet == "qAttached") {
            // respond as "attached"
            tx("1");
        } else
        // not supported, send empty response packet
        {
            tx("");
        }
    }

    ////////////////////////////////////////
    // RSP verbose
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::verbose (std::string_view packet) {
        std::string str;

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
        if (std::sscanf(packet.data(), "vCont;%s", str.data()) > 0) {
//            string_queue_t features;
//            string action;
//            string thread;
//            // parse action list
//            features = split(str, byte'(";"));
//            foreach (features[i]) {
//                // parse action/thread pair
//                features[i] = char2space(features[i], byte'(":"));
//                int code = $sscanf(features[i], "%s %s", action, thread);
//                // TODO
//            }
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

    //    std::println("DBG: rsp_mem_read: adr = %08x, len=%08x", adr, len);

        // read from memory
        std::span<std::byte> data;
        if (m_state.dut_memory) {
            data = dut_mem_read(addr, size);
        } else {
            data = m_shadow.mem_read(addr, size);
        }
    //    std::println("DBG: rsp_mem_read: pkt = %s", pkt);

        // send response
        tx(bin2hex(data));
    }

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
        //    std::println("DBG: rsp_mem_write: adr = 'h%08h, len = 'd%0d", adr, len);

        // remove the header from the packet, only data remains
        std::string_view hex { packet.substr(packet.size() - 2*len, 2*len) };
        //    std::println("DBG: rsp_mem_write: str = %s", str);

        // write memory
//        dut_mem_write(adr+i,                 dat  ));
        m_shadow.mem_write(adr, hex2bin(hex));

        // send response
        tx("OK");
    }

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

    ////////////////////////////////////////
    // RSP breakpoints/watchpoints
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::point (std::string_view packet) {
        int status;
        char command;
        typename SHADOW::ptype_t type;
                 XLEN            addr;
        typename SHADOW::pkind_t kind;

        switch (sizeof(XLEN)*8) {
            case 32: status = std::sscanf(packet.data(), "[zZ]%x,%8x,%x", &command, &type, &addr, &kind);
            case 64: status = std::sscanf(packet.data(), "[zZ]%x,%16x,%x", &command, &type, &addr, &kind);
        }

        // insert/remove
        switch (command) {
            case 'z': m_shadow.point_remove(type, addr, kind);
            case 'Z': m_shadow.point_insert(type, addr, kind);
        }

        // send  response
        tx("OK");
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

    ////////////////////////////////////////
    // main loop
    ////////////////////////////////////////

    template <typename XLEN, typename SHADOW>
    void Protocol<XLEN, SHADOW>::loop () {
        std::println("Hello from HDLDB!");
    }

};
