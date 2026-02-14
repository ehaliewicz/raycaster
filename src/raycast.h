#ifndef THREAD_H
#define THREAD_H

#include "common.h"


//void* thread_pool_create(int cpu_threads);
//void thread_pool_add_work(void* tp, void (*func)(void*), void* arg_var);
//void thread_pool_destroy(void* tp);
void draw_first_person_level(
    u8* output, edit_wall_id* edit_id_buffer,
    int start_x, int end_x, 
    int frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, int pitch,
    int editor_mode_enabled, int editor_selected_map_idx, wall_side editor_selected_side
);

#endif 