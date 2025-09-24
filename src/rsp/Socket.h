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

namespace rsp {

    class Socket {

        // socket file descriptor
        int m_socketFd;

        // client file descriptor
        int client_fd;


        // create a UNIX socket and mark it as passive
        int socketUnixListen(const char* name);
        // create a TCP socket and mark it as passive
        int socket_tcp_listen(const std::uint16_t port);
        // accept connection from client (to a given socket fd)
        int socket_unix_accept ();
        // accept connection from client (to a given TCP socket fd)
        int socket_tcp_accept ();
        // close connection from client
        int socket_close ();
        // transmitter
        int socket_send (const svOpenArrayHandle data, int flags);
        // receiver
        int socket_recv (const svOpenArrayHandle data, int flags);

}
