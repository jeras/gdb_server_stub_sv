///////////////////////////////////////////////////////////////////////////////
// RSP (remote serial protocol) protocol
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ includes
#include <string>

// HDLDB includes
#include "Packet.hpp"

namespace rsp {

    template <typename XLEN>
    class Protocol {

        struct m_state {
            bool acknowledge;
            bool extended;
        };

        Packet m_packet;

        // constructor/destructor
        Protocol (std::string_view);
        ~Protocol ();

        std::string_view rx ();
        void tx (std::string_view);

        std::span<std::byte> hex2bin (std::string_view) const;
        std::string bin2hex (std::span<std::byte>) const;

        // packet parsers
        void mem_read    (std::string_view);
        void mem_write   (std::string_view);
        void reg_readall (std::string_view);
        void reg_writeall(std::string_view);
        void reg_readone (std::string_view);
        void reg_writeone(std::string_view);
        void point_remove(std::string_view);
        void point_insert(std::string_view);

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
    };
};
