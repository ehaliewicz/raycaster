#ifndef LZ_H
#define LZ_H

#include "common.h"

typedef struct {
    int num_opcodes;
    int num_operand_bytes;
    int uncompressed_size;
    u8 data[];
} compressed;

compressed* compress(u8* data, int data_len);
u8* decompress(compressed* comp);

#endif