//author https://github.com/autergame

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "math.h"


#ifndef PLATFORM_WEB 
#include <windows.h>
#include <synchapi.h>
#include <winternl.h>

#pragma comment(lib, "ntdll")


typedef struct thread_pool_
{
    TP_CALLBACK_ENVIRON callback_environ;
    PTP_CLEANUP_GROUP cleanup_group;
    PTP_POOL pool;
} thread_pool;

#define thread_pool_function(function_name, arg_var_name) \
    void CALLBACK function_name(PTP_CALLBACK_INSTANCE instance, PVOID arg_var_name, PTP_WORK work)



thread_pool* thread_pool_create(int cpu_threads)
{
    assert(cpu_threads > 0);
    thread_pool* tp = (thread_pool*)calloc(1, sizeof(thread_pool));

    if (tp) {


        InitializeThreadpoolEnvironment(&tp->callback_environ);

        tp->pool = CreateThreadpool(NULL);

        SetThreadpoolThreadMinimum(tp->pool, cpu_threads);
        SetThreadpoolThreadMaximum(tp->pool, cpu_threads);

        tp->cleanup_group = CreateThreadpoolCleanupGroup();

        SetThreadpoolCallbackPool(&tp->callback_environ, tp->pool);
        SetThreadpoolCallbackCleanupGroup(&tp->callback_environ, tp->cleanup_group, NULL);
    }

    return tp;
}

void thread_pool_add_work(thread_pool* tp, PTP_WORK_CALLBACK function, void* arg_var)
{
    if (tp)
    {
        PTP_WORK work = CreateThreadpoolWork(function, arg_var, &tp->callback_environ);
        SubmitThreadpoolWork(work);
    }
}

void thread_pool_destroy(thread_pool* tp)
{
    if (tp)
    {
        CloseThreadpoolCleanupGroupMembers(tp->cleanup_group, FALSE, NULL);
        CloseThreadpoolCleanupGroup(tp->cleanup_group);

        DestroyThreadpoolEnvironment(&tp->callback_environ);

        CloseThreadpool(tp->pool);

        free(tp);
    }
}
#endif 


#define FOCAL_LENGTH (FP_SCREEN_WIDTH / (2.0f * tanf(1.57f/2.0f)))
#define HEIGHT_SCALE (4)
#define HALF_SCREEN_HEIGHT (FP_SCREEN_HEIGHT/2)

#define FOG_COL ((255<<24)|(196<<16)|(162<<8)|(103<<0))

int project_to_screen(int height, float dist, int pitch, float player_z) {
    return pitch + HALF_SCREEN_HEIGHT - (HEIGHT_SCALE * (((height - player_z) * FOCAL_LENGTH / dist) / MAX_WALL_HEIGHT));
}

void draw_tint_vline(u8* output, int x, int y0, int y1, int prev_drawn_top, int prev_drawn_bot) {
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot-1); y <= CLAMP(y1, prev_drawn_top, prev_drawn_bot-1); y++) {
        output[(x*FP_SCREEN_HEIGHT+y)*4+0] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+1] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+2] >>= 1;
    }
}

/*
void draw_depth_interp_vline(u8* output, int x, int y0, int y1, float z0, float z1, int prev_drawn_top, int prev_drawn_bot, Color col) {
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot-1); y <= CLAMP(y1, prev_drawn_top, prev_drawn_bot-1); y++) {
        float inv_z = inv_z0 + (inv_z1 - inv_z0) * ((y - y0) / (float)(y1 - y0));
        float cur_z = 1.0f / inv_z;
        float scale = 1.0f - CLAMP(cur_z / DARK_DIST, 0.0f, 1.0f);
        output[(x*FP_SCREEN_HEIGHT+y)*4+0] = col.r * scale;
        output[(x*FP_SCREEN_HEIGHT+y)*4+1] = col.g * scale;
        output[(x*FP_SCREEN_HEIGHT+y)*4+2] = col.b * scale;
    }
}
*/

// draws a textured
void draw_lit_fogged_tex_flat(
    u8* output, u8* texture, u8* decal, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, u32 fog_col) {
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    float one_over_z = 1.0f/z0;
    float d_one_over_z = ((1.0f/z1) - one_over_z) / (y1-y0);
    float u_over_z = start_u * inv_z0;
    float v_over_z = start_v * inv_z0;
    float d_u_over_z = ((end_u * inv_z1) - u_over_z) / (y1-y0);
    float d_v_over_z = ((end_v * inv_z1) - v_over_z) / (y1-y0);
    int clipped_y0 = CLAMP(y0, prev_drawn_top, prev_drawn_bot);
    int clipped_y1 = CLAMP(y1, prev_drawn_top, prev_drawn_bot);
    
    if(texture == textures[SKYBOX_TEX_IDX]) {
        return;
    }

    if(y0 > prev_drawn_bot) {
        return;
    }
    if(y1 < prev_drawn_top) {
        return;
    }
    
    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    float cur_z = 1.0f / (inv_z0 + d_one_over_z*(clipped_y0-y0));
    float cur_u = CLAMP((u_over_z+d_u_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    float cur_v = CLAMP((v_over_z+d_v_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    for(int y = clipped_y0; y < clipped_y1; y++) {
        float next_z = 1.0f / (inv_z0 + d_one_over_z*(y+1-y0));
        float next_u = CLAMP((u_over_z+d_u_over_z*(y+1-y0)) * next_z * 32.0f, 0.0f, 31.0f);
        float next_v = CLAMP((v_over_z+d_v_over_z*(y+1-y0)) * next_z * 32.0f, 0.0f, 31.0f);

        float mult = light_factor;
        float depth_scale = (CLAMP(cur_z/DARK_DIST, 0.0f, 1.0f)) * light_factor;
        float inv_depth_scale = 1.0f - depth_scale;
        u32 scaled_fog_r = (depth_scale * fog_r);
        u32 scaled_fog_g = (depth_scale * fog_g);
        u32 scaled_fog_b = (depth_scale * fog_b);

        //depth_scale *= depth_scale;
        int u = (int)floorf(cur_u);
        int v = (int)floorf(cur_v);

        int idx = (v<<5)+u;

        u32 decal_texel = *(u32*)(&decal[idx*4]);
        u32 texel = *(u32*)(&texture[idx*4]);
        u8 decal_alpha = decal_texel>>24;
        float decal_a = decal_alpha/255.0f;
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        u32 decal_r = ((decal_texel >> 16) & 0xFF);
        u32 decal_g = ((decal_texel >> 8) & 0xFF);
        u32 decal_b = ((decal_texel >> 0) & 0xFF);
        float r = ((decal_r * decal_a) + ((1.0f - decal_a) * texel_r));
        float g = ((decal_g * decal_a) + ((1.0f - decal_a) * texel_g));
        float b = ((decal_b * decal_a) + ((1.0f - decal_a) * texel_b));
        r *= mult;
        g *= mult;
        b *= mult;
        r = ((r * inv_depth_scale) + scaled_fog_r);
        g = ((g * inv_depth_scale) + scaled_fog_g);
        b = ((b * inv_depth_scale) + scaled_fog_b);
        u32 intr = CLAMP((int)r, 0, 0xFF);
        u32 intg = CLAMP((int)g, 0, 0xFF);
        u32 intb = CLAMP((int)b, 0, 0xFF);
        *(u32*)(&output[(x*FP_SCREEN_HEIGHT+y)*4]) = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        cur_z = next_z;
        cur_u = next_u;
        cur_v = next_v;
    }
}


void draw_edit_vline(edit_wall_id* edit_id_buffer, int x, float y0, float y1, int prev_drawn_top, int prev_drawn_bot, int cell_idx, wall_side side) {
    edit_wall_id id = {.alpha = 0xFF, .cell_idx = cell_idx<<2, .side = side<<2};
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}

typedef enum {
    TOP_PEGGED,
    BOTTOM_PEGGED,
} pegging_type;

void draw_lit_fogged_clipped_textured_wall(
    u8* output, 
    int draw_skybox,
    u8 *tex_column, u8* decal_tex_column,
    int x,
    float y0, float y1, 
    float world_z0, float world_z1, pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float z, float light_factor, u32 fog_col) {
    if(draw_skybox) {
        return;
    }
    if(y0 > prev_drawn_bot) {
        return;
    }
    if(y1 < prev_drawn_top) {
        return;
    }
    int units = fabsf(world_z1 - world_z0);
    float depth_scale = CLAMP(z / DARK_DIST, 0.0f, 1.0f);
    float inv_depth_scale = (1.0f - depth_scale);
    float mult = light_factor; //depth_scale * light_factor;

    float start_v = 0.0f;
    float tex_per_pix = units * 4.0f / (y1-y0);
    float decal_tex_per_pix = units * 4.0f / (y1-y0);
    if(peg_type == BOTTOM_PEGGED) {
        float end_v = (float)units * 4.0f;
        float full_wraps = end_v / 32.0f;
        float unfinished_last_wrap = (full_wraps - floorf(full_wraps));

        start_v = (32.0f * unfinished_last_wrap);
    }

    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    u32 scaled_fog_r = (depth_scale * fog_r);
    u32 scaled_fog_g = (depth_scale * fog_g);
    u32 scaled_fog_b = (depth_scale * fog_b);

    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
            int dy = y-y0;
            int idx = (int)(start_v + dy*tex_per_pix)&31;
            int decal_idx = CLAMP((int)floorf(dy*decal_tex_per_pix), 0, 31);

            u32 decal_texel = *(u32*)(&decal_tex_column[decal_idx*4]);
            u32 texel = *(u32*)(&tex_column[idx*4]);
            u8 decal_alpha = decal_texel>>24;
            float decal_a = decal_alpha/255.0f;
            u32 texel_r = ((texel >> 16) & 0xFF);
            u32 texel_g = ((texel >> 8) & 0xFF);
            u32 texel_b = ((texel >> 0) & 0xFF);
            u32 decal_r = (decal_texel >> 16)&0xFF;
            u32 decal_g = (decal_texel >> 8)&0xFF;
            u32 decal_b = (decal_texel >> 0)&0xFF;
            float r = ((decal_r * decal_a) + ((1.0f - decal_a) * texel_r));
            float g = ((decal_g * decal_a) + ((1.0f - decal_a) * texel_g));
            float b = ((decal_b * decal_a) + ((1.0f - decal_a) * texel_b));
            r *= mult;
            g *= mult;
            b *= mult;
            r = ((r * inv_depth_scale) + scaled_fog_r);
            g = ((g * inv_depth_scale) + scaled_fog_g);
            b = ((b * inv_depth_scale) + scaled_fog_b);

            u32 intr = CLAMP((int)r, 0, 0xFF);
            u32 intg = CLAMP((int)g, 0, 0xFF);
            u32 intb = CLAMP((int)b, 0, 0xFF);


            *(u32*)(&output[(x*FP_SCREEN_HEIGHT+y)*4]) = 0xFF000000|(intr<<16)|(intg<<8)|intb;

    }
}


u8* get_texture_column(u8* texture, float wall_u) {
    float u_scaled_to_tex_size = CLAMP(wall_u * 32.0f, 0.0f, 31.0f);
    int int_u = (int)floorf(u_scaled_to_tex_size);
    return &texture[int_u*TEX_SIZE*4];
}

typedef enum {
    VERTICAL_SIDE = 0,
    HORIZONTAL_SIDE = 1
} side;

#define DEGREES_TO_RAD(deg) ((deg)*.0174f)


typedef struct {
    float diag_wall_u;
    float diag_perp_dist;
    float mid_flat_u;
    float mid_flat_v;
} diag_intersect;

const float diag_dy[3] = {
    1.0f, // dummy entry for normal walls
    -1.0f, // NE_TO_SW_DIAG
    1.0f,  // NW_TO_SE_DIAG
};

#define CEIL_LIGHT_FACTOR (0.35f)
#define FLOOR_LIGHT_FACTOR (0.65f)
#define DIAG_LIGHT_FACTOR (0.87)

int calc_diag_hit(diag_intersect *result, float ray_dir_x, float ray_dir_y, float cam_dir_x, float cam_dir_y, float player_x, float player_y, int map_x, int map_y, float perp_dist, cell_types cell_type) {
    int upper_hits_diag = 0;
    result->mid_flat_u = 0.0f;
    result->mid_flat_v = 0.0f;
    result->diag_perp_dist = perp_dist;

    float diag_ix = 0.0f;
    float diag_iy = 0.0f;
    float p1x = player_x;
    float p1y = player_y;
    float q1x = player_x + ray_dir_x;
    float q1y = player_y + ray_dir_y;
    float p2x = map_x+0.5f;
    float p2y = map_y+0.5f;
    float q2x = p2x + 1.0f;
    float q2y = p2y + diag_dy[cell_type];
    float a1 = q1y - p1y;
    float b1 = p1x - q1x;
    float c1 = a1 * p1x + b1 * p1y;

    float a2 = q2y - p2y;//-1;
    float b2 = p2x - q2x;//-1;
    float c2 = a2 * p2x + b2 * p2y;

    float determinant = a1 * b2 - a2 * b1;

    diag_ix = (c1 * b2 - c2 * b1) / determinant;
    diag_iy = (a1 * c2 - a2 * c1) / determinant;
    
    float lx = fabsf(diag_ix - map_x);
    result->diag_wall_u = lx;


    result->mid_flat_u = diag_ix - floorf(diag_ix);
    result->mid_flat_v = diag_iy - floorf(diag_iy);


    if(floorf(diag_ix) == map_x && floorf(diag_iy) == map_y) {
        upper_hits_diag = 1;
        float dx = diag_ix-player_x;
        float dy = diag_iy-player_y;
        result->diag_perp_dist = dx*cam_dir_x + dy*cam_dir_y;
    }
    return upper_hits_diag;
                
}

float light_level_mults[4] = {1.0f, 0.25f, 1.5f, 1.5f};

typedef struct {
    u8* output;
    edit_wall_id* edit_id_buffer;
    int start_x;
    int end_x;
    int frame; 
    level* this_level;
    float player_x; float player_y; float player_z; float player_ang; int pitch;
    int editor_mode_enabled;
    int editor_selected_map_idx;
    wall_side editor_selected_side;
    volatile u64 finished;
} thread_params;

void draw_first_person_level_inner(
    u8* output, edit_wall_id* edit_id_buffer,
    int start_x, int end_x, 
    int frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, int pitch,
    int editor_mode_enabled, int editor_selected_map_idx, wall_side editor_selected_side
) {


    int flash_frame = (frame&0b1000000) == 0b1000000;

    //u8* cur_level = this_level;
    u8* cur_level_floor = this_level->floor;
    u8* cur_level_ceil = this_level->ceil;
    u8* cur_level_upper_floor = this_level->upper_floor;
    u8* cur_level_upper_ceil = this_level->upper_ceil;

    float cam_dir_x = cosf(player_ang);
    float cam_dir_y = sinf(player_ang);

    const int start_map_x = floorf(player_x);
    const int start_map_y = floorf(player_y);
    
    for(int ix = start_x; ix < end_x; ix++) {
        int screen_x = (FP_SCREEN_WIDTH-1)-ix;
        float cam_x = 2.0f * screen_x / (float)FP_SCREEN_WIDTH - 1.0f; // -1 to 1
        float ray_dir_x = cosf(player_ang) + cam_x * -sinf(player_ang);
        float ray_dir_y = sinf(player_ang) + cam_x * cosf(player_ang);

        float ray_ang = atan2f(ray_dir_y, ray_dir_x);
        if(ray_ang < 0.0f) {
            ray_ang += 6.28f;
        }
         {
            u8* skybox = textures[SKYBOX_TEX_IDX];

            float u = 256.0f* (0.5f + ray_ang / (2.0f * 3.14159));
            int int_u = ((int)u)&(SKYBOX_TEX_WIDTH-1);
            for(int y = 0; y < FP_SCREEN_HEIGHT; y++) {
                int v = (SKYBOX_TEX_HEIGHT/2+(int)(SKYBOX_V_PER_PIX*(y+-pitch)))&(SKYBOX_TEX_HEIGHT-1);
                u32 texel = *(u32*)(&skybox[(int_u*SKYBOX_TEX_HEIGHT+v)*4]);
                *(u32*)(&output[(screen_x*FP_SCREEN_HEIGHT+y)*4]) = texel; //(0xFF000000 | texel);
            }
            //return;
        }

        // length of ray from one x/y side to the next x/y side
        float delta_dist_x = fabsf(1.0f / ray_dir_x);
        float delta_dist_y = fabsf(1.0f / ray_dir_y);

        int map_x = start_map_x;
        int map_y = start_map_y;
        

        float def_exit_u = (ray_dir_x >= 0) ? 1.0f : 0.0f;
        float def_exit_v = (ray_dir_y >= 0) ? 1.0f : 0.0f;
        float def_start_u = (ray_dir_x >= 0) ? 0.0f : 1.0f;
        float def_start_v = (ray_dir_y >= 0) ? 0.0f : 1.0f;
        int cell_idx = map_y * MAP_SIZE + map_x;
        int prev_floor_height = cur_level_floor[cell_idx];
        int prev_ceil_height = cur_level_ceil[cell_idx];

        // TODO: not always correct if the current cell has a step!!
        u8 prev_ceil_texture = this_level->ctex[cell_idx];
        u8 prev_floor_texture = this_level->ftex[cell_idx];
        float prev_cell_light_level = light_level_mults[this_level->light[cell_idx]];
        wall_side prev_ceil_side = WALL_SIDE_BOTTOM;
        wall_side prev_floor_side = WALL_SIDE_TOP;

        float prev_perp_dist = NEAR_PLANE_DIST;
        float prev_flat_u = player_x - map_x;
        float prev_flat_v = player_y - map_y;
        int proj_prev_floor_height_at_prev_dist = project_to_screen(prev_floor_height, prev_perp_dist, pitch, player_z);
        int proj_prev_ceil_height_at_prev_dist = project_to_screen(prev_ceil_height, prev_perp_dist, pitch, player_z);


        int step_x = (ray_dir_x < 0) ? -1 : 1;
        float side_dist_x = (ray_dir_x < 0) ? ((player_x - map_x) * delta_dist_x) : ((map_x + 1.0f - player_x) * delta_dist_x);

        int step_y = (ray_dir_y < 0) ? -1 : 1;
        float side_dist_y = (ray_dir_y < 0) ? ((player_y - map_y) * delta_dist_y) : ((map_y + 1.0 - player_y) * delta_dist_y);


        int prev_drawn_top = 0;
        int prev_drawn_bot = FP_SCREEN_HEIGHT;
        const int MAX_STEPS = 64;

        int prev_map_idx = cell_idx;
        

        for(int i = 0; i < MAX_STEPS && (prev_drawn_top < prev_drawn_bot); i++) {
            float perp_dist;
            float wall_u;
            float flat_u, flat_v;           // the u,v position of where we enter the next cell (which we use on the next iteration)
            float exit_flat_u, exit_flat_v; // the u,v position of where we "exit" the current cell before stepping to the new one
            float hit_x;
            float hit_y;

            int side;
            float light_factor;

            if(side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = VERTICAL_SIDE;
                light_factor = 1.0f;
                perp_dist = ((map_x - player_x + (1 - step_x) * 0.5f) / ray_dir_x);
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = HORIZONTAL_SIDE;
                light_factor = .75f;
                perp_dist = ((map_y - player_y + (1 - step_y) * 0.5f) / ray_dir_y);
            }
            if(map_x >= MAP_SIZE || map_x < 0 || map_y >= MAP_SIZE || map_y < 0) {
                break;
            }

            int map_idx = map_y * MAP_SIZE + map_x;
            int selected_cur_map_idx = editor_selected_map_idx == map_idx;
            int selected_prev_map_idx = editor_selected_map_idx == prev_map_idx;
            cell_types upper_cell_type = this_level->upper_cell_types[map_idx];
            cell_types lower_cell_type = this_level->lower_cell_types[map_idx];

            perp_dist = MAX(prev_perp_dist, perp_dist);


            int upper_hits_diag = 0;
            int lower_hits_diag = 0;

            diag_intersect upper_diag_intersect, lower_diag_intersect;
            if(upper_cell_type == NE_TO_SW_DIAG || upper_cell_type == NW_TO_SE_DIAG) {
                upper_hits_diag = calc_diag_hit(&upper_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, player_x, player_y, map_x, map_y, perp_dist, upper_cell_type);
            }
            if(lower_cell_type == NE_TO_SW_DIAG || lower_cell_type == NW_TO_SE_DIAG) {            
                lower_hits_diag = calc_diag_hit(&lower_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, player_x, player_y, map_x, map_y, perp_dist, lower_cell_type);    
            }


            int proj_prev_floor_height = project_to_screen(prev_floor_height, perp_dist, pitch, player_z);
            int proj_prev_ceil_height = project_to_screen(prev_ceil_height, perp_dist, pitch, player_z);

            hit_x = player_x + perp_dist * ray_dir_x;
            hit_y = player_y + perp_dist * ray_dir_y;

            

            // lower floor height and higher ceil height are used for diagonals and other special cell types
            int floor_height = cur_level_floor[map_idx];
            int upper_floor_height = cur_level_upper_floor[map_idx];
            int ceil_height = cur_level_ceil[map_idx];
            int upper_ceil_height = cur_level_upper_ceil[map_idx];


            u8 upper_wall_tex, lower_wall_tex;
            wall_side upper_intersect_wall_side, lower_intersect_wall_side;
            if(side == VERTICAL_SIDE) {
                wall_u = hit_y - floorf(hit_y);
                flat_u = def_start_u;
                flat_v = wall_u;
                exit_flat_u = def_exit_u;
                exit_flat_v = wall_u;

                if(ray_dir_x > 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_WEST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_WEST;
                    wall_u = 1.0f - wall_u;
                    upper_wall_tex = this_level->uwtex[map_idx];
                    lower_wall_tex = this_level->lwtex[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_EAST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_EAST;
                    upper_wall_tex = this_level->uetex[map_idx];
                    lower_wall_tex = this_level->letex[map_idx];
                }
            } else {
                wall_u = hit_x - floorf(hit_x);
                flat_u = wall_u;
                flat_v = def_start_v;
                exit_flat_u = wall_u;
                exit_flat_v = def_exit_v;
                if(ray_dir_y < 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_NORTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_NORTH;
                    wall_u = 1.0f - wall_u;
                    upper_wall_tex = this_level->untex[map_idx];
                    lower_wall_tex = this_level->lntex[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_SOUTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_SOUTH;
                    upper_wall_tex = this_level->ustex[map_idx];
                    lower_wall_tex = this_level->lstex[map_idx];
                }
            }
            u8 upper_diag_tex = this_level->udtex[map_idx];
            u8 lower_diag_tex = this_level->ldtex[map_idx];

            
            u8 upper_floor_texture = this_level->uftex[map_idx];
            u8 floor_texture = this_level->ftex[map_idx];
            u8 upper_ceil_texture = this_level->uctex[map_idx];
            u8 ceil_texture = this_level->ctex[map_idx];

            float cell_light_level = light_level_mults[this_level->light[map_idx]];

            int enters_right_side = (step_x == -1) && (side == VERTICAL_SIDE);
            int enters_left_side = (step_x == 1) && (side == VERTICAL_SIDE);
            int enters_top_side = (step_y == 1) && (side == HORIZONTAL_SIDE);
            //int enters_bot_side = (step_y == -1) && (side == HORIZONTAL_SIDE);


            int first_floor_height = floor_height;
            int second_floor_height = upper_floor_height;
            int first_ceil_height = ceil_height;
            int second_ceil_height = upper_ceil_height;

            u8 first_floor_texture = floor_texture;
            u8 second_floor_texture = upper_floor_texture;
            u8 first_ceil_texture = ceil_texture;
            u8 second_ceil_texture = upper_ceil_texture;
            wall_side first_floor_side = WALL_SIDE_TOP;
            wall_side second_floor_side = WALL_SIDE_UPPER_TOP;
            wall_side first_ceil_side = WALL_SIDE_BOTTOM;
            wall_side second_ceil_side = WALL_SIDE_UPPER_BOTTOM;
            if((upper_cell_type == NE_TO_SW_DIAG && (enters_top_side || enters_left_side)) ||
                (upper_cell_type == NW_TO_SE_DIAG && (enters_top_side || enters_right_side))) {
                first_ceil_height = upper_ceil_height;
                first_ceil_texture = upper_ceil_texture;
                first_ceil_side = WALL_SIDE_UPPER_BOTTOM;
                second_ceil_height = ceil_height;
                second_ceil_texture = ceil_texture;
                second_ceil_side = WALL_SIDE_BOTTOM;
            }   


            if((lower_cell_type == NE_TO_SW_DIAG && (enters_top_side || enters_left_side)) ||
                (lower_cell_type == NW_TO_SE_DIAG && (enters_top_side || enters_right_side))) {
                first_floor_height = upper_floor_height;
                first_floor_texture = upper_floor_texture;
                first_floor_side = WALL_SIDE_UPPER_TOP;
                second_floor_height = floor_height;
                second_floor_texture = floor_texture;
                second_floor_side = WALL_SIDE_TOP;
            }

            int proj_first_floor_height_at_boundary = project_to_screen(first_floor_height, perp_dist, pitch, player_z);
            int proj_second_floor_height_at_boundary = project_to_screen(second_floor_height,perp_dist, pitch, player_z);
            int proj_first_floor_height_at_diag = project_to_screen(first_floor_height, lower_diag_intersect.diag_perp_dist, pitch, player_z);
            int proj_second_floor_height_at_diag = project_to_screen(second_floor_height,lower_diag_intersect.diag_perp_dist, pitch, player_z);


            int proj_first_ceil_height_at_boundary = project_to_screen(first_ceil_height, perp_dist, pitch, player_z);
            int proj_second_ceil_height_at_boundary = project_to_screen(second_ceil_height,perp_dist, pitch, player_z);
            int proj_first_ceil_height_at_diag = project_to_screen(first_ceil_height, upper_diag_intersect.diag_perp_dist, pitch, player_z);
            int proj_second_ceil_height_at_diag = project_to_screen(second_ceil_height,upper_diag_intersect.diag_perp_dist, pitch, player_z);



            if(proj_prev_ceil_height > prev_drawn_top) {
                draw_lit_fogged_tex_flat(
                    output, 
                    textures[prev_ceil_texture&0xF], decals[prev_ceil_texture>>4],
                    screen_x, 
                    proj_prev_ceil_height_at_prev_dist, proj_prev_ceil_height, 
                    prev_perp_dist, perp_dist, 
                    prev_flat_u, prev_flat_v, exit_flat_u, exit_flat_v, 
                    prev_drawn_top, prev_drawn_bot, prev_cell_light_level*CEIL_LIGHT_FACTOR,
                    FOG_COL);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        edit_id_buffer, screen_x,
                        proj_prev_ceil_height_at_prev_dist, proj_prev_ceil_height, 
                        prev_drawn_top, prev_drawn_bot, 
                        prev_map_idx, prev_ceil_side
                    );
                    if(flash_frame && selected_prev_map_idx && editor_selected_side == prev_ceil_side) {
                        draw_tint_vline(
                            output, screen_x, proj_prev_ceil_height_at_prev_dist, proj_prev_ceil_height,
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_top = proj_prev_ceil_height;   
            }
                
            if(proj_prev_floor_height < prev_drawn_bot) {
                draw_lit_fogged_tex_flat(
                    output, 
                    textures[prev_floor_texture&0xF],  decals[prev_floor_texture>>4],
                    screen_x, 
                    proj_prev_floor_height, proj_prev_floor_height_at_prev_dist, 
                    perp_dist, prev_perp_dist, 
                    exit_flat_u, exit_flat_v, prev_flat_u, prev_flat_v, 
                    prev_drawn_top, prev_drawn_bot, prev_cell_light_level*FLOOR_LIGHT_FACTOR,
                    FOG_COL);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        edit_id_buffer, screen_x,
                        proj_prev_floor_height, proj_prev_floor_height_at_prev_dist,
                        prev_drawn_top, prev_drawn_bot, 
                        prev_map_idx, prev_floor_side
                    );
                    if(flash_frame && selected_prev_map_idx && editor_selected_side == prev_floor_side) {
                        draw_tint_vline(
                            output, screen_x, 
                            proj_prev_floor_height, proj_prev_floor_height_at_prev_dist,
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_bot = proj_prev_floor_height;
            }


            // draw upper step


            // draw ceil first step
            
            if(proj_first_ceil_height_at_boundary > prev_drawn_top) {
                draw_lit_fogged_clipped_textured_wall(output, 
                    ((upper_wall_tex&0xF) == SKYBOX_TEX_IDX),
                    get_texture_column(textures[upper_wall_tex&0xF], wall_u),
                    get_texture_column(decals[upper_wall_tex>>4], wall_u),
                    screen_x, 
                    proj_prev_ceil_height, proj_first_ceil_height_at_boundary, 
                    prev_ceil_height, first_ceil_height, TOP_PEGGED,
                    prev_drawn_top, prev_drawn_bot, perp_dist, cell_light_level*light_factor, FOG_COL);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        edit_id_buffer, screen_x,
                        proj_prev_ceil_height, proj_first_ceil_height_at_boundary,
                        prev_drawn_top, prev_drawn_bot, 
                        map_idx, upper_intersect_wall_side
                    );
                    if(flash_frame && selected_cur_map_idx && editor_selected_side == upper_intersect_wall_side) {
                        draw_tint_vline(
                            output, screen_x, 
                            proj_prev_ceil_height, proj_first_ceil_height_at_boundary,
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_top = proj_first_ceil_height_at_boundary;
            }
            
            // draw floor first step
            if(proj_first_floor_height_at_boundary < prev_drawn_bot) {
                draw_lit_fogged_clipped_textured_wall(output, 
                    ((lower_wall_tex&0xF) == SKYBOX_TEX_IDX),
                    get_texture_column(textures[lower_wall_tex&0xF], wall_u),
                    get_texture_column(decals[lower_wall_tex>>4], wall_u),
                    screen_x, 
                    proj_first_floor_height_at_boundary, proj_prev_floor_height, 
                    prev_floor_height, first_floor_height, BOTTOM_PEGGED,
                    prev_drawn_top, prev_drawn_bot, perp_dist, cell_light_level*light_factor, FOG_COL);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        edit_id_buffer, screen_x,
                        proj_first_floor_height_at_boundary, proj_prev_floor_height, 
                        prev_drawn_top, prev_drawn_bot,
                        map_idx, lower_intersect_wall_side
                    );
                    if(flash_frame && selected_cur_map_idx && editor_selected_side == lower_intersect_wall_side) {
                        draw_tint_vline(
                            output, screen_x, 
                        proj_first_floor_height_at_boundary, proj_prev_floor_height, 
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_bot = proj_first_floor_height_at_boundary;
            }
            
            if(!upper_hits_diag) {
                ceil_height = first_ceil_height;
                proj_second_ceil_height_at_boundary = proj_first_ceil_height_at_boundary;
                second_ceil_texture = first_ceil_texture;
                prev_ceil_side = first_ceil_side;
            } else {
                ceil_height = second_ceil_height;
                prev_ceil_side = second_ceil_side;

                // draw previous ceiling
                if(proj_first_ceil_height_at_diag > prev_drawn_top) {
                    draw_lit_fogged_tex_flat(
                        output, 
                        textures[first_ceil_texture&0xF],  decals[first_ceil_texture>>4],
                        screen_x, 
                        proj_first_ceil_height_at_boundary, proj_first_ceil_height_at_diag, 
                        perp_dist, upper_diag_intersect.diag_perp_dist, 
                        flat_u, flat_v, upper_diag_intersect.mid_flat_u, upper_diag_intersect.mid_flat_v, 
                        prev_drawn_top, prev_drawn_bot, cell_light_level*CEIL_LIGHT_FACTOR,
                    FOG_COL);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_first_ceil_height_at_boundary, proj_first_ceil_height_at_diag, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, first_ceil_side
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == first_ceil_side) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_first_ceil_height_at_boundary, proj_first_ceil_height_at_diag, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_top = proj_first_ceil_height_at_diag;
                }

                // draw step from ceiling
                if(proj_second_ceil_height_at_diag > prev_drawn_top) {
                    draw_lit_fogged_clipped_textured_wall(output, 
                        ((upper_diag_tex&0xF) == SKYBOX_TEX_IDX),
                        get_texture_column(textures[upper_diag_tex&0xF], upper_diag_intersect.diag_wall_u),
                        get_texture_column(decals[upper_diag_tex>>4], upper_diag_intersect.diag_wall_u),
                        screen_x, 
                        proj_first_ceil_height_at_diag, proj_second_ceil_height_at_diag, 
                        first_ceil_height, second_ceil_height, TOP_PEGGED,
                        prev_drawn_top, prev_drawn_bot, upper_diag_intersect.diag_perp_dist, cell_light_level*DIAG_LIGHT_FACTOR, FOG_COL);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_first_ceil_height_at_diag, proj_second_ceil_height_at_diag, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, WALL_SIDE_UPPER_DIAG
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_UPPER_DIAG) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_first_ceil_height_at_diag, proj_second_ceil_height_at_diag, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_top = proj_second_ceil_height_at_diag;
                }


            }

            if(!lower_hits_diag) {
                floor_height = first_floor_height;
                proj_second_floor_height_at_boundary = proj_first_floor_height_at_boundary;
                second_floor_texture = first_floor_texture;

                prev_floor_side = first_floor_side;
            } else {
                floor_height = second_floor_height;

                prev_floor_side = second_floor_side;

                // draw flats from first height at boundary to first height at diag
                
                if(proj_first_floor_height_at_diag < prev_drawn_bot) {
                    draw_lit_fogged_tex_flat(
                        output, 
                        textures[first_floor_texture&0xF],  decals[first_floor_texture>>4],
                        screen_x, 
                        proj_first_floor_height_at_diag, proj_first_floor_height_at_boundary, 
                        lower_diag_intersect.diag_perp_dist, perp_dist, 
                        lower_diag_intersect.mid_flat_u, lower_diag_intersect.mid_flat_v, flat_u, flat_v, 
                        prev_drawn_top, prev_drawn_bot, cell_light_level*FLOOR_LIGHT_FACTOR,
                    FOG_COL);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_first_floor_height_at_diag, proj_first_floor_height_at_boundary, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, first_floor_side
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == first_floor_side) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_first_floor_height_at_diag, proj_first_floor_height_at_boundary, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_bot = proj_first_floor_height_at_diag;
                }

                // draw walls from first height at diag to second height at diag

                if(proj_second_floor_height_at_diag < prev_drawn_bot) {
                    draw_lit_fogged_clipped_textured_wall(output, 
                        ((lower_diag_tex&0xF) == SKYBOX_TEX_IDX),
                        get_texture_column(textures[lower_diag_tex&0xF], lower_diag_intersect.diag_wall_u),
                        get_texture_column(decals[lower_diag_tex>>4], lower_diag_intersect.diag_wall_u),
                        screen_x, 
                        proj_second_floor_height_at_diag, proj_first_floor_height_at_diag, 
                        first_floor_height, second_floor_height, BOTTOM_PEGGED,
                        prev_drawn_top, prev_drawn_bot, lower_diag_intersect.diag_perp_dist, cell_light_level*DIAG_LIGHT_FACTOR, FOG_COL);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_second_floor_height_at_diag, proj_first_floor_height_at_diag, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, WALL_SIDE_LOWER_DIAG
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_LOWER_DIAG) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_second_floor_height_at_diag, proj_first_floor_height_at_diag, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_bot = proj_second_floor_height_at_diag;
                }
            }

        //next_iter:
            prev_floor_height = floor_height;
            prev_ceil_height = ceil_height;
            proj_prev_floor_height_at_prev_dist = proj_second_floor_height_at_boundary;
            proj_prev_ceil_height_at_prev_dist = proj_second_ceil_height_at_boundary;

            prev_perp_dist = perp_dist;
            prev_flat_u = flat_u;
            prev_flat_v = flat_v;
            prev_ceil_texture = second_ceil_texture;
            prev_floor_texture = second_floor_texture;

            prev_cell_light_level = cell_light_level;

            prev_map_idx = map_idx;
        }
    }

    return; 
}

#ifndef PLATFORM_WEB
thread_pool_function(raycast_wrapper, arg_var)
{
    thread_params* tp = (thread_params*)arg_var;
    //printf("end x %i\n", tp->end_x);
    draw_first_person_level_inner(
        tp->output, tp->edit_id_buffer, tp->start_x, tp->end_x,
        tp->frame, tp->this_level, tp->player_x, tp->player_y, tp->player_z,
        tp->player_ang, tp->pitch, 
        tp->editor_mode_enabled, tp->editor_selected_map_idx, tp->editor_selected_side
    );
    InterlockedIncrement64(&tp->finished);

}
#endif

void draw_first_person_level(
    u8* output, edit_wall_id* edit_id_buffer,
    int start_x, int end_x, 
    int frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, int pitch,
    int editor_mode_enabled, int editor_selected_map_idx, wall_side editor_selected_side) {

#ifndef PLATFORM_WEB
    static int tp_created = 0;
    static thread_pool* tp;
    //return;
    if(!tp_created) {
        tp_created = 1;
        tp = thread_pool_create(NUM_THREADS);
    }
#endif

    thread_params parms[NUM_THREADS];
    for(int i = 0; i < NUM_THREADS; i++) {
        parms[i].output = output;
        parms[i].edit_id_buffer = edit_id_buffer;
        parms[i].start_x = i*FP_SCREEN_WIDTH/NUM_THREADS;
        parms[i].end_x = parms[i].start_x + FP_SCREEN_WIDTH/NUM_THREADS;
        parms[i].frame = frame;
        parms[i].this_level = this_level;
        parms[i].player_x = player_x;
        parms[i].player_y = player_y;
        parms[i].player_z = player_z;
        parms[i].player_ang = player_ang;
        parms[i].pitch = pitch;
        parms[i].editor_mode_enabled = editor_mode_enabled;
        parms[i].editor_selected_map_idx = editor_selected_map_idx;
        parms[i].editor_selected_side = editor_selected_side;
        parms[i].finished = 0;
    }


#ifndef PLATFORM_WEB
    for(int i = 0; i < NUM_THREADS; i++) {
        thread_pool_add_work(tp, raycast_wrapper, &parms[i]);
    }
    while(1) {
        int finished = 1;
        for(int i = 0; i < NUM_THREADS; i++) {
            if(parms[i].finished == 0) {
                finished = 0;
                break;
            }
        }
        if(finished) {
            break;
        }
    }
#else
    for(int i = 0; i < NUM_THREADS; i++) {
        thread_params *tp = parms+i;
        draw_first_person_level_inner(
            tp->output, tp->edit_id_buffer, tp->start_x, tp->end_x,
            tp->frame, tp->this_level, tp->player_x, tp->player_y, tp->player_z,
            tp->player_ang, tp->pitch, 
            tp->editor_mode_enabled, tp->editor_selected_map_idx, tp->editor_selected_side
        );
    }
#endif 



        return;
        
}