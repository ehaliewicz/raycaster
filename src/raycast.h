#ifndef RAYCAST_H
#define RAYCAST_H

#include "common.h"
#include "6dof.h"

#define FOCAL_LENGTH (RENDER_WIDTH / (2.0f * 1.0f)) //tanf(1.57f/2.0f)))
#define HEIGHT_SCALE (MAX_WALL_HEIGHT/8)
#define HALF_SCREEN_HEIGHT (RENDER_HEIGHT/2)


//void* thread_pool_create(int cpu_threads);
//void thread_pool_add_work(void* tp, void (*func)(void*), void* arg_var);
//void thread_pool_destroy(void* tp);
void clear_requested_sprites();

#define MAX_STEPS 128



void launch_render_frame(
    u32* output, edit_wall_id* edit_id_buffer, u16* z_buffer,
    int start_x, int end_x, 
    int flash_frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, float pitch,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side
);

void join_render_frame();


int request_draw_sprite(float x, float y, float z, int entity_idx, u8 image_idx);

int request_draw_screen_space_sprite(
    float screen_x0, float screen_x1, float screen_y0, float screen_y1, 
    u8 image_idx, int entity_id
);

void init_raycast_module();


typedef struct {
    float x,y,z;
} player_pos;


extern player_pos pos0, pos1;
extern player_pos *cur_other_player_pos;

typedef struct {
    float start_height, end_height;
} slope_heights;

slope_heights get_slope_heights(int in_start_cell, int map_x, int map_z, int next_map_x, int next_map_z, 
    float hit_x, float hit_z, float next_hit_x, float next_hit_z,
    wall_side side, cell_types cell_type, int step_x, int step_z, float ray_origin_x, float ray_origin_z, float first_height, float second_height);
    
#endif 