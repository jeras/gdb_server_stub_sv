///////////////////////////////////////////////////////////////////////////////
// HDLDB main (stand alone executable)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ includes
#include <print>
#include <memory>

// C++ libraries
#include <cxxopts.hpp>

// HDLDB includes
#include <hdldb.hpp>

int main(int argc, char* argv[]) {
    // CLI argument list
    cxxopts::Options options("HDLDB", "CPU debug server for recorded HDL simulations.");
    options.add_options()
        ("h,help", "Print help")
        ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
        ("d,debug", "Enable debugging")
        ("p,port", "TCP port", cxxopts::value<int>()->default_value("1234"))
        ("s,socket", "UNIX socket", cxxopts::value<std::string>()->default_value("unix-socket"))
        ("i,input", "HDL simulation trace record input file name", cxxopts::value<std::string>())
        ("o,output", "HDLDB processed trace output file name", cxxopts::value<std::string>())
    ;

    SystemHdlDb shadow { };
    std::unique_ptr<ProtocolHdlDb> protocol;

    try {
        auto result{ options.parse(argc, argv) };
        if (result.count("help")) {
            std::print("{}", options.help());
            return 0;
        }
        // if port is defined, open TCP socket port, otherwise
        // use a defined or default UNIX socket name
        if (result.count("port")) {
            std::uint16_t socket_port = result["port"].as<std::uint16_t>();
            protocol = std::make_unique<ProtocolHdlDb>(socket_port, shadow);
            std::println("Server will listen on TCP port {}.", socket_port);
        } else {
            socket_name = result["socket"].as<std::string>();
            std::println("Server will listen on TCP port {}.", socket_name);
            protocol = std::make_unique<ProtocolHdlDb>(socket_name, shadow);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    }

    // start main loop
    protocol->loop();

    return 0;
}
