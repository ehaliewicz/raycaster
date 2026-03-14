#ifndef RAYCAST_H
#define RAYCAST_H

#include "common.h"

#define FOCAL_LENGTH (FP_SCREEN_WIDTH / (2.0f * 1.0f)) //tanf(1.57f/2.0f)))
#define HEIGHT_SCALE (MAX_WALL_HEIGHT/8)
#define HALF_SCREEN_HEIGHT (FP_SCREEN_HEIGHT/2)


//void* thread_pool_create(int cpu_threads);
//void thread_pool_add_work(void* tp, void (*func)(void*), void* arg_var);
//void thread_pool_destroy(void* tp);
void clear_requested_sprites();


void launch_render_frame(
    u32* output, edit_wall_id* edit_id_buffer, u16* z_buffer,
    int start_x, int end_x, 
    int flash_frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, float pitch,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side
);
void join_render_frame();


int request_draw_sprite(float x, float y, float z, u8 image_idx);
int request_draw_screen_space_sprite(float screen_x0, float screen_x1, float screen_y0, float screen_y1, u8 image_idx);

void init_raycast_module();


typedef struct {
    float x,y,z;
} player_pos;


extern player_pos pos0, pos1;
extern player_pos *cur_other_player_pos;

#endif 