#ifndef LZ_H
#define LZ_H

#include "common.h"

typedef struct {
    int header; // 4 byte header
    int num_opcodes;
    int num_operand_bytes;
    int uncompressed_size;
    u8 data[];
} compressed;

compressed* compress(u8* data, int data_len);
int decompress(compressed* comp, u8** output_ptr);

#endif