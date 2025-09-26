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

// C++ includes
#include <byte>

// HDLDB includes
#include "Socket.h"

namespace rsp {

    // create a UNIX socket and mark it as passive
    void Socket::listenUnix(const std::string_view& name) {
        struct sockaddr_un server;

        std::print("SOCKET: Creating UNIX socket {}\n", name);
        // check file name length
        if (strlen(name) == 0 || strlen(name) > sizeof(server.sun_path)-1) {
    		throw std::system_error(errno, std::generic_category(std::format("SOCKET: Server UNIX socket path too long: {}\n", name)));
            return;
        }

        // delete UNIX socket file if it exists
        if (remove(name) == -1 && errno != ENOENT) {
    		throw std::system_error(errno, std::generic_category(std::format("SOCKET: Failed to remove UNIX socket file {}\n", name)));
            return;
        }

        // create UNIX socket file descriptor
        m_SocketFd = socket(AF_UNIX, SOCK_STREAM, 0);
        std::print("SOCKET: UNIX socket fd = {}\n", m_SocketFd);

        memset(&server, 0, sizeof(struct sockaddr_un));
        server.sun_family = AF_UNIX;
        strncpy(server.sun_path, name, sizeof(server.sun_path) - 1);

        // bind socket file descriptor to socket
        if (bind(m_SocketFd, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) != 0) {
            throw std::system_error(errno, std::generic_category("SOCKET: Bind failed\n"));
            return;
        } else {
            std::print("SOCKET: Socket successfully binded...\n");
        }

        // mark the socket as passive (accepting connections from clients)
        if (listen(m_SocketFd, 5) == -1) {
            throw std::system_error(errno, std::generic_category("SOCKET: Listen failed\n"));
            return;
        } else {
            std::print("SOCKET: Server listening..\n");
        }
    }

    // create a TCP socket and mark it as passive
    void Socket::listenTcp(const uint16_t port) {
        struct sockaddr_in server;

        // TCP socket create and verification
        m_SocketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_SocketFd == -1) {
    		throw std::system_error(errno, std::generic_category("SOCKET: Server TCP socket creation failed...\n"));
            return;
        } else {
            std::print("SOCKET: TCP socket fd = %d\n", m_SocketFd);
        }
        bzero(&server, sizeof(server));

        // assign IP, PORT
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(port);

        // Binding newly created socket to given IP and verification
        if ((bind(m_SocketFd, (struct sockaddr *)&server, sizeof(server))) != 0) {
            std::print("SOCKET: Bind failed with errno = %0d.\n", errno);
            perror("SOCKET: socket bind:");
            return -1;
        } else {
            std::print("SOCKET: Socket successfully binded...\n");
        }

        // Now server is ready to listen and verification
        if ((listen(m_SocketFd, 5)) != 0) {
            std::print("SOCKET: Listen failed.\n");
            return -1;
        } else {
            std::print("SOCKET: Server listening..\n");
        }
    }

    // accept connection from client (to a given socket fd)
    void Socket::acceptUnix () {
        std::print("SOCKET: Waiting for client to connect...\n");
        m_clientFd = accept(m_SocketFd, NULL, NULL);
        if (m_clientFd < 0) {
            std::print("SOCKET: Server accept failed with errno = %d.\n", errno);
            perror("SOCKET: socket accept:");
            exit(0);
        } else {
            std::print("SOCKET: Accepted client connection fd = %d\n", m_clientFd);
        }
    }

    // accept connection from client (to a given TCP socket fd)
    void Socket::acceptTcp () {
        int len;
        struct sockaddr_in client;

        len = sizeof(client);

        // Accept the data packet from client and verification
        m_clientFd = accept(m_SocketFd, (struct sockaddr *)&client, &len);
        if (m_clientFd < 0) {
            std::print("SOCKET: Server accept failed with errno = %d.\n", errno);
            perror("SOCKET: socket accept:");
            exit(0);
        }
        else {
            std::print("SOCKET: Accepted client connection fd = %d\n", m_clientFd);
        }

        // disable the disable Nagle's algorithm in an attempt to speed up TCP
        len = 1;
        if (setsockopt(m_clientFd, IPPROTO_TCP, TCP_NODELAY, &len, sizeof(len)) != 0) {
            std::print("SOCKET: Server socket options failed with errno = %d.\n", errno);
            perror("SOCKET: setsockopt:");
            exit(0);
        } else {
            std::print("SOCKET: Server socket options set.\n");
        }
    }

    // close connection from client
    void Socket::close () {
        return close(m_clientFd);
        std::print("SOCKET: Closed connection from client.");
    }

        // transmitter
    int Socket::send (const std::byte* data, const size_t size, int flags) {
        int status;
        status = send(m_clientFd, data, size, flags);
        if (status == -1) {
            // https://en.wikipedia.org/wiki/Errno.h
            throw std::system_error(errno, std::generic_category("SEND failed"));
            return -1;
        }
        return status;
    }

    // receiver
    int Socket::recv (const std::byte* data, const size_t size, int flags) {
        int status = recv(m_clientFd, data, size, flags);
        if (status == -1) {
            // https://en.wikipedia.org/wiki/Errno.h
            throw std::system_error(errno, std::generic_category("RECV failed"));
            return -1;
        }
        return status;
    }

}
