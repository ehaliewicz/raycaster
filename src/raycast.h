#ifndef RAYCAST_H
#define RAYCAST_H

#include "common.h"


//void* thread_pool_create(int cpu_threads);
//void thread_pool_add_work(void* tp, void (*func)(void*), void* arg_var);
//void thread_pool_destroy(void* tp);
void draw_first_person_level(
    u32* output, edit_wall_id* edit_id_buffer, float* z_buffer,
    int start_x, int end_x, 
    int flash_frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, float pitch,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side
);

void request_draw_sprite(float x, float y, float z, u8 image_idx);

void init_raycast_module();


typedef struct {
    float x,y,z;
} player_pos;


extern player_pos pos0, pos1;
extern player_pos *cur_other_player_pos;

#endif 