//author https://github.com/autergame

#include <assert.h>
//#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "collision.h"
#include "draw.h"
#include "my_defs.h"
#include "raycast.h"
#include "resources.h"
#include "platform.h"



#ifdef PLATFORM_WEB
#define NIGHT_FOG_COL ((0<<24)|(0<<16)|(0<<8)|(255<<0))
#define FOG_COL ((255<<24)|(196<<16)|(162<<8)|(103<<0))
#else
#define NIGHT_FOG_COL ((255<<24)|(0<<16)|(0<<8)|(0<<0))
#define FOG_COL ((255<<24)|(103<<16)|(162<<8)|(196<<0))
#endif

int project_to_screen(float height, float dist, float pitch, float player_z) {
    return pitch + HALF_SCREEN_HEIGHT - (HEIGHT_SCALE * (((height- player_z) * FOCAL_LENGTH / dist) / MAX_WALL_HEIGHT)); 
}


#define DEGREES_TO_RAD(deg) ((deg)*.0174f)


typedef struct {
    float diag_wall_u;
    float diag_perp_dist;
    float mid_flat_u;
    float mid_flat_v;
} diag_intersect;

const float diag_dx[NUM_CELL_TYPES] = {
    0.0f, // dummy entry for normal walls
    1.0f, // NE_TO_SW_DIAG
    1.0f,  // NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    0.0f,// DOOR_Y=5,
    0.0f,// DOOR_X=6
    0.0f, // THIN_WALL_X
    1.0f // THIN_WALL_Y
};

const float diag_dy[NUM_CELL_TYPES] = {
    1.0f, // dummy entry for normal walls
    -1.0f, // NE_TO_SW_DIAG
    1.0f,  // NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    0.0f,// DOOR_Y=5,
    0.0f,// DOOR_X=6
    1.0f, // THIN_WALL_X
    0.0f // THIN_WALL_Y
};

const float diag_start_x_offsets[NUM_CELL_TYPES] = {
    // normal walls
    0.0f,
    0.0f,// NE_TO_SW_DIAG
    0.0f,// NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    0.0f,//0.1f,// DOOR_Y=5,
    0.0f,// DOOR_X=6
    0.5f, // THIN WALL X
    0.0f, // THIN WALL Y
};
const float diag_start_y_offsets[NUM_CELL_TYPES] = {
    // normal walls
    0.0f,
    1.0f,// NE_TO_SW_DIAG
    0.0f,// NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    0.0f,// DOOR_Y=5,
    0.0f,// DOOR_X=6
    0.0f, // THIN WALL X
    0.5f, // THIN WALL Y
};


const float door_start_x_offsets[NUM_CELL_TYPES] = {
    // normal walls
    0.0f,
    0.0f,// NE_TO_SW_DIAG
    0.0f,// NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    0.0f/32.0f,//0.1f,// DOOR_Y=5,
    4.0f/32.0f,// DOOR_X=6
};
const float door_start_y_offsets[NUM_CELL_TYPES] = {
    // normal walls
    0.0f,
    1.0f,// NE_TO_SW_DIAG
    0.0f,// NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    0.0f/32.0f,// DOOR_Y=5,
    4.0f/32.0f,// DOOR_X=6
};


const float door_end_x_offsets[NUM_CELL_TYPES] = {
    // normal walls
    0.0f,
    0.0f,// NE_TO_SW_DIAG
    0.0f,// NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    4.0f/32.0f,//0.1f,// DOOR_Y=5,
    0.0f,// DOOR_X=6
};
const float door_end_y_offsets[NUM_CELL_TYPES] = {
    // normal walls
    0.0f,
    1.0f,// NE_TO_SW_DIAG
    0.0f,// NW_TO_SE_DIAG
    0.0f,// SLOPE_Y=3,
    0.0f,// SLOPE_X=4,
    0.0f,// DOOR_Y=5,
    4.0f/32.0f,// DOOR_X=6
};



#define CEIL_LIGHT_FACTOR (0.35f)
#define FLOOR_LIGHT_FACTOR (0.65f)
#define DIAG_LIGHT_FACTOR (0.87f)
#define HORIZONTAL_LIGHT_FACTOR (0.75f)
#define VERTICAL_LIGHT_FACTOR (1.0f)


#define EPSILON 1e-6f

int calc_line_hit(
    diag_intersect *result, float ray_dir_x, float ray_dir_y, float cam_dir_x, float cam_dir_y, float player_x, float player_y, 
    int map_x, int map_y, float x1, float y1, float x2, float y2, 
    float perp_dist, float u0, float u1) { 
    result->mid_flat_u = 0.0f;
    result->mid_flat_v = 0.0f;
    //result->diag_perp_dist = perp_dist;

    float diag_ix = 0.0f;
    float diag_iy = 0.0f;

    float p1x = player_x;
    float p1y = player_y;
    float q1x = player_x + ray_dir_x;
    float q1y = player_y + ray_dir_y;


    float door_dx = x2 - x1;
    float door_dy = y2 - y1;
    float door_len_sq = door_dx * door_dx + door_dy * door_dy;
    
    float a1 = q1y - p1y;
    float b1 = p1x - q1x;
    float c1 = a1 * p1x + b1 * p1y;
    float a2 = y2 - y1;//-1;
    float b2 = x1 - x2;//-1;
    float c2 = a2 * x1 + b2 * y1;
    float determinant = a1 * b2 - a2 * b1;
    diag_ix = (c1 * b2 - c2 * b1) / determinant;
    diag_iy = (a1 * c2 - a2 * c1) / determinant;
    float hit_dx = diag_ix - x1;
    float hit_dy = diag_iy - y1;

    float u = (hit_dx * door_dx + hit_dy * door_dy) / door_len_sq;
    
    float dx = diag_ix-player_x;
    float dy = diag_iy-player_y;
    
    int within_bounds = (
        (my_floorf(diag_ix + EPSILON) == map_x || my_floorf(diag_ix-EPSILON) == map_x) && 
        (my_floorf(diag_iy + EPSILON) == map_y || my_floorf(diag_iy-EPSILON) == map_y)
    );



    if (u >= -EPSILON && u <= 1.0f + EPSILON && within_bounds) {
        result->mid_flat_u = CLAMP(diag_ix - my_floorf(diag_ix), 0.0f, 1.0f);
        result->mid_flat_v = CLAMP(diag_iy - my_floorf(diag_iy), 0.0f, 1.0f);
        //if(my_floorf(diag_ix) != map_x || my_floorf(diag_iy) != map_y) {
        //    result->mid_flat_u = 1.0f;
        //    result->mid_flat_v = 1.0f;
        //}


        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
        float draw_u = lerp(u0, u1, u);
        result->diag_wall_u = draw_u;
        result->diag_perp_dist = dx*cam_dir_x + dy*cam_dir_y;

        return 1;

    }
    
    return 0;
}


int calc_diag_hit(diag_intersect *result, float ray_dir_x, float ray_dir_y, float cam_dir_x, float cam_dir_y, float player_x, float player_y, int map_x, int map_y, float perp_dist, cell_types cell_type) {
    float x1 = map_x + diag_start_x_offsets[cell_type];
    float y1 = map_y + diag_start_y_offsets[cell_type];

    // can't just use +1,+1 and +1,-1 here, for some reason.
    
    float x2 = x1 + diag_dx[cell_type]; //1.0f;
    float y2 = y1 + diag_dy[cell_type];
    result->diag_perp_dist = 1e9f;

    return calc_line_hit(result, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, player_x, player_y, map_x, map_y, x1, y1, x2, y2, perp_dist, 0.0f, 1.0f);
}

int calc_door_hit(diag_intersect *result, float ray_dir_x, float ray_dir_y, float cam_dir_x, float cam_dir_y, float player_x, float player_y, int map_x, int map_y, float perp_dist, cell_types cell_type, float door_open_amount) { 
    const float thickness = 4.0f/32.0f;
    
    float lerp_open_amount = door_open_amount*255.0f;
    lerp_open_amount = lerp_open_amount/250.0f;
    if(cell_type == DOOR_X) { lerp_open_amount = 1.0f - lerp_open_amount; }
    float x1 = lerp(map_x+0.01f, map_x+0.01f + door_end_x_offsets[cell_type], lerp_open_amount);
    float y1 = lerp(map_y+0.01f, map_y+0.01f + door_end_y_offsets[cell_type], lerp_open_amount);
    float angle = door_open_amount * (3.14159 / 2.0f);
    float dir_x = my_cosf(angle);
    float dir_y = my_sinf(angle); 
    float x2 = CLAMP(x1 + dir_x, map_x+0.01f, map_x+.99f);
    float y2 = CLAMP(y1 + dir_y, map_y+0.01f, map_y+.99f);

    // back face - offset inward along perpendicular
    float perp_x = -dir_y * thickness;//cur_thickness;
    float perp_y =  dir_x * thickness;//cur_thickness;
    if(cell_type == DOOR_X) {
        perp_x = -perp_x;
        perp_y = -perp_y;
    }
    float cap_x1 = CLAMP(x1+perp_x, map_x+0.01f, map_x+.99f);
    float cap_y1 = CLAMP(y1+perp_y, map_y+0.01f, map_y+.99f);
    float cap_x2 = CLAMP(x2+perp_x, map_x+0.01f, map_x+.99f);
    float cap_y2 = CLAMP(y2+perp_y, map_y+0.01f, map_y+.99f);
   
    diag_intersect res_main_line1, res_main_line2, end_cap_line, start_cap_line;
    res_main_line1.diag_perp_dist = 1e9f;
    res_main_line2.diag_perp_dist = 1e9f;
    start_cap_line.diag_perp_dist = 1e9f;
    end_cap_line.diag_perp_dist = 1e9f;
    int hits_line1 = calc_line_hit(
        &res_main_line1, 
        ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, player_x, player_y, 
        map_x, map_y, 
        x1, y1, 
        x2, y2, 
        perp_dist, 0.0f, 1.0f
    );


    int hits_line2 = calc_line_hit(
        &res_main_line2, 
        ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, player_x, player_y, 
        map_x, map_y, 
        cap_x1, cap_y1,
        cap_x2, cap_y2,
        perp_dist, 0.0f, 1.0f
    );
    int hits_start_cap = calc_line_hit(
        &start_cap_line, 
        ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, player_x, player_y,
        map_x, map_y,  
        x1, y1,
        cap_x1, cap_y1,
        perp_dist, 0.0f, 4.0f/32.0f
    );
    int hits_end_cap = calc_line_hit(
        &end_cap_line, 
        ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, player_x, player_y, 
        map_x, map_y, 
        x2, y2,
        cap_x2, cap_y2,
        perp_dist, 0.0f, 4.0f/32.0f
    );
    
    diag_intersect best_hit;
    best_hit.diag_perp_dist = DARK_DIST;
    int got_hit = 0;
    if (hits_line1 && res_main_line1.diag_perp_dist > NEAR_PLANE_DIST && res_main_line1.diag_perp_dist < best_hit.diag_perp_dist) {
        got_hit = 1;
        best_hit = res_main_line1;
    }

    if(hits_line2 && res_main_line2.diag_perp_dist > NEAR_PLANE_DIST && res_main_line2.diag_perp_dist < best_hit.diag_perp_dist) {
        got_hit = 1;
        best_hit = res_main_line2;
    }
    if(hits_start_cap && start_cap_line.diag_perp_dist > NEAR_PLANE_DIST && start_cap_line.diag_perp_dist < best_hit.diag_perp_dist) {
        got_hit = 1;
        best_hit = start_cap_line;
    }
    if(hits_end_cap && end_cap_line.diag_perp_dist > NEAR_PLANE_DIST && end_cap_line.diag_perp_dist < best_hit.diag_perp_dist) {
        got_hit = 1;
        best_hit = end_cap_line;
    }

    *result = best_hit;
    return got_hit;

}

void set_bit_in_bitmap(u8* bitmap, int bit_idx) {
    int byte_idx = bit_idx>>3;
    int bit = bit_idx&0b111;
    bitmap[byte_idx] |= (1 << bit);

}

#define MAX_STEPS 128
// front wall, back wall, floor, ceiling, and middle
#define MAX_SPRITE_HITS (MAX_STEPS*2)


/*
    rounded up to 24 bytes each

    currently up to 320 sprites per column
    worst case 7,680 bytes written per column

    that's if we had a sprite on every possible surface of every one of a max of 64 steps a thread took through a column :)

    we'd have other issues before then, like massive overdraw
    since, each of these entries represents a whole column of pixels

*/

/*
    POSITIONED sprites, raycasted, still in world space
*/
typedef struct {
    // screen y coordinates, used to calculate texture v coords in the innermost loop
    //float unclipped_y0, unclipped_y1;
    int prev_drawn_top, prev_drawn_bot; 
    float z0; // the depth of this column, written directly to framebuffer without testing
    float z1; // used for flats, two dists
    float u0; // the texture column, 0 <= u < 1
    float u1, v0, v1; // used for flat sprites, need two pairs of UVs
    float light_factor;
    u16 map_idx;
    u8 sprite_thg:7;
    //u8 double_sided:1;
    u8 flat_sprite:1; // if it's a floor/ceiling/middle sprite or not
    u8 sprite_idx; // which sprite?
    float bottom_height;
    float top_height;
} sprite_cache_entry;

/*
    Each thread gets MAX_STEPS entry cache for sprites
    while raycasting, whenever we take a step into a new cell,
    check if the entered side of the new cell, or the exited side of the new cell have sprites assigned.

    If any sprites exist, they are pushed into the cache in a front to back order.
    Once raycasting is complete, these sprite columns are drawn in reverse, back to front order.

    The pixels are blended with previous pixels, if necessary, or overwrite the previous pixels otherwise.
    No depth testing is performed, it is not necessary, but each pixel that writes a color also writes depth.

*/
// 1000 entries
//
sprite_cache_entry *per_thread_sprite_cache[NUM_THREADS];//[MAX_STEPS*4];

u8 *visited_cells[NUM_THREADS];//[MAP_SIZE*MAP_SIZE/8];


typedef struct {
    u32* output;
    edit_wall_id* edit_id_buffer;
    u16* z_buffer;
    int start_x;
    int end_x;
    int flash_frame; 
    level* this_level;
    float player_x; float player_y; float player_z; float player_ang; float pitch;
    int editor_mode_enabled;
    int editor_selected_map_idx;
    editor_selected_thing editor_selected_thg;
    u8 *visited_cell_bitmap;
    sprite_cache_entry* sprite_cache;
} thread_params;




void draw_first_person_level_inner(
    u32* output, edit_wall_id* edit_id_buffer, u16* z_buffer,
    int start_x, int end_x, 
    int flash_frame, 
    level* this_level, 
    float ray_origin_x, float ray_origin_y, float ray_origin_z, float cam_ang, float pitch,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side,
    u8* visited_cell_bitmap, sprite_cache_entry* sprite_cache
) {
    ray_origin_x += 1e-6f;
    cam_ang += 1e-4f;

    //u8* cur_level = this_level;
    u8* cur_level_floor = this_level->floor;
    u8* cur_level_ceil = this_level->ceil;
    u8* cur_level_upper_floor = this_level->upper_floor;
    u8* cur_level_upper_ceil = this_level->upper_ceil;

    float start_cam_dir_x = my_cosf(cam_ang);
    float start_cam_dir_y = my_sinf(cam_ang);
    for(int ix = start_x; ix < end_x; ix++) {

        float cam_dir_x = start_cam_dir_x;
        float cam_dir_y = start_cam_dir_y;
        int screen_x = (FP_SCREEN_WIDTH-1)-ix;
        float cam_x = 2.0f * (ix) / (float)FP_SCREEN_WIDTH - 1.0f; // -1 to 1

        float ray_dir_x = my_cosf(cam_ang) + cam_x * -my_sinf(cam_ang);
        float ray_dir_y = my_sinf(cam_ang) + cam_x * my_cosf(cam_ang);



        int num_sprites_hit = 0;
        
        int rem_steps = MAX_STEPS;

        float perp_dist = NEAR_PLANE_DIST;

        int prev_drawn_top = 0;
        int prev_drawn_bot = FP_SCREEN_HEIGHT;
        int start_map_x = my_floorf(ray_origin_x);
        int start_map_y = my_floorf(ray_origin_y);
        
        // length of ray from one x/y side to the next x/y side
        float delta_dist_x = my_fabsf(1.0f / ray_dir_x);
        float delta_dist_y = my_fabsf(1.0f / ray_dir_y);

        int map_x = my_floorf(ray_origin_x);
        int map_y = my_floorf(ray_origin_y);
        

        // these define where we exit a flat if we don't hit the corresponding side
        // e.g. if we leave via a VERTICAL SIDE, then the U coord will be either 0 or 1
        // if we leave via a HORIZONTAL side, the U coord will be either 0 or 1
        float def_exit_u = (ray_dir_x >= 0) ? 1.0f : 0.0f;
        float def_exit_v = (ray_dir_y >= 0) ? 1.0f : 0.0f;

        float def_start_u = 1.0f - def_exit_u;//(ray_dir_x >= 0) ? 0.0f : 1.0f;
        float def_start_v = 1.0f - def_exit_v; //(ray_dir_y >= 0) ? 0.0f : 1.0f;
        
        float flat_u = ray_origin_x - my_floorf(ray_origin_x);
        float flat_v = ray_origin_y - my_floorf(ray_origin_y);           // the u,v position of where we enter the next cell (which we use on the next iteration)

        int step_x = (ray_dir_x < 0) ? -1 : 1;
        float side_dist_x = (ray_dir_x < 0) ? ((ray_origin_x - map_x) * delta_dist_x) : ((map_x + 1.0f - ray_origin_x) * delta_dist_x);

        int step_y = (ray_dir_y < 0) ? -1 : 1;
        float side_dist_y = (ray_dir_y < 0) ? ((ray_origin_y - map_y) * delta_dist_y) : ((map_y + 1.0 - ray_origin_y) * delta_dist_y);


        
        int next_map_x = map_x;
        int next_map_y = map_y;
        float next_perp_dist = perp_dist;
        

        int next_side;
        wall_side side = HORIZONTAL_SIDE;
        float light_factor = 0.75f;
        float next_light_factor;
        
        u32* skybox_column = NULL;
        {
            float ray_ang = my_atan2f(ray_dir_y, ray_dir_x);
            if(ray_ang < 0.0f) {
                ray_ang += 6.28f;
            }

            u32* skybox = textures[SKYBOX_TEX_IDX];

            float u = ((ray_ang) / (6.28f));
            float flt_u = (1024.0f*u+skybox_u_offset);
            int int_u = ((int)flt_u)&(SKYBOX_TEX_WIDTH-1);
            skybox_column = &skybox[int_u*SKYBOX_TEX_HEIGHT];
        }
            

        for(int step = 0; 
            (step < rem_steps) && 
            (prev_drawn_top < prev_drawn_bot);
            step++,
            map_x = next_map_x,
            map_y = next_map_y,
            perp_dist = next_perp_dist,
            side = next_side,
            light_factor = next_light_factor
        ) {
            float wall_u;
            float exit_flat_u, exit_flat_v; // the u,v position of where we "exit" the current cell before stepping to the new one
            float hit_x;
            float hit_y;

            if(side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                next_map_x = map_x + step_x;
                next_side = VERTICAL_SIDE;
                next_light_factor = 1.0f;
                next_perp_dist = ((next_map_x - ray_origin_x + (1 - step_x) * 0.5f) / ray_dir_x);
            } else {
                side_dist_y += delta_dist_y;
                next_map_y = map_y + step_y;
                next_side = HORIZONTAL_SIDE;
                next_light_factor = .75f;
                next_perp_dist = ((next_map_y - ray_origin_y + (1 - step_y) * 0.5f) / ray_dir_y);
            }
            if(map_x >= MAP_SIZE || map_x < 0 || map_y >= MAP_SIZE || map_y < 0) {
                break;
            }
            next_perp_dist = MAX(next_perp_dist, perp_dist);

            int map_idx = map_y * MAP_SIZE + map_x;
            int in_start_cell = (map_x == start_map_x && map_y == start_map_y);
            

            set_bit_in_bitmap(visited_cell_bitmap, map_idx);
            int selected_cur_map_idx = editor_selected_map_idx == map_idx;
            cell_types upper_cell_type = this_level->upper_cell_types[map_idx];
            cell_types lower_cell_type = this_level->lower_cell_types[map_idx];

            int floor_anchor = this_level->floor_anchor[map_idx];
            int ceil_anchor = this_level->ceil_anchor[map_idx];
            int floor_anchor_is_not_zero = floor_anchor > 0;
            int ceil_anchor_is_not_max = ceil_anchor < MAX_WALL_HEIGHT;

            int proj_floor_anchor_height = project_to_screen(floor_anchor, perp_dist, pitch, ray_origin_z);
            int proj_zero_height = project_to_screen(0, perp_dist, pitch, ray_origin_z);
            int proj_ceil_anchor_height = project_to_screen(ceil_anchor, perp_dist, pitch, ray_origin_z);
            int proj_max_height = project_to_screen(MAX_WALL_HEIGHT, perp_dist, pitch, ray_origin_z);

            hit_x = ray_origin_x + perp_dist * ray_dir_x;
            hit_y = ray_origin_y + perp_dist * ray_dir_y;
            float next_hit_x = ray_origin_x + next_perp_dist * ray_dir_x;
            float next_hit_y = ray_origin_y + next_perp_dist * ray_dir_y;

            

            // lower floor height and higher ceil height are used for diagonals and other special cell types
            int floor_height = cur_level_floor[map_idx];
            int upper_floor_height = cur_level_upper_floor[map_idx];
            int ceil_height = cur_level_ceil[map_idx];
            int upper_ceil_height = cur_level_upper_ceil[map_idx];




            u8 upper_wall_tex, lower_wall_tex;
            editor_selected_thing upper_intersect_wall_side, lower_intersect_wall_side;
            u8 lower_intersect_wall_light_level, upper_intersect_wall_light_level;
            if(side == VERTICAL_SIDE) {
                wall_u = hit_y - my_floorf(hit_y);
                flat_u = in_start_cell ? (ray_origin_x - my_floorf(ray_origin_x)) : def_start_u;
                flat_v = in_start_cell ? (ray_origin_y - my_floorf(ray_origin_y)) : wall_u;

                if(ray_dir_x > 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_WEST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_WEST;
                    wall_u = 1.0f - wall_u;
                    upper_wall_tex = this_level->uwtex[map_idx];
                    lower_wall_tex = this_level->lwtex[map_idx];
                    upper_intersect_wall_light_level = this_level->uw_light[map_idx];
                    lower_intersect_wall_light_level = this_level->lw_light[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_EAST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_EAST;
                    upper_wall_tex = this_level->uetex[map_idx];
                    lower_wall_tex = this_level->letex[map_idx];
                    upper_intersect_wall_light_level = this_level->ue_light[map_idx];
                    lower_intersect_wall_light_level = this_level->le_light[map_idx];
                }
            } else {
                wall_u = hit_x - my_floorf(hit_x);
                flat_u = in_start_cell ? (ray_origin_x - my_floorf(ray_origin_x)) : wall_u;
                flat_v = in_start_cell ? (ray_origin_y - my_floorf(ray_origin_y)) : def_start_v;
                if(ray_dir_y < 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_SOUTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_SOUTH;
                    wall_u = 1.0f - wall_u;
                    upper_wall_tex = this_level->ustex[map_idx];
                    lower_wall_tex = this_level->lstex[map_idx];
                    upper_intersect_wall_light_level = this_level->us_light[map_idx];
                    lower_intersect_wall_light_level = this_level->ls_light[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_NORTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_NORTH;
                    upper_wall_tex = this_level->untex[map_idx];
                    lower_wall_tex = this_level->lntex[map_idx];
                    upper_intersect_wall_light_level = this_level->un_light[map_idx];
                    lower_intersect_wall_light_level = this_level->ln_light[map_idx];
                }
            }
            if(next_side == VERTICAL_SIDE) {
                exit_flat_u = def_exit_u;
                exit_flat_v = next_hit_y - my_floorf(next_hit_y);

            } else {
                exit_flat_u = next_hit_x - my_floorf(next_hit_x);
                exit_flat_v = def_exit_v;
            }

            int hit_enter_sprite = EMPTY_SPRITE_INDEX;
            int hit_exit_sprite = EMPTY_SPRITE_INDEX;
            int hit_middle_sprite = this_level->m_sprite_index[map_idx];
            int hit_floor_sprite = this_level->f_sprite_index[map_idx];
            int hit_ceiling_sprite = this_level->c_sprite_index[map_idx];
            float enter_sprite_wall_u = wall_u;
            float exit_sprite_wall_u = 0.0f;
            editor_selected_thing enter_sprite_thg, exit_sprite_thg;
            
            if(side == HORIZONTAL_SIDE) {
                if(ray_dir_y >= 0) {
                    hit_enter_sprite = this_level->n_sprite_index[map_idx];
                    enter_sprite_thg = N_SPRITE;
                } else {
                    hit_enter_sprite = this_level->s_sprite_index[map_idx];
                    enter_sprite_thg = S_SPRITE;
                }
            } else {
                if(ray_dir_x >= 0) {
                    hit_enter_sprite = this_level->w_sprite_index[map_idx];
                    enter_sprite_thg = W_SPRITE;
                } else {
                    hit_enter_sprite = this_level->e_sprite_index[map_idx];
                    enter_sprite_thg = E_SPRITE;
                }
            }
            
            if(next_side == HORIZONTAL_SIDE) {
                exit_sprite_wall_u = (next_hit_x - my_floorf(next_hit_x));   
                if(ray_dir_y >= 0) {
                    hit_exit_sprite = this_level->s_sprite_index[map_idx];
                    exit_sprite_thg = S_SPRITE;
                } else {
                    hit_exit_sprite = this_level->n_sprite_index[map_idx];
                    exit_sprite_thg = N_SPRITE;
                }
            } else {          
                exit_sprite_wall_u = (next_hit_y - my_floorf(next_hit_y));   
                if(ray_dir_x >= 0) {
                    hit_exit_sprite = this_level->e_sprite_index[map_idx];
                    exit_sprite_thg = E_SPRITE;
                } else {
                    hit_exit_sprite = this_level->w_sprite_index[map_idx];
                    exit_sprite_thg = W_SPRITE;
                }
            }
            int can_add_sprite = (num_sprites_hit < MAX_SPRITE_HITS);

            if(can_add_sprite && hit_enter_sprite != EMPTY_SPRITE_INDEX && !in_start_cell && num_sprites_hit < MAX_SPRITE_HITS) {
                float sprite_bot_y = floor_height;
                if((lower_cell_type == THIN_WALL_X && enter_sprite_thg == E_SPRITE) ||
                   (lower_cell_type == THIN_WALL_Y && enter_sprite_thg == N_SPRITE) || 
                   (lower_cell_type == NW_TO_SE_DIAG && (enter_sprite_thg == E_SPRITE || enter_sprite_thg == N_SPRITE)) || 
                   (lower_cell_type == NE_TO_SW_DIAG && (enter_sprite_thg == W_SPRITE || enter_sprite_thg == W_SPRITE))) {
                    sprite_bot_y = upper_floor_height;
                }

                sprite_cache[num_sprites_hit].bottom_height = sprite_bot_y;
                sprite_cache[num_sprites_hit].top_height = sprite_bot_y+8.0f;
                sprite_cache[num_sprites_hit].prev_drawn_top = prev_drawn_top;
                sprite_cache[num_sprites_hit].prev_drawn_bot = prev_drawn_bot;
                sprite_cache[num_sprites_hit].sprite_idx = hit_enter_sprite;
                sprite_cache[num_sprites_hit].u0 = enter_sprite_wall_u;
                sprite_cache[num_sprites_hit].v0 = 0.0f;
                sprite_cache[num_sprites_hit].v1 = 1.0f;
                sprite_cache[num_sprites_hit].map_idx = map_idx;
                sprite_cache[num_sprites_hit].sprite_thg = enter_sprite_thg;
                sprite_cache[num_sprites_hit].flat_sprite = 0;
                sprite_cache[num_sprites_hit].light_factor = 1.0f;
                sprite_cache[num_sprites_hit++].z0 = perp_dist;
            }
            if(can_add_sprite && hit_floor_sprite != EMPTY_SPRITE_INDEX) {
                float sprite_bot_y = floor_height;
                sprite_cache[num_sprites_hit].bottom_height = sprite_bot_y;
                sprite_cache[num_sprites_hit].prev_drawn_top = prev_drawn_top;
                sprite_cache[num_sprites_hit].prev_drawn_bot = prev_drawn_bot;
                sprite_cache[num_sprites_hit].sprite_idx = hit_floor_sprite;
                sprite_cache[num_sprites_hit].u0 = exit_flat_u;
                sprite_cache[num_sprites_hit].v0 = exit_flat_v;
                sprite_cache[num_sprites_hit].u1 = flat_u;
                sprite_cache[num_sprites_hit].v1 = flat_v;
                sprite_cache[num_sprites_hit].map_idx = map_idx;
                sprite_cache[num_sprites_hit].sprite_thg = FLOOR_SPRITE;
                sprite_cache[num_sprites_hit].flat_sprite = 1;
                sprite_cache[num_sprites_hit].light_factor = FLOOR_LIGHT_FACTOR;
                sprite_cache[num_sprites_hit].z0 = next_perp_dist;
                sprite_cache[num_sprites_hit++].z1 = perp_dist;

            }
            if(can_add_sprite && hit_ceiling_sprite != EMPTY_SPRITE_INDEX) {
                float sprite_bot_y = ceil_height;
                sprite_cache[num_sprites_hit].bottom_height = sprite_bot_y;
                sprite_cache[num_sprites_hit].prev_drawn_top = prev_drawn_top;
                sprite_cache[num_sprites_hit].prev_drawn_bot = prev_drawn_bot;
                sprite_cache[num_sprites_hit].sprite_idx = hit_ceiling_sprite;
                sprite_cache[num_sprites_hit].u0 = flat_u;
                sprite_cache[num_sprites_hit].v0 = flat_v;
                sprite_cache[num_sprites_hit].u1 = exit_flat_u;
                sprite_cache[num_sprites_hit].v1 = exit_flat_v;
                sprite_cache[num_sprites_hit].map_idx = map_idx;
                sprite_cache[num_sprites_hit].sprite_thg = CEIL_SPRITE;
                sprite_cache[num_sprites_hit].flat_sprite = 1;
                sprite_cache[num_sprites_hit].light_factor = CEIL_LIGHT_FACTOR;
                sprite_cache[num_sprites_hit].z0 = perp_dist;
                sprite_cache[num_sprites_hit++].z1 = next_perp_dist;
            }
            if(can_add_sprite && hit_middle_sprite != EMPTY_SPRITE_INDEX) {
                float sprite_top_y = floor_height + this_level->m_sprite_offset[map_idx];
            #define DRAW_FRONT_OF_MIDDLE_SPRITES
            #ifndef DRAW_FRONT_OF_MIDDLE_SPRITES 
                float sprite_bot_y = sprite_top_y;
            #else
                float sprite_bot_y = floor_height + this_level->m_sprite_offset[map_idx]-1.0f;
                int screen_y_near = project_to_screen(sprite_bot_y, perp_dist, pitch, ray_origin_z);
                // draw front "wall" of middle sprite
                sprite_cache[num_sprites_hit].bottom_height = sprite_bot_y;
                sprite_cache[num_sprites_hit].top_height = sprite_top_y;
                //sprite_cache[num_sprites_hit].top_height = sprite_top_y+32.0f;
                sprite_cache[num_sprites_hit].prev_drawn_top = prev_drawn_top;
                sprite_cache[num_sprites_hit].prev_drawn_bot = prev_drawn_bot;
                sprite_cache[num_sprites_hit].sprite_idx = hit_middle_sprite;
                sprite_cache[num_sprites_hit].u0 = wall_u;
                sprite_cache[num_sprites_hit].v0 = 0.0f;
                sprite_cache[num_sprites_hit].v1 = 4.0f/32.0f;

                sprite_cache[num_sprites_hit].map_idx = map_idx;
                sprite_cache[num_sprites_hit].sprite_thg = MIDDLE_SPRITE;
                sprite_cache[num_sprites_hit].flat_sprite = 0;
                sprite_cache[num_sprites_hit].light_factor = FLOOR_LIGHT_FACTOR;
                sprite_cache[num_sprites_hit++].z0 = perp_dist;
            #endif

                
                // upper half
                sprite_cache[num_sprites_hit].bottom_height = sprite_bot_y;
                sprite_cache[num_sprites_hit].prev_drawn_top = prev_drawn_top;
                sprite_cache[num_sprites_hit].prev_drawn_bot = prev_drawn_bot;
                sprite_cache[num_sprites_hit].sprite_idx = hit_middle_sprite;
                sprite_cache[num_sprites_hit].u0 = flat_u;
                sprite_cache[num_sprites_hit].v0 = flat_v;
                sprite_cache[num_sprites_hit].u1 = exit_flat_u;
                sprite_cache[num_sprites_hit].v1 = exit_flat_v;
                sprite_cache[num_sprites_hit].map_idx = map_idx;
                sprite_cache[num_sprites_hit].sprite_thg = MIDDLE_SPRITE;
                sprite_cache[num_sprites_hit].flat_sprite = 1;
                sprite_cache[num_sprites_hit].light_factor = CEIL_LIGHT_FACTOR;
                sprite_cache[num_sprites_hit].z0 = perp_dist;
                sprite_cache[num_sprites_hit++].z1 = next_perp_dist;
                if(sprite_bot_y < ray_origin_z) {
                    // if bottom half, adjust some parameters
                    // bottom half
                    sprite_cache[num_sprites_hit-1].bottom_height = sprite_top_y;
                    sprite_cache[num_sprites_hit-1].u0 = exit_flat_u;
                    sprite_cache[num_sprites_hit-1].v0 = exit_flat_v;
                    sprite_cache[num_sprites_hit-1].u1 = flat_u;
                    sprite_cache[num_sprites_hit-1].v1 = flat_v;
                    sprite_cache[num_sprites_hit-1].light_factor = FLOOR_LIGHT_FACTOR;
                    sprite_cache[num_sprites_hit-1].z0 = next_perp_dist;
                    sprite_cache[num_sprites_hit-1].z1 = perp_dist;
                }
            }

            if(can_add_sprite && hit_exit_sprite != EMPTY_SPRITE_INDEX && num_sprites_hit < MAX_SPRITE_HITS) {
                float sprite_bot_y = floor_height;
                if((lower_cell_type == THIN_WALL_X && exit_sprite_thg == E_SPRITE) || 
                   (lower_cell_type == THIN_WALL_Y && exit_sprite_thg == N_SPRITE) || 
                   (lower_cell_type == NW_TO_SE_DIAG && (exit_sprite_thg == E_SPRITE || exit_sprite_thg == N_SPRITE)) || 
                   (lower_cell_type == NE_TO_SW_DIAG && (exit_sprite_thg == W_SPRITE || exit_sprite_thg == W_SPRITE))) {
                    sprite_bot_y = upper_floor_height;
                }

                sprite_cache[num_sprites_hit].bottom_height = sprite_bot_y;
                sprite_cache[num_sprites_hit].top_height = sprite_bot_y+8.0f;
                //sprite_cache[num_sprites_hit].prev_drawn_top = prev_drawn_top;
                //sprite_cache[num_sprites_hit].prev_drawn_bot = prev_drawn_bot;
                sprite_cache[num_sprites_hit].sprite_idx = hit_exit_sprite;
                sprite_cache[num_sprites_hit].u0 = exit_sprite_wall_u;
                sprite_cache[num_sprites_hit].v0 = 0.0f;
                sprite_cache[num_sprites_hit].v1 = 1.0f;
                sprite_cache[num_sprites_hit].map_idx = map_idx;
                sprite_cache[num_sprites_hit].sprite_thg = exit_sprite_thg;
                sprite_cache[num_sprites_hit].flat_sprite = 0;
                sprite_cache[num_sprites_hit].light_factor = 1.0f;
                sprite_cache[num_sprites_hit++].z0 = next_perp_dist;
            }

            u8 lower_diag_wall_tex = this_level->ldtex[map_idx];
            u8 lower_diag_light_level = this_level->ld_light[map_idx];

            u8 floor_texture = this_level->ftex[map_idx];
            u8 floor_light_level = this_level->f_light[map_idx];

            u8 upper_floor_texture = this_level->uftex[map_idx];
            u8 upper_floor_light_level = this_level->uf_light[map_idx];

            u8 upper_diag_wall_tex = this_level->udtex[map_idx];
            u8 upper_diag_light_level = this_level->ud_light[map_idx];

            u8 ceil_texture = this_level->ctex[map_idx];
            u8 ceil_light_level = this_level->c_light[map_idx];

            u8 upper_ceil_texture = this_level->uctex[map_idx];
            u8 upper_ceil_light_level = this_level->uc_light[map_idx];

            {
                int first_floor_height = floor_height;
                int second_floor_height = upper_floor_height;
                u8 first_floor_texture = floor_texture;
                u8 second_floor_texture = upper_floor_texture;
                u8 first_floor_light_level = floor_light_level;
                u8 second_floor_light_level = upper_floor_light_level;
                editor_selected_thing first_floor_side = WALL_SIDE_TOP;
                editor_selected_thing second_floor_side = WALL_SIDE_UPPER_TOP;

                int first_ceil_height = ceil_height;
                int second_ceil_height = upper_ceil_height;
                u8 first_ceil_texture = ceil_texture;
                u8 second_ceil_texture = upper_ceil_texture;
                u8 first_ceil_light_level = ceil_light_level;
                u8 second_ceil_light_level = upper_ceil_light_level;
                editor_selected_thing first_ceil_side = WALL_SIDE_BOTTOM;
                editor_selected_thing second_ceil_side = WALL_SIDE_UPPER_BOTTOM;

                // simple, not perfect check, if you're inside a wall in editor mode, don't draw this cell and skip to the next
                // can miss diagonals or slopes
                if(editor_mode_enabled && in_start_cell) {
                    if(first_ceil_height < player_z || first_floor_height > player_z) {
                        continue;
                    }
                }
                

                // miscellaneous stuff for diagonal draw order sorting 
                float subx = ray_origin_x - my_floorf(ray_origin_x);
                float suby = ray_origin_y - my_floorf(ray_origin_y);
                int in_top = (suby < 0.5f);
                int in_right = (subx >= 0.5f);

                int in_top_right = (subx >= suby);
                //int in_bottom_left = !in_top_right;
                int in_top_left = (subx < (1.0f - suby));
                //int in_bottom_right = !in_top_left;

                int enters_right_side = (step_x == -1) && (side == VERTICAL_SIDE);
                int enters_left_side = (step_x == 1) && (side == VERTICAL_SIDE);
                int enters_bot_side = (step_y == -1) && (side == HORIZONTAL_SIDE);
                int enters_top_side = (step_y == 1) && (side == HORIZONTAL_SIDE);

                int enters_top_side_right_half = (enters_top_side && ((hit_x - my_floorf(hit_x)) >= 0.5f));
                int enters_bot_side_right_half = (enters_bot_side && ((hit_x - my_floorf(hit_x)) >= 0.5f));
                int enters_left_side_top_half = (enters_left_side && ((hit_y - my_floorf(hit_y)) <= 0.5f));
                int enters_right_side_top_half = (enters_right_side && ((hit_y - my_floorf(hit_y)) <= 0.5f));

                if(lower_cell_type == NE_TO_SW_DIAG || lower_cell_type ==  NW_TO_SE_DIAG || 
                   lower_cell_type == THIN_WALL_X || lower_cell_type == THIN_WALL_Y) { //} || lower_cell_type == DOOR_Y) {
                    int draw_upper_first = 0;
                    if(lower_cell_type == NE_TO_SW_DIAG) {
                        draw_upper_first = (in_start_cell ? in_top_left : (enters_left_side || enters_top_side));
                    } else if (lower_cell_type == NW_TO_SE_DIAG) {
                        draw_upper_first = (in_start_cell ? in_top_right : (enters_right_side || enters_top_side));
                    } else if (lower_cell_type == THIN_WALL_X) {
                        draw_upper_first = (in_start_cell ? in_right : (enters_right_side || enters_top_side_right_half || enters_bot_side_right_half));
                    } else if (lower_cell_type == THIN_WALL_Y) {
                        draw_upper_first = (in_start_cell ? in_top : (enters_top_side || enters_left_side_top_half || enters_right_side_top_half));
                    }

                    if(draw_upper_first) {
                        first_floor_height = upper_floor_height;
                        second_floor_height = floor_height;
                        first_floor_texture = upper_floor_texture;
                        second_floor_texture = floor_texture;
                        first_floor_light_level = upper_floor_light_level;
                        second_floor_light_level = floor_light_level;
                        first_floor_side = WALL_SIDE_UPPER_TOP;
                        second_floor_side = WALL_SIDE_TOP;
                    }
                }

                if(upper_cell_type == NE_TO_SW_DIAG || upper_cell_type ==  NW_TO_SE_DIAG || 
                   upper_cell_type == THIN_WALL_X || upper_cell_type == THIN_WALL_Y) {
                    // handle diagonal stuff
                    int draw_upper_first = 0;
                    if(upper_cell_type == NE_TO_SW_DIAG) {
                        draw_upper_first = (in_start_cell ? in_top_left : (enters_left_side || enters_top_side));
                    } else if(upper_cell_type == NW_TO_SE_DIAG) { // NW_TO_SE_DIAG
                        draw_upper_first = (in_start_cell ? in_top_right : (enters_right_side || enters_top_side));
                    } else if (upper_cell_type == THIN_WALL_X) {
                        draw_upper_first = (in_start_cell ? in_right : (enters_right_side || enters_top_side_right_half || enters_bot_side_right_half));
                    } else if (upper_cell_type == THIN_WALL_Y) {
                        draw_upper_first = (in_start_cell ? in_top : (enters_top_side || enters_left_side_top_half || enters_right_side_top_half));
                    }


                    if(draw_upper_first) {
                        first_ceil_height = upper_ceil_height;
                        second_ceil_height = ceil_height;
                        first_ceil_texture = upper_ceil_texture;
                        second_ceil_texture = ceil_texture;
                        first_ceil_light_level = upper_ceil_light_level;
                        second_ceil_light_level = ceil_light_level;
                        first_ceil_side = WALL_SIDE_UPPER_BOTTOM;
                        second_ceil_side = WALL_SIDE_BOTTOM;
                    }
                }
                int lower_step_slope = (lower_cell_type == SLOPE_X) || (lower_cell_type == SLOPE_Y);
                int upper_step_slope = (upper_cell_type == SLOPE_X) || (upper_cell_type == SLOPE_Y);


                // draw first steps, this happens regardless of whether we're drawing a diagonal cell or not
                // (not done if in initial map cell, ie where the player is)
                // draw first floor step
                int proj_floor_first_step_height = project_to_screen(first_floor_height, perp_dist, pitch, ray_origin_z);
                int proj_floor_first_step_height_at_next_dist = project_to_screen(first_floor_height, next_perp_dist, pitch, ray_origin_z);
                int proj_ceil_first_step_height = project_to_screen(first_ceil_height, perp_dist, pitch, ray_origin_z);

                if(!in_start_cell && !lower_step_slope && proj_floor_first_step_height < prev_drawn_bot) {
                    if(floor_anchor_is_not_zero) {
                        draw_skybox_vline(
                            output, skybox_column, screen_x, MAX(prev_drawn_top, proj_floor_anchor_height), prev_drawn_bot
                        );
                    }
                    draw_lit_fogged_clipped_textured_wall(
                        output, z_buffer,
                        (lower_wall_tex == SKYBOX_TEX_IDX),
                        get_texture_column(textures[lower_wall_tex], wall_u),skybox_column,
                        screen_x, proj_floor_first_step_height, proj_floor_anchor_height,
                        first_floor_height, floor_anchor, BOTTOM_PEGGED,
                        prev_drawn_top, prev_drawn_bot, perp_dist, light_factor, lower_intersect_wall_light_level, 
                        FOG_COL, REPEAT_TEX
                    );

                    draw_edit_vline(
                        edit_id_buffer, screen_x,
                        proj_floor_first_step_height, proj_zero_height, 
                        prev_drawn_top, prev_drawn_bot,
                        MAP_CELL_EDIT_ID(map_idx, lower_intersect_wall_side)
                    );
                    if(editor_mode_enabled) {
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == lower_intersect_wall_side) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_floor_first_step_height, proj_zero_height, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }

                    prev_drawn_bot = proj_floor_first_step_height;
                } else if (!in_start_cell && lower_step_slope) {
                    //float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-my_floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - my_floorf(ray_origin_y)) : hit_y - my_floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    //float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-my_floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - my_floorf(ray_origin_x)) : hit_x - my_floorf(hit_x);
                    if(!in_start_cell && side == VERTICAL_SIDE) {
                        if(step_x == -1) {
                            x_start = 1.0f;
                        } else {
                            x_start = 0.0f;
                        }
                    }
                    float start = (lower_cell_type == SLOPE_Y) ? y_start : x_start;
                    //float exit = (lower_cell_type == SLOPE_Y) ? y_exit : x_exit;
                    float slope_start_height = (float)first_floor_height + start*(float)((float)second_floor_height - (float)first_floor_height);
                    //float slope_end_height = (float)first_floor_height + exit*(float)((float)second_floor_height - (float)first_floor_height);

                    int proj_slope_start_height = project_to_screen(slope_start_height, perp_dist, pitch, ray_origin_z);

                    
                    if(!in_start_cell && proj_slope_start_height < prev_drawn_bot) {      
                        if(floor_anchor_is_not_zero) {
                            draw_skybox_vline(
                                output, skybox_column, screen_x, MAX(prev_drawn_top, proj_floor_anchor_height), prev_drawn_bot
                            );
                        }
                        // draw wall up to start of slope    
                        draw_lit_fogged_clipped_textured_wall(
                            output, z_buffer,
                            ((lower_wall_tex) == SKYBOX_TEX_IDX),
                            get_texture_column(textures[lower_wall_tex], wall_u),skybox_column,
                            screen_x, proj_slope_start_height, proj_floor_anchor_height,
                            slope_start_height, floor_anchor, BOTTOM_PEGGED,
                            prev_drawn_top, prev_drawn_bot, perp_dist, light_factor, lower_intersect_wall_light_level, 
                            FOG_COL, REPEAT_TEX
                        );
                        
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_slope_start_height, proj_zero_height, 
                            prev_drawn_top, prev_drawn_bot,
                            MAP_CELL_EDIT_ID(map_idx, lower_intersect_wall_side)
                        );
                        if(editor_mode_enabled) {
                            if(flash_frame && selected_cur_map_idx && editor_selected_side == lower_intersect_wall_side) {
                                draw_tint_vline(
                                    output, screen_x, 
                                    proj_slope_start_height, proj_zero_height, 
                                    prev_drawn_top, prev_drawn_bot
                                );
                            }
                        }
                        prev_drawn_bot = proj_slope_start_height;
                    }
                    
                }

                // draw first ceil step
                if(!in_start_cell && !upper_step_slope && proj_ceil_first_step_height > prev_drawn_top) {
                    if(ceil_anchor_is_not_max) {
                        draw_skybox_vline(
                            output, skybox_column, screen_x, prev_drawn_top, MIN(prev_drawn_bot, proj_ceil_anchor_height)
                        );
                    }
                    draw_lit_fogged_clipped_textured_wall(
                        output, z_buffer,
                        ((upper_wall_tex) == SKYBOX_TEX_IDX),
                        get_texture_column(textures[upper_wall_tex], wall_u),skybox_column,
                        screen_x, proj_ceil_anchor_height, proj_ceil_first_step_height,
                        ceil_anchor, first_ceil_height, TOP_PEGGED,
                        prev_drawn_top, prev_drawn_bot, perp_dist, light_factor, upper_intersect_wall_light_level, 
                        FOG_COL, REPEAT_TEX
                    );

                    
                    draw_edit_vline(
                        edit_id_buffer, screen_x,
                        proj_max_height, proj_ceil_first_step_height, 
                        prev_drawn_top, prev_drawn_bot,
                        MAP_CELL_EDIT_ID(map_idx, upper_intersect_wall_side)
                    );
                    if(editor_mode_enabled) {
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == upper_intersect_wall_side) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_max_height, proj_ceil_first_step_height, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_top = proj_ceil_first_step_height;
                } else if (!in_start_cell && upper_step_slope) {
                    //float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-my_floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - my_floorf(ray_origin_y)) : hit_y - my_floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    //float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-my_floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - my_floorf(ray_origin_x)) : hit_x - my_floorf(hit_x);
                    if(!in_start_cell && side == VERTICAL_SIDE) {
                        if(step_x == -1) {
                            x_start = 1.0f;
                        } else {
                            x_start = 0.0f;
                        }
                    }
                    //float exit = (upper_cell_type == SLOPE_X) ? x_exit : y_exit;
                    float start = (upper_cell_type == SLOPE_X) ? x_start : y_start;

                    float slope_start_height = (float)first_ceil_height + start*(float)((float)second_ceil_height - (float)first_ceil_height);
                    //float slope_end_height = (float)first_ceil_height + exit*(float)((float)second_ceil_height - (float)first_ceil_height);

                    int proj_slope_start_height = project_to_screen(slope_start_height, perp_dist, pitch, ray_origin_z);


                    if(proj_slope_start_height > prev_drawn_top) {  
                        if(ceil_anchor_is_not_max) {
                            if(ceil_anchor_is_not_max) {
                                draw_skybox_vline(
                                    output, skybox_column, screen_x, prev_drawn_top, MIN(prev_drawn_bot, proj_ceil_anchor_height)
                                );
                            }
                        }
                        
                        // draw wall up to start of slope    
                        draw_lit_fogged_clipped_textured_wall(
                            output, z_buffer,
                            ((upper_wall_tex) == SKYBOX_TEX_IDX),
                            get_texture_column(textures[upper_wall_tex], wall_u), skybox_column,
                            screen_x, 
                            proj_ceil_anchor_height, proj_slope_start_height,
                            ceil_anchor, slope_start_height, TOP_PEGGED,
                            prev_drawn_top, prev_drawn_bot, perp_dist, light_factor, upper_intersect_wall_light_level,
                            FOG_COL, REPEAT_TEX
                        );
                        
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_max_height, proj_slope_start_height, 
                            prev_drawn_top, prev_drawn_bot,
                            MAP_CELL_EDIT_ID(map_idx, upper_intersect_wall_side)
                        );
                        if(editor_mode_enabled) {
                            if(flash_frame && selected_cur_map_idx && editor_selected_side == upper_intersect_wall_side) {
                                draw_tint_vline(
                                    output, screen_x, 
                                    proj_max_height, proj_slope_start_height, 
                                    prev_drawn_top, prev_drawn_bot
                                );
                            }
                        }
                        prev_drawn_top = proj_slope_start_height;
                    }

                }
                if(lower_cell_type == SLOPE_Y || lower_cell_type == SLOPE_X) {
                    float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-my_floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - my_floorf(ray_origin_y)) : hit_y - my_floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-my_floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - my_floorf(ray_origin_x)) : hit_x - my_floorf(hit_x);
                    if(!in_start_cell && side == VERTICAL_SIDE) {
                        if(step_x == -1) {
                            x_start = 1.0f;
                        } else {
                            x_start = 0.0f;
                        }
                    }
                    float start = (lower_cell_type == SLOPE_Y) ? y_start : x_start;
                    float exit = (lower_cell_type == SLOPE_Y) ? y_exit : x_exit;
                    float slope_start_height = (float)first_floor_height + start*(float)((float)second_floor_height - (float)first_floor_height);
                    float slope_end_height = (float)first_floor_height + exit*(float)((float)second_floor_height - (float)first_floor_height);

                    int proj_slope_start_height = project_to_screen(slope_start_height, perp_dist, pitch, ray_origin_z);
                    int proj_slope_end_height = project_to_screen(slope_end_height, next_perp_dist, pitch, ray_origin_z);

                    
                    // draw wall for slope

                    if (proj_slope_end_height < prev_drawn_bot) {
                        draw_lit_fogged_tex_flat(
                            output, z_buffer, textures[upper_floor_texture],skybox_column,
                            screen_x, proj_slope_end_height, proj_slope_start_height, 
                            next_perp_dist, perp_dist, 
                            exit_flat_u, exit_flat_v, flat_u, flat_v,
                            prev_drawn_top, prev_drawn_bot, FLOOR_LIGHT_FACTOR, upper_floor_light_level, FOG_COL
                        );
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_slope_end_height, proj_slope_start_height, 
                            prev_drawn_top, prev_drawn_bot,
                            MAP_CELL_EDIT_ID(map_idx, WALL_SIDE_UPPER_TOP)
                        );
                        if(editor_mode_enabled) {
                            if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_UPPER_TOP) {
                                draw_tint_vline(
                                    output, screen_x, 
                                    proj_slope_end_height, proj_slope_start_height, 
                                    prev_drawn_top, prev_drawn_bot
                                );
                            }
                        }
                        prev_drawn_bot = proj_slope_end_height;
                    }


                } else if(lower_cell_type == NORMAL_CELL) {
                // draw normal no-diagonal floor
                    int proj_step_next_height = project_to_screen(first_floor_height, next_perp_dist, pitch, ray_origin_z);
                    if(proj_step_next_height < prev_drawn_bot) {
                        draw_lit_fogged_tex_flat(
                            output, z_buffer, textures[first_floor_texture],skybox_column,
                            screen_x, proj_step_next_height, proj_floor_first_step_height, next_perp_dist, perp_dist,
                            exit_flat_u, exit_flat_v, flat_u, flat_v, prev_drawn_top, prev_drawn_bot,  FLOOR_LIGHT_FACTOR, first_floor_light_level,
                            FOG_COL
                        );
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_step_next_height, proj_floor_first_step_height, 
                            prev_drawn_top, prev_drawn_bot,
                            MAP_CELL_EDIT_ID(map_idx, WALL_SIDE_TOP)
                        );
                        if(editor_mode_enabled) {
                            if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_TOP) {
                                draw_tint_vline(
                                    output, screen_x, 
                                    proj_step_next_height, proj_floor_first_step_height, 
                                    prev_drawn_top, prev_drawn_bot
                                );
                            }
                        }
                        prev_drawn_bot = proj_step_next_height;
                    }
                } else if(lower_cell_type == NW_TO_SE_DIAG || lower_cell_type == NE_TO_SW_DIAG || 
                    lower_cell_type == THIN_WALL_X || lower_cell_type == THIN_WALL_Y ||
                    lower_cell_type == DOOR_Y || lower_cell_type == DOOR_X) {
                    diag_intersect lower_diag_intersect;
                    int lower_hits_diag;
                    float door_open_amount = this_level->parameter[map_idx]/255.0f;
                    if(lower_cell_type == DOOR_Y || lower_cell_type == DOOR_X) { 
                        lower_hits_diag = calc_door_hit(&lower_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, ray_origin_x, ray_origin_y, map_x, map_y, perp_dist, lower_cell_type, (lower_cell_type == DOOR_Y ? door_open_amount : (0.9999f-door_open_amount)));
                        // use flat exit UV coords for floor/ceiling next to door, when the door is fully open
                        //if(lower_hits_diag) {
                        //    lower_diag_intersect.mid_flat_u = CLAMP(lower_diag_intersect.mid_flat_u, MIN(flat_u, exit_flat_u), MAX(flat_u, exit_flat_u));
                        //    lower_diag_intersect.mid_flat_v = CLAMP(lower_diag_intersect.mid_flat_v, MIN(flat_v, exit_flat_v), MAX(flat_v, exit_flat_v));
                        //}
                    } else {                        
                        lower_hits_diag = calc_diag_hit(&lower_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, ray_origin_x, ray_origin_y, map_x, map_y, perp_dist, lower_cell_type);
                        //lower_hits_diag = calc_door_hit(&lower_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, ray_origin_x, ray_origin_y, map_x, map_y, perp_dist, lower_cell_type, (lower_cell_type == NE_TO_SW_DIAG ? -0.5f : 0.5f));
                    }
                    if(lower_hits_diag && lower_diag_intersect.diag_perp_dist > NEAR_PLANE_DIST) {                        

                        // draw first floor
                        int proj_first_height_diag = project_to_screen(first_floor_height, lower_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_first_height_diag < prev_drawn_bot) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_floor_texture],skybox_column,
                                screen_x, proj_first_height_diag, proj_floor_first_step_height, lower_diag_intersect.diag_perp_dist, perp_dist,
                                lower_diag_intersect.mid_flat_u, lower_diag_intersect.mid_flat_v, flat_u, flat_v, prev_drawn_top, prev_drawn_bot,  FLOOR_LIGHT_FACTOR, first_floor_light_level,
                                FOG_COL
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_first_height_diag, proj_floor_first_step_height, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, first_floor_side)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == first_floor_side) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_first_height_diag, proj_floor_first_step_height, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_bot = proj_first_height_diag;
                        }

                        // draw step
                        int proj_second_height_diag = project_to_screen(second_floor_height, lower_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        int proj_floor_anchor_height_diag = project_to_screen(floor_anchor, lower_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        int proj_zero_height_diag = project_to_screen(0, lower_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_second_height_diag < prev_drawn_bot) {
                            if(floor_anchor_is_not_zero) {
                                draw_skybox_vline(
                                    output, skybox_column, screen_x, MAX(prev_drawn_top, proj_floor_anchor_height_diag), prev_drawn_bot
                                );
                            }
                            draw_lit_fogged_clipped_textured_wall(
                                output, z_buffer,
                                ((lower_diag_wall_tex) == SKYBOX_TEX_IDX), 
                                get_texture_column(textures[lower_diag_wall_tex], lower_diag_intersect.diag_wall_u), skybox_column,
                                screen_x, proj_second_height_diag, proj_floor_anchor_height_diag,
                                second_floor_height, floor_anchor, BOTTOM_PEGGED,
                                prev_drawn_top, prev_drawn_bot, lower_diag_intersect.diag_perp_dist, DIAG_LIGHT_FACTOR, lower_diag_light_level, 
                                FOG_COL, REPEAT_TEX
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_second_height_diag, proj_zero_height_diag, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, WALL_SIDE_LOWER_DIAG)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_LOWER_DIAG) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_second_height_diag, proj_zero_height_diag, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_bot = proj_second_height_diag;
                        }
                        // draw second floor
                        int proj_second_height_next = project_to_screen(second_floor_height, next_perp_dist, pitch, ray_origin_z);
                        if(lower_cell_type != DOOR_Y && lower_cell_type != DOOR_X && proj_second_height_next < prev_drawn_bot) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[second_floor_texture],skybox_column,
                                screen_x, proj_second_height_next, proj_second_height_diag, next_perp_dist, lower_diag_intersect.diag_perp_dist,
                                exit_flat_u, exit_flat_v, lower_diag_intersect.mid_flat_u, lower_diag_intersect.mid_flat_v, prev_drawn_top, prev_drawn_bot,  
                                FLOOR_LIGHT_FACTOR, second_floor_light_level,
                                FOG_COL
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_second_height_next, proj_second_height_diag, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, second_floor_side)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == second_floor_side) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_second_height_next, proj_second_height_diag, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_bot = proj_second_height_next;

                        } 
                        
                        if ((lower_cell_type == DOOR_Y || lower_cell_type == DOOR_X) && proj_floor_first_step_height_at_next_dist < prev_drawn_bot) {
                            // draw lower floor a second time..
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_floor_texture],skybox_column,
                                screen_x, proj_floor_first_step_height_at_next_dist,  proj_first_height_diag, next_perp_dist, lower_diag_intersect.diag_perp_dist,
                                exit_flat_u, exit_flat_v, lower_diag_intersect.mid_flat_u, lower_diag_intersect.mid_flat_v, prev_drawn_top, prev_drawn_bot, FLOOR_LIGHT_FACTOR, first_floor_light_level,
                                FOG_COL
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_floor_first_step_height_at_next_dist,  proj_first_height_diag,
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, first_floor_side)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == first_floor_side) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_floor_first_step_height_at_next_dist,  proj_first_height_diag,
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_bot = proj_floor_first_step_height_at_next_dist;
                        }


                    } else {

                        // just draw regular floor
                        int proj_step_next_height = project_to_screen(first_floor_height, next_perp_dist, pitch, ray_origin_z);
                        if(proj_step_next_height < prev_drawn_bot) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_floor_texture],skybox_column,
                                screen_x, proj_step_next_height, proj_floor_first_step_height, next_perp_dist, perp_dist,
                                exit_flat_u, exit_flat_v, flat_u, flat_v, prev_drawn_top, prev_drawn_bot,  FLOOR_LIGHT_FACTOR, first_floor_light_level,
                                FOG_COL
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_step_next_height, proj_floor_first_step_height, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, first_floor_side)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == first_floor_side) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_step_next_height, proj_floor_first_step_height, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_bot = proj_step_next_height;
                        }
                    }
                }

                // draw normal no-diagonal ceil
                
                if(upper_cell_type == SLOPE_Y || upper_cell_type == SLOPE_X) {
                    float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-my_floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - my_floorf(ray_origin_y)) : hit_y - my_floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-my_floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - my_floorf(ray_origin_x)) : hit_x - my_floorf(hit_x);
                    if(!in_start_cell && side == VERTICAL_SIDE) {
                        if(step_x == -1) {
                            x_start = 1.0f;
                        } else {
                            x_start = 0.0f;
                        }
                    }
                    float exit = (upper_cell_type == SLOPE_X) ? x_exit : y_exit;
                    float start = (upper_cell_type == SLOPE_X) ? x_start : y_start;

                    float slope_start_height = (float)first_ceil_height + start*(float)((float)second_ceil_height - (float)first_ceil_height);
                    float slope_end_height = (float)first_ceil_height + exit*(float)((float)second_ceil_height - (float)first_ceil_height);

                    int proj_slope_start_height = project_to_screen(slope_start_height, perp_dist, pitch, ray_origin_z);
                    int proj_slope_end_height = project_to_screen(slope_end_height, next_perp_dist, pitch, ray_origin_z);

                    // x+ side is upper height


                    if (proj_slope_end_height > prev_drawn_top) {
                        draw_lit_fogged_tex_flat(
                            output, z_buffer, textures[upper_ceil_texture],skybox_column,
                            screen_x, proj_slope_start_height, proj_slope_end_height, 
                            perp_dist, next_perp_dist, 
                            flat_u, flat_v, exit_flat_u, exit_flat_v, 
                            prev_drawn_top, prev_drawn_bot, CEIL_LIGHT_FACTOR, upper_ceil_light_level, FOG_COL
                        );

                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_slope_start_height, proj_slope_end_height, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, WALL_SIDE_UPPER_BOTTOM)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_UPPER_BOTTOM) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_slope_start_height, proj_slope_end_height, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                        prev_drawn_top = proj_slope_end_height;
                    }

                } else if(upper_cell_type == NORMAL_CELL) {
                    int proj_step_next_height = project_to_screen(first_ceil_height, next_perp_dist, pitch, ray_origin_z);
                    if(proj_step_next_height > prev_drawn_top) {
                        draw_lit_fogged_tex_flat(
                            output, z_buffer, textures[first_ceil_texture],skybox_column,
                            screen_x, proj_ceil_first_step_height, proj_step_next_height, perp_dist, next_perp_dist,
                            flat_u, flat_v, exit_flat_u, exit_flat_v, prev_drawn_top, prev_drawn_bot,  CEIL_LIGHT_FACTOR, first_ceil_light_level,
                            FOG_COL
                        );
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_ceil_first_step_height, proj_step_next_height, 
                            prev_drawn_top, prev_drawn_bot,
                            MAP_CELL_EDIT_ID(map_idx, WALL_SIDE_BOTTOM)
                        );
                        if(editor_mode_enabled) {
                            if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_BOTTOM) {
                                draw_tint_vline(
                                    output, screen_x, 
                                    proj_ceil_first_step_height, proj_step_next_height, 
                                    prev_drawn_top, prev_drawn_bot
                                );
                            }
                        }
                        prev_drawn_top = proj_step_next_height;
                    }
                } else if(upper_cell_type == NW_TO_SE_DIAG || upper_cell_type == NE_TO_SW_DIAG ||
                    upper_cell_type == THIN_WALL_X || upper_cell_type == THIN_WALL_Y) {
                    diag_intersect upper_diag_intersect;
                    int upper_hits_diag = calc_diag_hit(&upper_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, ray_origin_x, ray_origin_y, map_x, map_y, perp_dist, upper_cell_type);    
                    if(upper_hits_diag && upper_diag_intersect.diag_perp_dist > NEAR_PLANE_DIST) {                        

                        // draw first ceil
                        int proj_first_height_diag = project_to_screen(first_ceil_height, upper_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_first_height_diag > prev_drawn_top) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_ceil_texture],skybox_column,
                                screen_x, proj_ceil_first_step_height, proj_first_height_diag, perp_dist, upper_diag_intersect.diag_perp_dist,
                                flat_u, flat_v,  upper_diag_intersect.mid_flat_u, upper_diag_intersect.mid_flat_v, prev_drawn_top, prev_drawn_bot, CEIL_LIGHT_FACTOR, first_ceil_light_level,
                                FOG_COL
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_ceil_first_step_height, proj_first_height_diag, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, first_ceil_side)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == first_ceil_side) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_ceil_first_step_height, proj_first_height_diag, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_top = proj_first_height_diag;
                        }
                        
                        // draw step
                        int proj_second_height_diag = project_to_screen(second_ceil_height, upper_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        int proj_ceil_anchor_height_diag = project_to_screen(ceil_anchor, upper_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        int proj_max_height_diag = project_to_screen(MAX_WALL_HEIGHT, upper_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_second_height_diag > prev_drawn_top) {

                            if(ceil_anchor_is_not_max) {
                                draw_skybox_vline(
                                    output, skybox_column, screen_x, prev_drawn_top, MIN(prev_drawn_bot, proj_ceil_anchor_height_diag)
                                );
                            }

                            draw_lit_fogged_clipped_textured_wall(
                                output, z_buffer,
                                ((upper_diag_wall_tex) == SKYBOX_TEX_IDX),
                                get_texture_column(textures[upper_diag_wall_tex], upper_diag_intersect.diag_wall_u),skybox_column,
                                screen_x, proj_ceil_anchor_height_diag, proj_second_height_diag,
                                ceil_anchor, second_ceil_height, TOP_PEGGED,
                                prev_drawn_top, prev_drawn_bot, upper_diag_intersect.diag_perp_dist, DIAG_LIGHT_FACTOR, upper_diag_light_level, 
                                FOG_COL, REPEAT_TEX
                            );

                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_max_height_diag, proj_second_height_diag, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, WALL_SIDE_UPPER_DIAG)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_UPPER_DIAG) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_max_height_diag, proj_second_height_diag, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_top = proj_second_height_diag;
                        }
                        
                        // draw second ceil
                        int proj_second_height_next = project_to_screen(second_ceil_height, next_perp_dist, pitch, ray_origin_z);
                        if(proj_second_height_next > prev_drawn_top) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[second_ceil_texture],skybox_column,
                                screen_x, proj_second_height_diag, proj_second_height_next, upper_diag_intersect.diag_perp_dist, next_perp_dist, 
                                upper_diag_intersect.mid_flat_u, upper_diag_intersect.mid_flat_v, exit_flat_u, exit_flat_v, prev_drawn_top, prev_drawn_bot,  
                                CEIL_LIGHT_FACTOR, second_ceil_light_level,
                                FOG_COL
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_second_height_diag, proj_second_height_next, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, second_ceil_side)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == second_ceil_side) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_second_height_diag, proj_second_height_next, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_top = proj_second_height_next;

                        }
                        

                    } else {
                        
                        // just draw regular ceil
                        int proj_step_next_height = project_to_screen(first_ceil_height, next_perp_dist, pitch, ray_origin_z);
                        if(proj_step_next_height > prev_drawn_top) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_ceil_texture],skybox_column,
                                screen_x, proj_ceil_first_step_height, proj_step_next_height, perp_dist, next_perp_dist,
                                flat_u, flat_v, exit_flat_u, exit_flat_v, prev_drawn_top, prev_drawn_bot, CEIL_LIGHT_FACTOR, first_ceil_light_level,
                                FOG_COL
                            );
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_ceil_first_step_height, proj_step_next_height, 
                                prev_drawn_top, prev_drawn_bot,
                                MAP_CELL_EDIT_ID(map_idx, first_ceil_side)
                            );
                            if(editor_mode_enabled) {
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == first_ceil_side) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_ceil_first_step_height, proj_step_next_height, 
                                        prev_drawn_top, prev_drawn_bot
                                    );
                                }
                            }
                            prev_drawn_top = proj_step_next_height;
                        }
                            
                    }
                    
                }


            }

            // adjust clipping for sprites we hit on exit of this cell
            if(can_add_sprite && hit_exit_sprite != EMPTY_SPRITE_INDEX) {
                sprite_cache[num_sprites_hit-1].prev_drawn_bot = prev_drawn_bot;
                sprite_cache[num_sprites_hit-1].prev_drawn_top = prev_drawn_top;
            }


        }
    

        draw_skybox_vline(
            output, skybox_column, screen_x, prev_drawn_top, prev_drawn_bot
        );



        if(num_sprites_hit > 0) {
            for(int i = num_sprites_hit-1; i >= 0; i--) {
                sprite_cache_entry spr = sprite_cache[i];

                if(spr.flat_sprite) {
                    int height = sprite_cache[i].bottom_height;
                    float z0 = sprite_cache[i].z0;
                    float z1 = sprite_cache[i].z1;
                    float unclipped_y0 = project_to_screen(height, z0, pitch, ray_origin_z);
                    float unclipped_y1 = project_to_screen(height, z1, pitch, ray_origin_z);
                    float u0 = spr.u0;
                    float u1 = spr.u1;
                    float v0 = spr.v0;
                    float v1 = spr.v1;
                    if(unclipped_y1 < unclipped_y0) {

                        //if(!spr.double_sided) {
                            continue;
                        //}
                        float tmp = unclipped_y0;
                        unclipped_y0 = unclipped_y1;
                        unclipped_y1 = tmp;
                        tmp = u0;
                        u0 = u1;
                        u1 = tmp;
                        tmp = v0;
                        v0 = v1;
                        v1 = tmp;

                        tmp = z0;
                        z0 = z1;
                        z1 = z0;
                    }
                    draw_lit_fogged_textured_z_buffered_blended_flat_sprite(
                        output, z_buffer, sprites[spr.sprite_idx], skybox_column, screen_x, 
                        unclipped_y0, unclipped_y1, 
                        z0, z1, u0, v0, u1, v1, 
                        spr.prev_drawn_top, spr.prev_drawn_bot, spr.light_factor, BRIGHT, FOG_COL, DO_ALPHA_BLEND
                    );               
                    draw_edit_vline(
                        edit_id_buffer, screen_x, 
                        unclipped_y0, unclipped_y1, spr.prev_drawn_top, spr.prev_drawn_bot,
                        MAP_CELL_EDIT_ID(spr.map_idx, spr.sprite_thg)
                    );
                    if(editor_mode_enabled) {     
                        if(flash_frame && editor_selected_map_idx == spr.map_idx && editor_selected_side == spr.sprite_thg) {
                            draw_tint_vline(
                                output, screen_x, 
                                unclipped_y0, unclipped_y1,
                                spr.prev_drawn_top, spr.prev_drawn_bot
                            );
                        }
                    }

                } else {
                    
                    float bot_height = sprite_cache[i].bottom_height;
                    float top_height = sprite_cache[i].top_height;
                    float z = sprite_cache[i].z0;
                    float unclipped_y0 = project_to_screen(top_height, z, pitch, ray_origin_z);
                    float unclipped_y1 = project_to_screen(bot_height, z, pitch, ray_origin_z);
                    u32* tex_col = get_texture_column(sprites[spr.sprite_idx], spr.u0);

                    u32 skip = rle_spr_top_skips[spr.sprite_idx*32+(int)(spr.u0*32.0f)];

                    draw_lit_fogged_textured_z_buffered_blended_sprite(
                        output, z_buffer, 0, tex_col, skip, skybox_column, screen_x, 
                        unclipped_y0, unclipped_y1, 
                        spr.v0*8.0f, spr.v1*8.0f, TOP_PEGGED,
                        spr.prev_drawn_top, spr.prev_drawn_bot,
                        z, spr.light_factor, BRIGHT, FOG_COL, 1, DO_ALPHA_BLEND, NO_DEPTH_TEST
                    );
                    draw_alpha_edit_vline(
                        edit_id_buffer, tex_col, screen_x, 
                        unclipped_y0, unclipped_y1,
                        spr.prev_drawn_top, spr.prev_drawn_bot,
                        MAP_CELL_EDIT_ID(spr.map_idx, spr.sprite_thg)
                    );
                    if(editor_mode_enabled) {
                        if(flash_frame && editor_selected_map_idx == spr.map_idx && editor_selected_side == spr.sprite_thg) {
                            draw_alpha_tint_vline(
                                output, tex_col, screen_x, 
                                unclipped_y0, unclipped_y1,
                                spr.prev_drawn_top, spr.prev_drawn_bot
                            );
                        }
                    }
                }
            }
        }
    }

  
    return; 
}

/*

    BILLBOARD SPRITES, transformed to camera space
*/
typedef struct {
    float x0, x1;
    float y0, y1;
    float world_y0, world_y1;
    float z;
    u16 map_or_entity_idx;
    int image_idx;
    int is_entity;
} transformed_sprite;
// tranformed into camera space: x, y0, y1, z (y0 and y1 are not actually transformed)
transformed_sprite *transformed_sprites; //[MAP_SIZE*MAP_SIZE];

#define MAX_REQUESTED_SPRITES (2048)
#define MAX_BILLBOARD_SPRITES ((MAP_SIZE*MAP_SIZE) + MAX_REQUESTED_SPRITES)
int num_trans_sprites = 0;

int sorted_insert_transformed_sprite(
    float screen_x0, float screen_x1, 
    float screen_y0, float screen_y1, 
    float world_y0, float world_y1, float cam_space_z, 
    int image_idx, int map_or_entity_idx, int is_entity) {
    if(num_trans_sprites >= MAX_BILLBOARD_SPRITES) {
        return -1;
    }
    int i = num_trans_sprites++;
    for(int j = i-1; j >= 0; j--) {
        if(transformed_sprites[j].z >= cam_space_z) {
            break;
        }
        transformed_sprites[i] = transformed_sprites[j];
        i = j;
    }
    transformed_sprites[i].x0 = screen_x0;
    transformed_sprites[i].x1 = screen_x1;
    transformed_sprites[i].y0 = screen_y0;
    transformed_sprites[i].y1 = screen_y1;
    transformed_sprites[i].world_y0 = world_y0;
    transformed_sprites[i].world_y1 = world_y1;
    transformed_sprites[i].z = cam_space_z;
    transformed_sprites[i].map_or_entity_idx = map_or_entity_idx;
    transformed_sprites[i].is_entity = is_entity;
    transformed_sprites[i].image_idx = image_idx;
    return i;
}

int transform_and_submit_sprite(
    float cam_x, float cam_y, float cam_z, 
    float right_x, float right_y, 
    float forward_x, float forward_y, 
    int image_idx, float x, float y, float z, 
    int map_or_entity_idx, int is_entity) {
    if(image_idx == EMPTY_SPRITE_INDEX) {
        return -1;
    }

    float scale = sprite_scales[image_idx];

    float sprite_world_x = x;
    float sprite_world_y = y;

    // vertical world coordinate
    float sprite_world_z1 = z;
    float sprite_world_z0 = sprite_world_z1+8.0f*scale;

    
    float rel_x = sprite_world_x - cam_x;
    float rel_y = sprite_world_y - cam_y; // 2d y axis

    float rot_x = rel_x * right_x  + rel_y * right_y;
    float rot_z = rel_x * forward_x + rel_y * forward_y;

    float width = 1.24f*scale;
    float half_width = width/2.0f;

    if(rot_z < NEAR_PLANE_DIST) { return -1; }

    float screen_x0 = FP_SCREEN_WIDTH/2.0f - ((rot_x-half_width)*FOCAL_LENGTH/rot_z);
    float screen_x1 = FP_SCREEN_WIDTH/2.0f - ((rot_x+half_width)*FOCAL_LENGTH/rot_z);

    if(MAX(screen_x0, screen_x1) < 0 || MIN(screen_x0, screen_x1) > FP_SCREEN_WIDTH-1) {
        return -1;
    }

    float screen_y0 = project_to_screen(sprite_world_z0, rot_z, pitch, cam_z);
    float screen_y1 = project_to_screen(sprite_world_z1, rot_z, pitch, cam_z);
    // find the correct place to put this transformed sprite
    return sorted_insert_transformed_sprite(
        screen_x0, screen_x1, screen_y0, screen_y1, sprite_world_z0, sprite_world_z1,
        rot_z, image_idx, map_or_entity_idx, is_entity
    );
}


void draw_transformed_sprites(
    u32* output, edit_wall_id* edit_id_buffer, u16* z_buffer, int flash_frame, float camera_z, int start_x, int end_x,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_thg
) {
    for(int spr = 0; spr < num_trans_sprites; spr++) {
        transformed_sprite sprite = transformed_sprites[spr];

        float screen_x0 = sprite.x0;
        float screen_x1 = sprite.x1;
        float screen_y0 = sprite.y0;
        float screen_y1 = sprite.y1;
        float z = sprite.z;
        int spr_idx = sprite.image_idx;
        int is_entity = sprite.is_entity;
        int map_or_entity_idx = sprite.map_or_entity_idx;

        if(screen_x0 > screen_x1) {
            float tmp = screen_x0;
            screen_x0 = screen_x1;
            screen_x1 = tmp;
        }
        if(spr_idx >= NUM_SPRITES || spr_idx == EMPTY_SPRITE_INDEX) {
            continue;
        }

        int clipped_sprite_left_x = MAX(start_x, (int)screen_x0);
        int clipped_sprite_right_x = MIN(end_x-1, (int)screen_x1);
        float tex_per_pix = 1.0f / (screen_x1-screen_x0);

        for(int x = clipped_sprite_left_x; x <= clipped_sprite_right_x; x++) {
            //int dx = x-screen_x0;
            if(screen_y1 > 0 && screen_y0 < FP_SCREEN_HEIGHT-1) {
                float u = (x-screen_x0)*tex_per_pix;
                u32* tex_col = get_texture_column(sprites[spr_idx], u);
                
                u32 skip = rle_spr_top_skips[spr_idx*32+(int)(u*32.0f)];
                draw_lit_fogged_textured_z_buffered_blended_sprite(
                    output, z_buffer,  0, tex_col, skip, textures[SKYBOX_TEX_IDX],
                    x, screen_y0, screen_y1, sprite.world_y0, sprite.world_y1, TOP_PEGGED, 0, FP_SCREEN_HEIGHT,
                    z, 1.0f, BRIGHT, FOG_COL, NO_REPEAT_TEX, DO_ALPHA_BLEND, DO_DEPTH_TEST
                );
                edit_wall_id id_val;
                if(is_entity) {
                    id_val = ENTITY_EDIT_ID(map_or_entity_idx);
                } else {
                    id_val = MAP_CELL_EDIT_ID(map_or_entity_idx, CELL_SPRITE);
                }
                draw_z_buffered_alpha_edit_vline(
                    edit_id_buffer, z_buffer, tex_col,
                    x, screen_y0, screen_y1, z, 0, FP_SCREEN_HEIGHT, 
                    id_val,
                    DO_DEPTH_TEST, DO_ALPHA_TEST
                );
                if(editor_mode_enabled) {
                    int selected = (editor_selected_map_idx == map_or_entity_idx && 
                        ((is_entity && editor_selected_thg == ENTITY) || 
                         (!is_entity && editor_selected_thg == CELL_SPRITE))
                    );

                    if(flash_frame && selected) {
                        draw_z_buffered_alpha_tint_vline(
                            output, z_buffer, tex_col,
                            x, screen_y0, screen_y1, z, 0, FP_SCREEN_HEIGHT, DO_DEPTH_TEST, DO_ALPHA_TEST
                        );
                    }
                }
            }
        }
    }
}


void raycast_wrapper(void* arg_var) {
    thread_params* tp = (thread_params*)arg_var;
    draw_first_person_level_inner(
        tp->output, tp->edit_id_buffer, tp->z_buffer, 
        tp->start_x, tp->end_x,
        tp->flash_frame, tp->this_level, tp->player_x, tp->player_y, tp->player_z,
        tp->player_ang, tp->pitch, 
        tp->editor_mode_enabled, tp->editor_selected_map_idx, tp->editor_selected_thg,
        tp->visited_cell_bitmap, tp->sprite_cache
    );
}

void draw_sprites_wrapper(void* arg_var) {
    thread_params* tp = (thread_params*)arg_var;
    draw_transformed_sprites(tp->output, tp->edit_id_buffer, tp->z_buffer, tp->flash_frame, tp->player_z, tp->start_x, tp->end_x, tp->editor_mode_enabled, tp->editor_selected_map_idx, tp->editor_selected_thg);
}


typedef struct { 
    float x, y, z;
    u8 image_idx;
    u16 entity_id;
} requested_sprite;

int num_requested_sprites = 0;
requested_sprite *requested_sprites;//[MAX_REQUESTED_SPRITES];

int request_draw_sprite(float x, float y, float z, int entity_idx, u8 image_idx) {
    if(num_requested_sprites < MAX_REQUESTED_SPRITES) {
        requested_sprites[num_requested_sprites].x = x;
        requested_sprites[num_requested_sprites].y = y;
        requested_sprites[num_requested_sprites].z = z;
        requested_sprites[num_requested_sprites].image_idx = image_idx;
        if(entity_idx < 0) {
            entity_idx = INVALID_ENTITY_ID; // invalid entity id
        }
        requested_sprites[num_requested_sprites].entity_id = entity_idx;
        return num_requested_sprites++;
    }
    return -1;
}

int request_draw_screen_space_sprite(
    float screen_x0, float screen_x1, float screen_y0, float screen_y1, 
    u8 image_idx, int entity_id
) {
    if(entity_id < 0) {
        entity_id = INVALID_ENTITY_ID;
    }
    return sorted_insert_transformed_sprite(screen_x0, screen_x1, screen_y0, screen_y1,
        0, 8, // these don't matter.  just tell the draw routine they're 8 units high so it doesn't repeat the texture
        NEAR_PLANE_DIST+0.01f,
        image_idx, entity_id, 1
    );
}

void clear_requested_sprites() {
    num_trans_sprites = 0;
}


jobpool* raycast_pool = NULL;
jobpool* raycast_manager_pool = NULL;

thread_params raycast_parms[NUM_THREADS];

void render_frame(
    thread_params* tp
) {


    u32* output = tp->output;
    edit_wall_id* edit_id_buffer = tp->edit_id_buffer;
    u16* z_buffer = tp->z_buffer;
    int start_x = tp->start_x;
    int end_x = tp->end_x;
    int flash_frame = tp->flash_frame;
    level* this_level = tp->this_level;
    float player_x = tp->player_x;
    float player_y = tp->player_y;
    float player_z = tp->player_z;
    float player_ang = tp->player_ang;
    float pitch = tp->pitch;
    int editor_mode_enabled = tp->editor_mode_enabled;
    int editor_selected_map_idx = tp->editor_selected_map_idx;
    editor_selected_thing editor_selected_side = tp->editor_selected_thg;
        
    for(int thd = 0; thd < NUM_THREADS; thd++) {
        for(int i = 0; i < MAP_SIZE*MAP_SIZE/8; i++) {
            visited_cells[thd][i] = 0;
        }
    }
    //my_memset(visited_cells, 0, sizeof(NUM_THREADS*MAP_SIZE*MAP_SIZE/8));
    for(int i = 0; i < NUM_THREADS; i++) {
        raycast_parms[i].output = output;
        raycast_parms[i].edit_id_buffer = edit_id_buffer;
        raycast_parms[i].z_buffer = z_buffer,
        raycast_parms[i].start_x = (i == 0) ? 0 : raycast_parms[i-1].end_x, //i*FP_SCREEN_WIDTH/NUM_THREADS;
        raycast_parms[i].end_x = (i == NUM_THREADS-1) ? FP_SCREEN_WIDTH : raycast_parms[i].start_x + FP_SCREEN_WIDTH/NUM_THREADS;
        raycast_parms[i].flash_frame = flash_frame;
        raycast_parms[i].this_level = this_level;
        raycast_parms[i].player_x = player_x;
        raycast_parms[i].player_y = player_y;
        raycast_parms[i].player_z = player_z;
        raycast_parms[i].player_ang = player_ang;
        raycast_parms[i].pitch = pitch;
        raycast_parms[i].editor_mode_enabled = editor_mode_enabled;
        raycast_parms[i].editor_selected_map_idx = editor_selected_map_idx;
        raycast_parms[i].editor_selected_thg = editor_selected_side;
        raycast_parms[i].visited_cell_bitmap = visited_cells[i];
        raycast_parms[i].sprite_cache = per_thread_sprite_cache[i];
    }

#ifndef PLATFORM_WEB
    for(int i = 0; i < NUM_THREADS; i++) {
        platform_add_task(raycast_pool, raycast_wrapper, &raycast_parms[i]);
    }
    platform_join_threadpool(raycast_pool);

#else
    draw_first_person_level_inner(
        output, edit_id_buffer, z_buffer, 
        start_x, end_x,
        flash_frame, this_level, player_x, player_y, player_z,
        player_ang, pitch, 
        editor_mode_enabled, editor_selected_map_idx, editor_selected_side, visited_cells[0], per_thread_sprite_cache[0]

    );
#endif

    {
        // draw the sprites we reached via raycasting
        // these are not entities but have a map idx

        float sinang = my_sinf(player_ang);
        float cosang = my_cosf(player_ang);

        // Forward
        float forward_x = cosang;
        float forward_y = sinang;

        // Right (90° clockwise from forward)
        float right_x = -sinang;
        float right_y = cosang;
        for(int j = 0; j < (MAP_SIZE*MAP_SIZE/8); j++) {
            u8 byte = visited_cells[0][j];
            for(int i = 1; i < NUM_THREADS; i++) {
                byte |= visited_cells[i][j];
            }
            for(int b = 0; b < 8; b++) {
                if((byte & (1 << b)) == 0) { 
                    continue;
                }
                
                int sector = j*8+b;

                int map_y = sector/MAP_SIZE;
                int map_x = sector - (map_y*MAP_SIZE);
                int image_idx = this_level->sprite_index[sector];
                float map_z = get_height_at_point_for_sprites(map_x, map_y, 0);
                transform_and_submit_sprite(
                    player_x, player_y, player_z, 
                    right_x, right_y, 
                    forward_x, forward_y, 
                    image_idx, 
                    map_x+0.5f, map_y+0.5f, map_z,
                    map_y*MAP_SIZE+map_x, 
                    0 // not an entity
                );
                
            }
        }

        // next are requested sprites
        // these do not have a map idx and correspond to entities
        for(int i = 0; i < num_requested_sprites; i++) {
            transform_and_submit_sprite(
                player_x, player_y, player_z, 
                right_x, right_y, 
                forward_x, forward_y, 
                requested_sprites[i].image_idx, 
                requested_sprites[i].x, 
                requested_sprites[i].y, 
                requested_sprites[i].z, 
                requested_sprites[i].entity_id, 
                1);
        }

    #ifndef PLATFORM_WEB
        for(int i = 0; i < NUM_THREADS; i++) {
            platform_add_task(raycast_pool, draw_sprites_wrapper, &raycast_parms[i]);
        }
        platform_join_threadpool(raycast_pool);


    #else 
        draw_transformed_sprites(output, edit_id_buffer, z_buffer, flash_frame, player_z, start_x, end_x, editor_mode_enabled, editor_selected_map_idx, editor_selected_side);
    #endif

        num_requested_sprites = 0;

        return;
    }

}



void render_frame_wrapper(
    void* arg_var
    ) {
    thread_params* tp = (thread_params*)arg_var;
    render_frame(tp);
}


thread_params frame_params;


void launch_render_frame(
    u32* output, edit_wall_id* edit_id_buffer, u16* z_buffer,
    int start_x, int end_x, 
    int flash_frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, float pitch,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side) {

        frame_params.output = output;
        frame_params.edit_id_buffer = edit_id_buffer;
        frame_params.z_buffer = z_buffer,
        frame_params.start_x = start_x; //i*FP_SCREEN_WIDTH/NUM_THREADS;
        frame_params.end_x = end_x;
        frame_params.flash_frame = flash_frame;
        frame_params.this_level = this_level;
        frame_params.player_x = player_x;
        frame_params.player_y = player_y;
        frame_params.player_z = player_z;
        frame_params.player_ang = player_ang;
        frame_params.pitch = pitch;
        frame_params.editor_mode_enabled = editor_mode_enabled;
        frame_params.editor_selected_map_idx = editor_selected_map_idx;
        frame_params.editor_selected_thg = editor_selected_side;

    //render_frame_wrapper(&frame_params);
    platform_add_task(
        raycast_manager_pool,
        render_frame_wrapper,
        &frame_params
    );
}

void join_render_frame() {
    platform_join_threadpool(raycast_manager_pool);
}


void init_raycast_module() {
    
    raycast_pool = platform_init_threadpool(NUM_THREADS);
    raycast_manager_pool = platform_init_threadpool(1);

    //sprite_cache_entry *sprite_cache_block = my_malloc(sizeof(sprite_cache_entry)*NUM_THREADS*MAX_SPRITE_HITS, "raycast sprite cache");
    //u8* visited_cells_block = my_malloc(sizeof(u8)*NUM_THREADS*MAP_SIZE*MAP_SIZE/8, "raycast visited cells bitmap");
    for(int i = 0; i < NUM_THREADS; i++) {
        //per_thread_sprite_cache[i] = sprite_cache_block + (i*MAX_SPRITE_HITS);
        per_thread_sprite_cache[i] = my_malloc(sizeof(sprite_cache_entry)*MAX_SPRITE_HITS, "sprite cache");
        visited_cells[i] = my_malloc(sizeof(u8)*MAP_SIZE*MAP_SIZE/8, "raycast visited cells bitmap");
        //visited_cells[i] = visited_cells_block + (MAP_SIZE*MAP_SIZE/8);
    }
    transformed_sprites = my_malloc(sizeof(transformed_sprite)*(MAX_BILLBOARD_SPRITES), "transformed billboard sprite buffer");
    requested_sprites = my_malloc(sizeof(requested_sprite)*MAX_REQUESTED_SPRITES, "requested sprite buffer");
}

