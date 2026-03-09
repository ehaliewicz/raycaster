#include "common.h"


#define CONSOLE_MAX_BUF_SIZE 1024

typedef struct {
    char buf[256];
    u16 len;
} console_line;

static u8 *console_buf;
static int console_open = 0;
static int cur_line_len_idx = 0;

void console_insert_key(char c) {
    console_buf[cur_line_len_idx]++;
    console_buf[console_buf[cur_line_len_idx]] = c;
    console_buf[cur_line_len_idx]++;
}

void console_init() {
    console_open = 0;
    console_buf = my_calloc(1, CONSOLE_MAX_BUF_SIZE);
}