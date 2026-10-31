// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only, single-client loopback relay. CA UDP search replies advertise a
// TCP port and optionally an IP address; redirect those fields to this relay.
// https://docs.epics-controls.org/en/latest/internal/ca_protocol.html
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned short backendPort = 25064, relayPort = 25066;
sockaddr_in endpoint(unsigned short port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    return address;
}
int listener(int type) {
    int fd = socket(AF_INET, type, 0);
    int one = 1;
    const auto address = endpoint(relayPort);
    if (fd < 0 || setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) ||
        bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) ||
        (type == SOCK_STREAM && listen(fd, 2))) {
        if (fd >= 0) close(fd);
        return -1;
    }
    return fd;
}
bool forward(int from, int to) {
    char buffer[16384];
    const ssize_t length = recv(from, buffer, sizeof(buffer), 0);
    if (length <= 0) return false;
    for (ssize_t offset = 0; offset < length;) {
        const ssize_t sent = send(to, buffer + offset, length - offset, 0);
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
    signal(SIGPIPE, SIG_IGN);
    const int udp = listener(SOCK_DGRAM), tcp = listener(SOCK_STREAM);
    const int backendUdp = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp < 0 || tcp < 0 || backendUdp < 0) { std::perror("relay setup"); return 1; }
    const auto backend = endpoint(backendPort);
    sockaddr_in clientUdp{};
    bool haveClient = false;
    int client = -1, server = -1;
    std::puts("ready"); std::fflush(stdout);
    for (;;) {
        pollfd fds[] = {{udp, POLLIN, 0}, {backendUdp, POLLIN, 0}, {tcp, POLLIN, 0},
                        {client, POLLIN, 0}, {server, POLLIN, 0}};
        if (poll(fds, 5, -1) < 0) { if (errno == EINTR) continue; std::perror("relay poll"); return 1; }
        if (fds[0].revents & POLLIN) {
            unsigned char data[16384]; socklen_t size = sizeof(clientUdp);
            const ssize_t count = recvfrom(udp, data, sizeof(data), 0, reinterpret_cast<sockaddr*>(&clientUdp), &size);
            if (count > 0) {
                haveClient = true;
                sendto(backendUdp, data, count, 0, reinterpret_cast<const sockaddr*>(&backend), sizeof(backend));
            }
        }
        if (fds[1].revents & POLLIN) {
            unsigned char data[16384];
            const ssize_t count = recv(backendUdp, data, sizeof(data), 0);
            if (count > 0 && haveClient) {
                redirectSearchReplies(data, static_cast<std::size_t>(count));
                sendto(udp, data, count, 0, reinterpret_cast<const sockaddr*>(&clientUdp), sizeof(clientUdp));
            }
        }
        if (fds[2].revents & POLLIN) {
            const int incoming = accept(tcp, nullptr, nullptr);
            if (incoming >= 0) {
                if (client >= 0) close(client);
                if (server >= 0) close(server);
                client = incoming; server = socket(AF_INET, SOCK_STREAM, 0);
                if (server < 0 || connect(server, reinterpret_cast<const sockaddr*>(&backend), sizeof(backend))) {
                    close(client); if (server >= 0) close(server); client = server = -1;
                }
            }
            continue;
        }
        if (client >= 0 && server >= 0 &&
            ((fds[3].revents && (!(fds[3].revents & POLLIN) || !forward(client, server))) ||
             (fds[4].revents && (!(fds[4].revents & POLLIN) || !forward(server, client))))) {
            close(client); close(server); client = server = -1;
        }
    }
}
