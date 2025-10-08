///////////////////////////////////////////////////////////////////////////////
// HDLDB main (stand alone executable)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ includes
#include <print>

// HDLDB includes
#include <hdldb.hpp>

int main() {
    std::println("Hello, World!");

    SystemHdlDb shadow { };

    ProtocolHdlDb protocol { "1234", shadow };

    protocol.loop();

    return 0;
}
