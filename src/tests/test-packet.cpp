///////////////////////////////////////////////////////////////////////////////
// HDLDB main (stand alone executable)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ include
#include <print>
#include <iostream>

// test include
#include <Packet.hpp>

int main() {
    std::println("Started 'test-packet'.");

    rsp::Packet packet { "unix-socket" };

    std::println("Waiting for message from client.");
    std::cout << packet.rx(false);
    std::println("Sending a message to client.");
    packet.tx("TX test", false);
    std::println("Ending 'test-packet'.");

    return 0;
}
