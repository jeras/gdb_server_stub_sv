///////////////////////////////////////////////////////////////////////////////
// HDLDB main (stand alone executable)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// C++ includes
#include <print>

// C++ libraries
#include <cxxopts.hpp>

// HDLDB includes
#include <hdldb.hpp>

int main(int argc, char* argv[]) {
    // CLI argument list
    cxxopts::Options options("HDLDB", "CPU debug server for recorded HDL simulations.");
    options.add_options()
        ("d,debug", "Enable debugging") // a bool parameter
        ("p,port", "TCP port", cxxopts::value<std::uint16_t>()->default_value("1234"))
        ("s,socket", "UNIX socket (default is 'unix-socket')", cxxopts::value<std::string>()->default_value("unix-socket"))
        ("i,input", "HDL simulation trace record input file name", cxxopts::value<std::string>())
        ("o,output", "HDLDB processed trace output file name", cxxopts::value<std::string>())
        ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
    ;

    // TCP/UNIX socket arguments
    std::uint16_t tcp_port;
    std::string unix_socket;

    try {
        auto result{ options.parse(argc, argv) };
        if (result.count("help")) {
                std::cout << options.help() << std::endl;
                return 0;
        }
        if (result.count("port")) {
            tcp_port = result["port"].as<std::uint16_t>();
            std::println("Server will listen on TCP port {}.", tcp_port);
        }
        if (result.count("socket")) {
            unix_socket = result["socket"].as<std::uint16_t>();
            std::println("Server will listen on TCP port {}.", unix_socket);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    }

    SystemHdlDb shadow { };

    ProtocolHdlDb protocol { "1234", shadow };

    protocol.loop();

    return 0;
}
