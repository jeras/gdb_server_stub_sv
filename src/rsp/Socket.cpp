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

    // create a UNIX socket and mark it as passive
    int socket_unix_listen(const char* name) {
        struct sockaddr_un server;

        vpi_printf("DPI-C: Creating UNIX socket %s\n", name);
        // check file name length
        if (strlen(name) == 0 || strlen(name) > sizeof(server.sun_path)-1) {
            vpi_printf("DPI-C: Server UNIX socket path too long: %s\n", name);
            return -1;
        }

        // delete UNIX socket file if it exists
        if (remove(name) == -1 && errno != ENOENT) {
            vpi_printf("DPI-C: Failed to remove UNIX socket file %s\n", name);
        }

        // create UNIX socket file descriptor
        m_SocketFd = socket(AF_UNIX, SOCK_STREAM, 0);
        vpi_printf("DPI-C: UNIX socket fd = %d\n", m_SocketFd);

        memset(&server, 0, sizeof(struct sockaddr_un));
        server.sun_family = AF_UNIX;
        strncpy(server.sun_path, name, sizeof(server.sun_path) - 1);

        // bind socket file descriptor to socket
        if (bind(m_SocketFd, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) != 0) {
            vpi_printf("DPI-C: Bind failed with errno = %0d.\n", errno);
            perror("DPI-C: socket bind:");
            return -1;
        } else {
            vpi_printf("DPI-C: Socket successfully binded...\n");
        }

        // mark the socket as passive (accepting connections from clients)
        if (listen(m_SocketFd, 5) == -1) {
            vpi_printf("DPI-C: Listen failed.\n");
            return -1;
        } else {
            vpi_printf("DPI-C: Server listening..\n");
        }

        // return socket fd
        return m_SocketFd;
    }

    // create a TCP socket and mark it as passive
    int socket_tcp_listen(const uint16_t port) {
        struct sockaddr_in server;

        // TCP socket create and verification
        m_SocketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_SocketFd == -1) {
            vpi_printf("DPI-C: Server TCP socket creation failed...\n");
            return -1;
        } else {
            vpi_printf("DPI-C: TCP socket fd = %d\n", m_SocketFd);
        }
        bzero(&server, sizeof(server));

        // assign IP, PORT
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(port);

        // Binding newly created socket to given IP and verification
        if ((bind(m_SocketFd, (struct sockaddr *)&server, sizeof(server))) != 0) {
            vpi_printf("DPI-C: Bind failed with errno = %0d.\n", errno);
            perror("DPI-C: socket bind:");
            return -1;
        } else {
            vpi_printf("DPI-C: Socket successfully binded...\n");
        }

        // Now server is ready to listen and verification
        if ((listen(m_SocketFd, 5)) != 0) {
            vpi_printf("DPI-C: Listen failed.\n");
            return -1;
        } else {
            vpi_printf("DPI-C: Server listening..\n");
        }

        // return socket fd
        return m_SocketFd;
    }

    void Socket::socketTcpListen(const uint16_t port) {
    	struct sockaddr_in address;
    	int reuse;

    	m_socketFd = socket(AF_INET, SOCK_STREAM, 0);
    	if (m_socketFd == -1)
    		throw std::system_error(errno, std::generic_category());

    	reuse = 1;
    	if (setsockopt(m_socketFd, SOL_TCP, TCP_NODELAY, &reuse, sizeof(reuse)) == -1)
    		goto err;
    	if (setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
    		goto err;

    	memset(&addr, 0, sizeof(address));
    	address.sin_family = AF_INET;
    	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    	address.sin_port = htons(port);

    	if (bind(m_socketFd, (struct sockaddr *)&address, sizeof(address)) == -1)
    		goto err;
    	if (listen(m_socketFd, 0) == -1)
    		goto err;

    	return;
    err:
    	close(m_socketFd);
    	throw std::system_error(errno, std::generic_category());
    }

    // accept connection from client (to a given socket fd)
    int socket_unix_accept () {
        vpi_printf("DPI-C: Waiting for client to connect...\n");
        m_clientFd = accept(m_SocketFd, NULL, NULL);
        if (m_clientFd < 0) {
            vpi_printf("DPI-C: Server accept failed with errno = %d.\n", errno);
            perror("DPI-C: socket accept:");
            exit(0);
        } else {
            vpi_printf("DPI-C: Accepted client connection fd = %d\n", m_clientFd);
        }

        // return client fd
        return m_clientFd;
    }

    // accept connection from client (to a given TCP socket fd)
    int socket_tcp_accept () {
        int len;
        struct sockaddr_in client;

        len = sizeof(client);

        // Accept the data packet from client and verification
        m_clientFd = accept(m_SocketFd, (struct sockaddr *)&client, &len);
        if (m_clientFd < 0) {
            vpi_printf("DPI-C: Server accept failed with errno = %d.\n", errno);
            perror("DPI-C: socket accept:");
            exit(0);
        }
        else {
            vpi_printf("DPI-C: Accepted client connection fd = %d\n", m_clientFd);
        }

        // disable the disable Nagle's algorithm in an attempt to speed up TCP
        len = 1;
        if (setsockopt(m_clientFd, IPPROTO_TCP, TCP_NODELAY, &len, sizeof(len)) != 0) {
            vpi_printf("DPI-C: Server socket options failed with errno = %d.\n", errno);
            perror("DPI-C: setsockopt:");
            exit(0);
        } else {
            vpi_printf("DPI-C: Server socket options set.\n");
        }

        // return client fd
        return m_clientFd;
    }

    // close connection from client
    int socket_close () {
        return close(m_clientFd);
        vpi_printf("DPI-C: Closed connection from client.");
    }

        // transmitter
    int socket_send (const svOpenArrayHandle data, int flags) {
        int status;
        status = send(m_clientFd, svGetArrayPtr(data), svSizeOfArray(data), flags);
        if (status == -1) {
            // https://en.wikipedia.org/wiki/Errno.h
            vpi_printf("DPI-C: SEND failed with errno = %0d.\n", errno);
            perror("DPI-C: socket send:");
            return -1;
        }
        return status;
    }

    // receiver
    int socket_recv (const svOpenArrayHandle data, int flags) {
        int status;
        status = recv(m_clientFd, svGetArrayPtr(data), svSizeOfArray(data), flags);
        if (status == -1) {
            // https://en.wikipedia.org/wiki/Errno.h
            vpi_printf("DPI-C: RECV failed with errno = %0d.\n", errno);
            perror("DPI-C: socket recv:");
            return -1;
        }
        return status;
    }

}
