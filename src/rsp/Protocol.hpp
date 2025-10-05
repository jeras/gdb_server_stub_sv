///////////////////////////////////////////////////////////////////////////////
// RSP (remote serial protocol) protocol
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ includes
#include <string>
#include <map>

// HDLDB includes
#include "Packet.hpp"

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
        std::map<std::string, char> m_features_server {
            {"swbreak"        , "+"},
            {"hwbreak"        , "+"},
            {"error-message"  , "+"},  // GDB (LLDB asks with QEnableErrorStrings)
            {"binary-upload"  , "-"},  // TODO: for now it is broken
            {"multiprocess"   , "-"},
            {"ReverseStep"    , "+"},
            {"ReverseContinue", "+"},
            {"QStartNoAckMode", "+"}
        };
        std::map<std::string, char> m_features_client { };

        SHADOW m_shadow;

        // constructor/destructor
        Protocol (std::string_view, SHADOW);
        ~Protocol ();

        std::string_view rx ();
        void tx (std::string_view);

        std::vector<std::byte> hex2bin (std::string_view) const;
        std::string bin2hex (std::span<std::byte>) const;

        // packet parsers
        void mem_read    (std::string_view);
        void mem_write   (std::string_view);
        void reg_readall (std::string_view);
        void reg_writeall(std::string_view);
        void reg_readone (std::string_view);
        void reg_writeone(std::string_view);
        void point       (std::string_view);

        void run_step    (std::string_view);
        void run_continue(std::string_view);
        void run_backward(std::string_view);
        void signal      (std::string_view);
        void reset       ();

        void query       (std::string_view);
        void verbose     (std::string_view);
        void extended    ();
        void detach      ();
        void kill        ();

        // packet parser
        void parse       (std::string_view);

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
};
