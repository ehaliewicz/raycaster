#ifndef NETWORK_H
#define NETWORK_H

typedef enum {
    CONNECT,
    SEND_POS,
    EDIT
} msg_type;

typedef struct {
    edit_wall_id edit_id;
    int keystroke;
} edit_cmd;

typedef struct {
    msg_type typ;
    union {
        float position[4];
        edit_cmd edit;
    };
} msg;

void* setup_udp(const char* peer_ip, int server_mode);

//int udp_frame(void* vconn, msg *send_msg, msg *recv_msg, int recv, int send);
int udp_frame(void* vconn, float my_pos[4], float other_pos[4], int recv, int send);

#endif 