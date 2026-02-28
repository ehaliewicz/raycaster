#include <stdio.h>
#include <assert.h>
#include <stdio.h>

#include <synchapi.h>
#include <windows.h>
#include <winsock.h>
#include <winternl.h>

#include "common.h"
#include "network.h"
#include "raycast.h"
#include "thread.h"


#define SERVER_PORT 27015

typedef struct {
    SOCKET sock;
    struct sockaddr_in peer_addr;
    int peer_known;
    int valid;
} UdpConnection;

UdpConnection conn;
void* setup_udp(const char* peer_ip, int server_mode) {

    conn = (UdpConnection){0};
    conn.valid = 0;
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    // 1️⃣ create UDP socket
    conn.sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (conn.sock == INVALID_SOCKET) {
        printf("Failed to create socket\n");
        return &conn;
    }

    // 2️⃣ make it non-blocking
    u_long mode = 1;
    ioctlsocket(conn.sock, FIONBIO, &mode);

    if (server_mode) {
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SERVER_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(conn.sock, (struct sockaddr*)&addr, sizeof(addr));
        conn.peer_known = 0; // we don’t know client yet
        printf("Server listening on port %d\n", SERVER_PORT);
    } else {
        conn.peer_addr.sin_family = AF_INET;
        conn.peer_addr.sin_port = htons(SERVER_PORT);
        conn.peer_addr.sin_addr.s_addr = inet_addr(peer_ip);
        conn.peer_known = 1;
        printf("Client sending to %s:%d\n", peer_ip, SERVER_PORT);
    }
    conn.valid = 1;
    return &conn;
}

//int udp_frame(void* vconn, msg *send_msg, msg *recv_msg, int recv, int send) {
int udp_frame(void* vconn, float my_pos[4], float other_pos[4], int recv, int send) {
    if(vconn == NULL) {
        return 0;
    }
    UdpConnection* conn = (UdpConnection*)vconn;
    if(!conn->valid) {
        return 0;
    }

    char buf[sizeof(float[4])];
    memcpy(buf, my_pos, sizeof(float[4]));

    // 1️⃣ send to peer if known
    if (send && conn->peer_known) {
        sendto(conn->sock, buf, sizeof(buf), 0,
               (struct sockaddr*)&conn->peer_addr, sizeof(conn->peer_addr));
    }

    // 2️⃣ receive all pending packets
    if(recv) {
        while (1) {
            struct sockaddr_in from;
            int from_len = sizeof(from);
            int bytes = recvfrom(conn->sock, buf, sizeof(buf), 0,
                                (struct sockaddr*)&from, &from_len);
            if (bytes <= 0) break; // no more packets

            // save peer address if server and unknown
            if (!conn->peer_known) {
                conn->peer_addr = from;
                conn->peer_known = 1;
            }

            if (bytes == sizeof(float[4])) {
                memcpy(other_pos, buf, sizeof(float[4]));
                return 1;
            }
        }
    }
    return 0;
}