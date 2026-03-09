#include "common.h"
#include "lz.h"
#include "my_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LZ_HEADER 0xB0B2

#define MATCH_LEN_BITS 8
#define MATCH_OFFSET_BITS 8

#define MAX_OPCODES (1024*1024*4)
#define MAX_OPERANDS (1024*1024*4)
int num_opcodes = 0;
u8* opcode_output_buf = NULL;
u8* operand_output_buf = NULL;

int num_bits = 0;

void output_bit(u8 bit) {
    if((num_bits>>3) >= MAX_OPCODES) {
        debug_printf("error compressing map, too many lz opcodes\n");
        exit(1);
    }
    if(bit) {
        opcode_output_buf[num_bits>>3] |= (bit << (num_bits&0b111));
    } else {
        opcode_output_buf[num_bits>>3] &= ~(bit<<(num_bits&0b111));
    }
    num_bits++;
}

int num_bytes = 0;
void output_byte(u8 byte) {
    if(num_bytes >= MAX_OPERANDS) {
        debug_printf("error compressing map, too many lz operands\n");
        exit(1);
    }
    operand_output_buf[num_bytes++] = byte;
}
void output_adjusted_byte(u8 byte) {
    output_byte(byte-1);
}


#define LITERAL_BIT 0
#define COPY_BIT 1 
// LITERAL is an 8-bit literal len (1-256)
// COPY is an 8-bit offset (1 to 256), 8-bit len (1-256)

compressed* compress(u8* data, int data_len) {
    if(opcode_output_buf == NULL) {
        opcode_output_buf = my_calloc(MAX_OPCODES/2, "lz compression scratch space");
    }
    if(operand_output_buf == NULL) {
        operand_output_buf = my_calloc(MAX_OPERANDS, "lz compression scratch space");
    }

    num_bits = 0;
    num_opcodes = 0;
    num_bytes = 0;
    
    int cur_cap = 8;
    int cur_len = 0;

    int idx = 0;
    
    int previous_was_copy_literal = 0;
    int previous_copy_literal_len = 0;
    int previous_copy_len_idx = 0;

    while(idx < data_len) {

        // check all possibilities
        
        int best_match_offset = 0;
        int best_match_len = 0;
        for(int offset = 1; offset <= (1<<MATCH_OFFSET_BITS); offset++) {
            if(idx-offset < 0) {
                break;
            }
            if(offset == 0) {
                continue;
            }
            int cur_match_len = -1;
            for(int len = 1; len <= (1<<MATCH_LEN_BITS); len++) {

                if(idx+len >= data_len) {
                    break;
                }

                if(data[(idx-offset)+len-1] != data[idx+len-1]) {
                    break;
                }
                cur_match_len = len;
            }
            if(cur_match_len > best_match_len) {
                best_match_len = cur_match_len;
                best_match_offset = offset;
            }
        }

        if(best_match_len >= 2) {
            //debug_printf("outputting match from %llu of len %llu\n", best_match_offset, best_match_len);
            // got a copy that's worth it
            idx += best_match_len;
            output_bit(COPY_BIT);
            output_byte(best_match_offset-1);
            output_byte(best_match_len-1);
            previous_was_copy_literal = 0;
        } else {
            //debug_printf("outputting literal %i\n", data[idx]);
            output_bit(LITERAL_BIT);
            previous_copy_len_idx = num_bytes;
            //output_byte(0);
            output_byte(data[idx]);
            idx += 1;
            previous_was_copy_literal = 1;
            previous_copy_literal_len = 1;
        }
    }
    int num_opcode_bytes = (num_bits+7)>>3;
    int num_operand_bytes = num_bytes;
    compressed* res = my_malloc(sizeof(compressed)+(num_opcode_bytes+num_operand_bytes), "compressed output");
    res->header = LZ_HEADER;
    res->num_opcodes = num_bits;
    res->num_operand_bytes = num_operand_bytes;
    res->uncompressed_size = data_len;
    my_memcpy(res->data, opcode_output_buf, num_opcode_bytes);
    my_memcpy(res->data+num_opcode_bytes, operand_output_buf, num_operand_bytes);
    return res;
}


int decompress(compressed* comp, u8** output_ptr) {
    if(comp->header != LZ_HEADER) {
        return -1;
    }
    int num_opcodes = comp->num_opcodes;
    int num_operand_bytes = comp->num_operand_bytes;
    u8* output = my_malloc(sizeof(u8)*comp->uncompressed_size, "decompressed output");

    int num_opcode_bytes = (num_opcodes+7)>>3;
    u8* opcode_bytes = comp->data;
    u8* operand_bytes = comp->data+num_opcode_bytes;
    int operand_idx = 0;

    int output_idx = 0;
    for(int op = 0; op < num_opcodes; op++) {
        u8 opcode = (opcode_bytes[op>>3] >> (op&0b111)) & 0b1;
        if(opcode == COPY_BIT) {
            // copy
            int offset = operand_bytes[operand_idx++]+1;
            int copy_len = operand_bytes[operand_idx++]+1;
            for(int i = 0; i < copy_len; i++) {
                output[output_idx] = output[output_idx-offset];
                output_idx++;
            }
        } else {
            // literal
            output[output_idx++] = operand_bytes[operand_idx++];
        }
    }

    if(output_idx != comp->uncompressed_size){
        debug_printf("Decompressed size doesn't match!\n");
        return -1;
    }

    *output_ptr = output;
    return output_idx;

}
