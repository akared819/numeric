// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only, single-client loopback relay. CA UDP search replies advertise a
// TCP port and optionally an IP address; redirect those fields to this relay.
// https://docs.epics-controls.org/en/latest/internal/ca_protocol.html
#include <osiSock.h>
#ifndef _WIN32
#include <signal.h>
#endif
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned short backendPort = 25064, relayPort = 25066;
struct SocketRuntime {
    bool attached = osiSockAttach() != 0;
    ~SocketRuntime() { if (attached) osiSockRelease(); }
};
struct Socket {
    SOCKET value;
    explicit Socket(SOCKET fd = INVALID_SOCKET) : value(fd) {}
    ~Socket() { reset(); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    operator SOCKET() const { return value; }
    void reset(SOCKET fd = INVALID_SOCKET) {
        if (value != INVALID_SOCKET) epicsSocketDestroy(value);
        value = fd;
    }
};
sockaddr_in endpoint(unsigned short port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    return address;
}
SOCKET listener(int type) {
    SOCKET fd = epicsSocketCreate(AF_INET, type, 0);
    int one = 1;
    const auto address = endpoint(relayPort);
    if (fd == INVALID_SOCKET || setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char*>(&one), sizeof(one)) ||
        bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) ||
        (type == SOCK_STREAM && listen(fd, 2))) {
        if (fd != INVALID_SOCKET) epicsSocketDestroy(fd);
        return INVALID_SOCKET;
    }
    return fd;
}
bool forward(SOCKET from, SOCKET to) {
    char buffer[16384];
    const int length = recv(from, buffer, sizeof(buffer), 0);
    if (length <= 0) return false;
    for (int offset = 0; offset < length;) {
        const int sent = send(to, buffer + offset, length - offset, 0);
        if (sent <= 0) return false;
        offset += sent;
    }
    return true;
}
void redirectSearchReplies(unsigned char* data, std::size_t length) {
    // Each CA header is 16 bytes; the payload size includes alignment padding.
    for (std::size_t offset = 0; offset + 16 <= length;) {
        unsigned char* header = data + offset;
        const unsigned command = (static_cast<unsigned>(header[0]) << 8) | header[1];
        const unsigned payload = (static_cast<unsigned>(header[2]) << 8) | header[3];
        if (payload == 0xffff || offset + 16 + payload > length) return;
        if (command == 6 && payload == 8) {
            header[4] = static_cast<unsigned char>(relayPort >> 8);
            header[5] = static_cast<unsigned char>(relayPort);
            // 0xffffffff tells the CA client to use the datagram sender's IP.
            std::memset(header + 8, 0xff, 4);
        }
        offset += 16 + payload;
    }
}
}
int main() {
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    SocketRuntime runtime;
    if (!runtime.attached) return 1;
    Socket udp(listener(SOCK_DGRAM)), tcp(listener(SOCK_STREAM));
    Socket backendUdp(epicsSocketCreate(AF_INET, SOCK_DGRAM, 0));
    if (udp == INVALID_SOCKET || tcp == INVALID_SOCKET || backendUdp == INVALID_SOCKET) {
        std::fprintf(stderr, "relay setup: socket error %d\n", SOCKERRNO); return 1;
    }
    const auto backend = endpoint(backendPort);
    sockaddr_in clientUdp{};
    bool haveClient = false;
    Socket client, server;
    std::puts("ready"); std::fflush(stdout);
    for (;;) {
        fd_set readable;
        FD_ZERO(&readable);
        SOCKET highest = 0;
        for (SOCKET fd : {udp.value, backendUdp.value, tcp.value, client.value, server.value}) {
            if (fd == INVALID_SOCKET) continue;
            if (!FD_IN_FDSET(fd)) { std::fputs("relay descriptor exceeds FD_SETSIZE\n", stderr); return 1; }
            FD_SET(fd, &readable);
            highest = (std::max)(highest, fd);
        }
#ifdef _WIN32
        const int nfds = 0; // Winsock ignores this argument; SOCKET is pointer-sized.
#else
        const int nfds = highest + 1;
#endif
        if (select(nfds, &readable, nullptr, nullptr, nullptr) < 0) {
            if (SOCKERRNO == SOCK_EINTR) continue;
            std::fprintf(stderr, "relay select: socket error %d\n", SOCKERRNO); return 1;
        }
        if (FD_ISSET(udp, &readable)) {
            char data[16384]; osiSocklen_t size = sizeof(clientUdp);
            const int count = recvfrom(udp, data, sizeof(data), 0, reinterpret_cast<sockaddr*>(&clientUdp), &size);
            if (count > 0) {
                haveClient = true;
                sendto(backendUdp, data, count, 0, reinterpret_cast<const sockaddr*>(&backend), sizeof(backend));
            }
        }
        if (FD_ISSET(backendUdp, &readable)) {
            char data[16384];
            const int count = recv(backendUdp, data, sizeof(data), 0);
            if (count > 0 && haveClient) {
                redirectSearchReplies(reinterpret_cast<unsigned char*>(data), static_cast<std::size_t>(count));
                sendto(udp, data, count, 0, reinterpret_cast<const sockaddr*>(&clientUdp), sizeof(clientUdp));
            }
        }
        if (FD_ISSET(tcp, &readable)) {
            const SOCKET incoming = epicsSocketAccept(tcp, nullptr, nullptr);
            if (incoming != INVALID_SOCKET) {
                client.reset(incoming);
                server.reset(epicsSocketCreate(AF_INET, SOCK_STREAM, 0));
                if (server == INVALID_SOCKET || connect(server, reinterpret_cast<const sockaddr*>(&backend), sizeof(backend))) {
                    client.reset(); server.reset();
                }
            }
            continue;
        }
        if (client != INVALID_SOCKET && server != INVALID_SOCKET &&
            ((FD_ISSET(client, &readable) && !forward(client, server)) ||
             (FD_ISSET(server, &readable) && !forward(server, client)))) {
            client.reset(); server.reset();
        }
    }
}
