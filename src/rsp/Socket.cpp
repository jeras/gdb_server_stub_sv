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
#include <string>
#include <print>

// HDLDB includes
#include "Socket.hpp"

namespace rsp {

    // open UNIX socket server
    Socket::Socket (std::string_view name) {
        listenUnix(name);
        m_tcp = false;
        accept();
    }

    // open TCP port server
    Socket::Socket (std::uint16_t port) {
        listenTcp(port);
//        static_cast<uint16_t>(std::stoi(name.data()+1)));
        m_tcp = true;
        accept();
    }

    // close connection from client
    Socket::~Socket () {
        int status { close(m_clientFd) };
        if (status == -1) {
            throw std::system_error(errno, std::generic_category(), "CLOSE failed");
        } else {
            std::print("SOCKET: Closed connection from client.");
        }
    }

    // create a UNIX socket and mark it as passive
    void Socket::listenUnix(std::string_view name) {
        struct sockaddr_un server;

        std::print("SOCKET: Creating UNIX socket {}\n", name);
        // check file name length
        if (name.size() == 0 || name.size() > sizeof(server.sun_path)-1) {
    		throw std::system_error(errno, std::generic_category(), std::format("SOCKET: Server UNIX socket path too long: {}\n", name));
            return;
        }

        // delete UNIX socket file if it exists
        if (::remove(name.data()) == -1 && errno != ENOENT) {
    		throw std::system_error(errno, std::generic_category(), std::format("SOCKET: Failed to remove UNIX socket file {}\n", name));
            return;
        }

        // create UNIX socket file descriptor
        m_socketFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        std::print("SOCKET: UNIX socket fd = {}\n", m_socketFd);

        ::memset(&server, 0, sizeof(struct sockaddr_un));
        server.sun_family = AF_UNIX;
        ::strncpy(server.sun_path, name.data(), sizeof(server.sun_path) - 1);

        // bind socket file descriptor to socket
        if (::bind(m_socketFd, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) != 0) {
            throw std::system_error(errno, std::generic_category(), "SOCKET: Bind failed\n");
            return;
        } else {
            std::print("SOCKET: Socket successfully binded...\n");
        }

        // mark the socket as passive (accepting connections from clients)
        if (::listen(m_socketFd, 5) == -1) {
            throw std::system_error(errno, std::generic_category(), "SOCKET: Listen failed\n");
            return;
        } else {
            std::print("SOCKET: Server listening..\n");
        }
    }

    // create a TCP socket and mark it as passive
    void Socket::listenTcp(const uint16_t port) {
        struct sockaddr_in server;

        // TCP socket create and verification
        m_socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_socketFd == -1) {
    		throw std::system_error(errno, std::generic_category(), "SOCKET: Server TCP socket creation failed...\n");
            return;
        } else {
            std::print("SOCKET: TCP socket fd = %d\n", m_socketFd);
        }
        ::bzero(&server, sizeof(server));

        // assign IP, PORT
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(port);

        // Binding newly created socket to given IP and verification
        if ((::bind(m_socketFd, (struct sockaddr *)&server, sizeof(server))) != 0) {
            std::print("SOCKET: Bind failed with errno = %0d.\n", errno);
            return;
        } else {
            std::print("SOCKET: Socket successfully binded...\n");
        }

        // Now server is ready to listen and verification
        if ((::listen(m_socketFd, 5)) != 0) {
            std::print("SOCKET: Listen failed.\n");
            return;
        } else {
            std::print("SOCKET: Server listening..\n");
        }
    }

    // accept connection from client (to a given socket fd)
    void Socket::acceptUnix () {
        std::print("SOCKET: Waiting for client to connect...\n");
        m_clientFd = ::accept(m_socketFd, NULL, NULL);
        if (m_clientFd < 0) {
            std::print("SOCKET: Server accept failed with errno = %d.\n", errno);
            exit(0);
        } else {
            std::print("SOCKET: Accepted client connection fd = %d\n", m_clientFd);
        }
    }

    // accept connection from client (to a given TCP socket fd)
    void Socket::acceptTcp () {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);

        // Accept the data packet from client and verification
        m_clientFd = ::accept(m_socketFd, (struct sockaddr *)&client, &len);
        if (m_clientFd < 0) {
            std::print("SOCKET: Server accept failed with errno = %d.\n", errno);
            exit(0);
        }
        else {
            std::print("SOCKET: Accepted client connection fd = %d\n", m_clientFd);
        }

        // disable the disable Nagle's algorithm in an attempt to speed up TCP
        len = 1;
        if (::setsockopt(m_clientFd, IPPROTO_TCP, TCP_NODELAY, &len, sizeof(len)) != 0) {
            std::print("SOCKET: Server socket options failed with errno = %d.\n", errno);
            perror("SOCKET: setsockopt:");
            exit(0);
        } else {
            std::print("SOCKET: Server socket options set.\n");
        }
    }

    void Socket::accept () {
        if (m_tcp)  acceptTcp();
        else        acceptUnix();
    }

    // transmitter
    ssize_t Socket::send (std::span<const std::byte> data, int flags) const {
        ssize_t status { ::send(m_clientFd, data.data(), data.size(), flags) };
        if (status == -1) {
            // https://en.wikipedia.org/wiki/Errno.h
            throw std::system_error(errno, std::generic_category(), "SEND failed");
            return -1;
        }
        return status;
    }

    // receiver
    ssize_t Socket::recv (std::span<std::byte> data, int flags) const {
        ssize_t status { ::recv(m_clientFd, data.data(), data.size(), flags) };
        if (status == -1) {
            // https://en.wikipedia.org/wiki/Errno.h
            throw std::system_error(errno, std::generic_category(), "RECV failed");
            return -1;
        }
        return status;
    }

}
