///////////////////////////////////////////////////////////////////////////////
// socket
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <netdb.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// C includes
#include <cstdint>
#include <cstddef>

// C++ includes
#include <string_view>
#include <span>

namespace rsp {

    class Socket {

        // socket file descriptor
        int m_socketFd;

        // client file descriptor
        int m_clientFd;

        // constructor
        Socket(const std::string_view&);
        // destructor
        ~Socket();

        // create a UNIX socket and mark it as passive
        void listenUnix (const std::string_view&);
        // create a TCP socket and mark it as passive
        void listenTcp (const std::uint16_t port);
        // accept connection from client (to a given socket fd)
        void acceptUnix ();
        // accept connection from client (to a given TCP socket fd)
        void acceptTcp ();

    protected:
        // stream
        int& stream {m_clientFd};

        // transmitter
        ssize_t send (std::span<const std::byte>, int flags) const;
        // receiver
        ssize_t recv (std::span<      std::byte>, int flags) const;
    };

};
