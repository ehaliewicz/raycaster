//author https://github.com/autergame

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
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


// MAX_HEIGHT/8
#define FOCAL_LENGTH (FP_SCREEN_WIDTH / (2.0f * tanf(1.57f/2.0f)))
#define HEIGHT_SCALE (MAX_WALL_HEIGHT/8)
//(4)
#define HALF_SCREEN_HEIGHT (FP_SCREEN_HEIGHT/2)

#define FOG_COL ((255<<24)|(196<<16)|(162<<8)|(103<<0))

int project_to_screen(float height, float dist, float pitch, float player_z) {
    return (pitch*(float)cur_render_height) + HALF_SCREEN_HEIGHT - (HEIGHT_SCALE * (((height- player_z) * FOCAL_LENGTH / dist) / MAX_WALL_HEIGHT)); 
}

void draw_tint_vline(u8* output, int x, int y0, int y1, int prev_drawn_top, int prev_drawn_bot) {
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot-1); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
        output[(x*FP_SCREEN_HEIGHT+y)*4+0] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+1] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+2] >>= 1;
    }
}

void draw_z_buffered_alpha_tint_vline(u8* output, float* z_buffer, u8 *tex_column, int x, int y0, int y1, float z) {
    float tex_per_pix = 32.0f / (y1-y0);
    for(int y = CLAMP(y0, 0, FP_SCREEN_HEIGHT-1); y < CLAMP(y1, 0, FP_SCREEN_HEIGHT); y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        float pix_z = z_buffer[(x*FP_SCREEN_HEIGHT+y)];
        if(pix_z < z) {
            continue;
        }
        u32 texel = *(u32*)(&tex_column[idx*4]);
        u32 texel_a = ((texel >> 24) & 0xFF);
        int a = texel_a == 255.0f ? 1 : 0;
        if(a == 0) { continue; }
        output[(x*FP_SCREEN_HEIGHT+y)*4+0] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+1] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+2] >>= 1;
    }
}

void draw_solid_vline(u8* output, float* z_buffer, int x, int y0, int y1, float world_z, u32 col, int prev_drawn_top, int prev_drawn_bot) {
    for(int y = CLAMP(y0, prev_drawn_top+1, prev_drawn_bot-1); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
        *(u32*)(&output[(x*FP_SCREEN_HEIGHT+y)*4]) = col;
        z_buffer[(x*FP_SCREEN_HEIGHT+y)] = world_z;
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

// draws a textured flat surface
void draw_lit_fogged_tex_flat(
    u8* output, float* z_buffer, u8* texture, u8* decal, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, u32 fog_col) {
       // return;
    if(texture == textures[SKYBOX_TEX_IDX]) {
        return;
    }

    if(y0 > prev_drawn_bot) {
        return;
    }
    if(y1 < prev_drawn_top) {
        return;
    }    
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    float d_one_over_z = ((1.0f/z1) - inv_z0) / (y1-y0);
    float u_over_z = start_u * inv_z0;
    float v_over_z = start_v * inv_z0;
    float d_u_over_z = ((end_u * inv_z1) - u_over_z) / (y1-y0);
    float d_v_over_z = ((end_v * inv_z1) - v_over_z) / (y1-y0);
    int clipped_y0 = CLAMP(y0, prev_drawn_top, prev_drawn_bot);
    int clipped_y1 = CLAMP(y1, prev_drawn_top, prev_drawn_bot);
    
    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    float cur_inv_z = (inv_z0 + d_one_over_z*(clipped_y0-y0));
    float cur_z = 1.0f / cur_inv_z;
    float cur_u = CLAMP((u_over_z+d_u_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    float cur_v = CLAMP((v_over_z+d_v_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    for(int y = clipped_y0; y < clipped_y1; y++) {
        
        float next_inv_z = (inv_z0 + d_one_over_z*(y+1-y0));
        float next_z = 1.0f / next_inv_z;
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

        //u32 decal_texel = *(u32*)(&decal[idx*4]);
        u32 texel = *(u32*)(&texture[idx*4]);
        //u8 decal_alpha = decal_texel>>24;
        //float decal_a = decal_alpha/255.0f;
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        //u32 decal_r = ((decal_texel >> 16) & 0xFF);
        //u32 decal_g = ((decal_texel >> 8) & 0xFF);
        //u32 decal_b = ((decal_texel >> 0) & 0xFF);
        float r = texel_r;
        //float r = ((decal_r * decal_a) + ((1.0f - decal_a) * texel_r));
        float g = texel_g;
        //float g = ((decal_g * decal_a) + ((1.0f - decal_a) * texel_g));
        float b = texel_b;
        //float b = ((decal_b * decal_a) + ((1.0f - decal_a) * texel_b));
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
        z_buffer[(x*FP_SCREEN_HEIGHT+y)] = cur_z;
        cur_inv_z = next_inv_z;
        cur_z = next_z;
        cur_u = next_u;
        cur_v = next_v;
    }
}


void draw_edit_vline(edit_wall_id* edit_id_buffer, int x, float y0, float y1, int prev_drawn_top, int prev_drawn_bot, int cell_idx, editor_selected_thing side) {
    edit_wall_id id = {.cell_idx = cell_idx, .side = side};
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}
void draw_z_buffered_alpha_edit_vline(edit_wall_id* edit_id_buffer, float* z_buffer, u8 *tex_column, int x, float y0, float y1, float z, int cell_idx, editor_selected_thing side) {
    edit_wall_id id = {.cell_idx = cell_idx, .side = side};
    float tex_per_pix = 32.0f / (y1-y0);
    for(int y = CLAMP(y0, 0, FP_SCREEN_HEIGHT-1); y < CLAMP(y1, 0, FP_SCREEN_HEIGHT-1); y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        u32 texel = *(u32*)(&tex_column[idx*4]);
        u32 texel_a = ((texel >> 24) & 0xFF);
        int a = texel_a == 255.0f ? 1 : 0;
        if(a == 0) { continue; }
        
        float pix_z = z_buffer[(x*FP_SCREEN_HEIGHT+y)];
        if(pix_z < z) {
            continue;
        }
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}


typedef enum {
    TOP_PEGGED,
    BOTTOM_PEGGED,
} pegging_type;


void draw_lit_fogged_textured_z_buffered_sprite(
    u8* output, float* z_buffer,
    u8 *tex_column,
    int x,
    float y0, float y1, 
    pegging_type peg_type,
    float z, float light_factor, u32 fog_col) {
    //return;
    float depth_scale = CLAMP(z / DARK_DIST, 0.0f, 1.0f);
    float inv_depth_scale = (1.0f - depth_scale);
    float mult = inv_depth_scale * light_factor;

    float tex_per_pix = 32.0f / (y1-y0);

    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    u32 scaled_fog_r = (depth_scale * fog_r);
    u32 scaled_fog_g = (depth_scale * fog_g);
    u32 scaled_fog_b = (depth_scale * fog_b);

    for(int y = CLAMP(y0, 0, FP_SCREEN_HEIGHT-1); y < CLAMP(y1, 0, FP_SCREEN_HEIGHT); y++) {
            int dy = y-y0;
            int idx = (int)(dy*tex_per_pix)&31;
            u32 texel = *(u32*)(&tex_column[idx*4]);
            u32 texel_a = ((texel >> 24) & 0xFF);
            float old_z = z_buffer[(x*FP_SCREEN_HEIGHT+y)];
            u32 old_pix = *(u32*)(&output[(x*FP_SCREEN_HEIGHT+y)*4]);
            int use_new_pix = (texel_a == 255.0f && z < old_z);
            //if(old_z < z) {
            //    continue;
            //}

            u32 texel_r = ((texel >> 16) & 0xFF) * mult + scaled_fog_r;
            u32 texel_g = ((texel >> 8) & 0xFF) * mult + scaled_fog_g;
            u32 texel_b = ((texel >> 0) & 0xFF) * mult + scaled_fog_b;
            u32 intr = CLAMP((int)texel_r, 0, 0xFF);
            u32 intg = CLAMP((int)texel_g, 0, 0xFF);
            u32 intb = CLAMP((int)texel_b, 0, 0xFF);
            u32 lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;

            //int a = texel_a == 255.0f ? 1 : 0;
            //if(a == 0) { continue; }
            float new_z = use_new_pix ? z : old_z;
            u32 use_pix = use_new_pix ? lit_texel : old_pix;
            z_buffer[(x*FP_SCREEN_HEIGHT+y)] = new_z;
            //u32 pix_r = ((pix >> 16) & 0xFF);
            //u32 pix_g = ((pix >> 8) & 0xFF);
            //u32 pix_b = ((pix >> 0) & 0xFF);


            //float r = texel_r;//a ? texel_r : pix_r;//((a * texel_r) + ((1 - a) * pix_r));
            //float g = texel_g;//a ? texel_g : pix_g;//((a * texel_g) + ((1 - a) * pix_g));
            //float b = texel_b;//a ? texel_b : pix_b;//((a * texel_b) + ((1 - a) * pix_b));
            //r = ((r * inv_depth_scale) + scaled_fog_r);
            //g = ((g * inv_depth_scale) + scaled_fog_g);
            //b = ((b * inv_depth_scale) + scaled_fog_b);

            //u32 intr = CLAMP((int)r, 0, 0xFF);
            //u32 intg = CLAMP((int)g, 0, 0xFF);
            //u32 intb = CLAMP((int)b, 0, 0xFF);
            *(u32*)(&output[(x*FP_SCREEN_HEIGHT+y)*4]) = use_pix;// 0xFF000000|(intr<<16)|(intg<<8)|intb;
    }
}


void draw_lit_fogged_clipped_textured_wall(
    u8* output, float* z_buffer,
    int draw_skybox,
    u8 *tex_column, u8* decal_column, u8 uses_decal,
    int x,
    float y0, float y1, 
    float world_y0, float world_y1, pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float world_z, float light_factor, u32 fog_col) {

    if(draw_skybox) {
        return;
    }
    if(y0 > prev_drawn_bot) {
        return;
    }
    if(y1 < prev_drawn_top) {
        return;
    }

    int units = fabsf(world_y1 - world_y0);
    float depth_scale = CLAMP(world_z / DARK_DIST, 0.0f, 1.0f);
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
    //float inv_z = 1.0f/z;
    if(uses_decal) {
        for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
            int dy = y-y0;
            int idx = (int)(start_v + dy*tex_per_pix)&31;
            int decal_idx = CLAMP((int)floorf(dy*decal_tex_per_pix), 0, 31);

            u32 decal_texel = *(u32*)(&decal_column[decal_idx*4]);
            u32 texel = *(u32*)(&tex_column[idx*4]);
            u8 decal_alpha = decal_texel>>24;
            float decal_a = decal_alpha == 255.0f ? 1.0f : 0.0f;
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
            z_buffer[(x*FP_SCREEN_HEIGHT+y)] = world_z;
        }
    } else {
        for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
            int dy = y-y0;

            int idx = (int)(start_v + dy*tex_per_pix)&31;

            u32 texel = *(u32*)(&tex_column[idx*4]);
            u32 texel_r = ((texel >> 16) & 0xFF);
            u32 texel_g = ((texel >> 8) & 0xFF);
            u32 texel_b = ((texel >> 0) & 0xFF);
            float r = texel_r;
            float g = texel_g;
            float b = texel_b;
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
            z_buffer[(x*FP_SCREEN_HEIGHT+y)] = world_z;
        }
    }
}


u8* get_texture_column(u8* texture, float wall_u) {
    float u_scaled_to_tex_size = CLAMP(wall_u * 32.0f, 0.0f, 31.0f);
    int int_u = (int)(u_scaled_to_tex_size);
    return &texture[int_u*TEX_SIZE*4];
}

u8* get_heightmap_column(u8* heightmap, float wall_u) {
    float u_scaled_to_tex_size = CLAMP(wall_u * 32.0f, 0.0f, 31.0f);
    int int_u = (int)(u_scaled_to_tex_size);
    return &heightmap[int_u*TEX_SIZE];
}

#define DEGREES_TO_RAD(deg) ((deg)*.0174f)


typedef struct {
    float diag_wall_u;
    float diag_perp_dist;
    float mid_flat_u;
    float mid_flat_v;
} diag_intersect;

float diag_dy[3] = {
    1.0f, // dummy entry for normal walls
    -1.0f, // NE_TO_SW_DIAG
    1.0f,  // NW_TO_SE_DIAG
};

float lerp(float start, float end, float amount)
{
    float result = start + amount*(end - start);

    return result;
}


#define CEIL_LIGHT_FACTOR (0.35f)
#define FLOOR_LIGHT_FACTOR (0.65f)
#define DIAG_LIGHT_FACTOR (0.87)

//int ray_vs_segment(diag_intersect *result, float ray_dir_x, float ray_dir_y, float cam_dir_x, float cam_dir_y, float player_x, float ray_origin_x, float ray_origin_y, int map_x, int map_y,
//    float perp_dist)    


#define EPSILON 1e-6f

int calc_door_hit(diag_intersect *result, float ray_dir_x, float ray_dir_y, float cam_dir_x, float cam_dir_y, float player_x, float player_y, int map_x, int map_y, float perp_dist,
    float door_open_amount) {
    int hits_diag = 0;
    result->mid_flat_u = 0.0f;
    result->mid_flat_v = 0.0f;
    result->diag_perp_dist = perp_dist;

    float diag_ix = 0.0f;
    float diag_iy = 0.0f;

    float p1x = player_x;
    float p1y = player_y;
    float q1x = player_x + ray_dir_x;
    float q1y = player_y + ray_dir_y;
    float p2x = map_x;
    float p2y = map_y;

    // expensive, should do this once per frame not column which intersects it :)
    float angle = door_open_amount * (3.14159 / 2.0f);
    float dir_x = cosf(angle);
    float dir_y = sinf(angle);
    float q2x = p2x + dir_x*1.0f;
    float q2y = p2y + dir_y*1.0f;
    float door_dx = q2x - p2x;
    float door_dy = q2y - p2y;
    float door_len_sq = door_dx * door_dx + door_dy * door_dy;

    
    float a1 = q1y - p1y;
    float b1 = p1x - q1x;
    float c1 = a1 * p1x + b1 * p1y;

    float a2 = q2y - p2y;//-1;
    float b2 = p2x - q2x;//-1;
    float c2 = a2 * p2x + b2 * p2y;

    float determinant = a1 * b2 - a2 * b1;

    diag_ix = (c1 * b2 - c2 * b1) / determinant;
    diag_iy = (a1 * c2 - a2 * c1) / determinant;

    float hit_dx = diag_ix - p2x;
    float hit_dy = diag_iy - p2y;

    float u = (hit_dx * door_dx + hit_dy * door_dy) / door_len_sq;
    //float lx = fabsf(diag_ix - map_x);
    result->diag_wall_u = u;


    result->mid_flat_u = diag_ix - floorf(diag_ix);
    result->mid_flat_v = diag_iy - floorf(diag_iy);


    if (u >= -EPSILON && u <= 1.0f + EPSILON) {
    // Treat as a hit
    // Clamp u if needed for further calculations
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;

        hits_diag = 1;
        float dx = diag_ix-player_x;
        float dy = diag_iy-player_y;
        result->diag_perp_dist = dx*cam_dir_x + dy*cam_dir_y;

    }

    return hits_diag;
}

int calc_diag_hit(diag_intersect *result, float ray_dir_x, float ray_dir_y, float cam_dir_x, float cam_dir_y, float player_x, float player_y, int map_x, int map_y, float perp_dist, cell_types cell_type) {
    int hits_diag = 0;
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
        hits_diag = 1;
        float dx = diag_ix-player_x;
        float dy = diag_iy-player_y;
        result->diag_perp_dist = dx*cam_dir_x + dy*cam_dir_y;
    }
    return hits_diag;
                
}

float light_level_mults[4] = {1.0f, 0.25f, 1.5f, 1.5f};

typedef struct {
    u8* output;
    edit_wall_id* edit_id_buffer;
    float* z_buffer;
    int start_x;
    int end_x;
    int flash_frame; 
    level* this_level;
    float player_x; float player_y; float player_z; float player_ang; float pitch;
    int editor_mode_enabled;
    int editor_selected_map_idx;
    editor_selected_thing editor_selected_thg;
    volatile s64 finished;
    u8 *visited_cell_bitmap;
} thread_params;

void draw_normal_cell_first_step() {

}
void draw_normal_cell_top() {

}

void draw_slope_first_step() {

}

void draw_slope_top() {

}

void draw_diagonal_first_step() {

}

void draw_diagonal_top() {

}

void draw_diagonal_second_step() {

}

void set_byte_in_bitmap(u8* bitmap, int bit_idx) {
    int byte_idx = bit_idx>>3;
    int bit = bit_idx&0b111;
    bitmap[byte_idx] |= (1 << bit);

}

const int MAX_STEPS = 64;

typedef struct {
    // screen y coordinates, used to calculate texture v coords in the innermost loop
    float unclipped_y0, unclipped_y1;
    float clipped_y0, clipped_y1; 
    int sprite_idx; // which sprite?
    float z; // the depth of this column, written directly to framebuffer without testing
    float u; // the texture column, 0 <= u < 1
} sprite_cache_entry;

/*
    Each thread gets MAX_STEPS*2 entry cache for sprites
    while raycasting, whenever we take a step into a new cell,
    check if the entered side of the new cell, and the exited side of the new cell have sprites assigned.

    If any sprites exist, they are pushed into the cache in a front to back order.
    Once raycasting is complete, these sprite columns are drawn in reverse, back to front order.

    The pixels are blended with previous pixels, if necessary, or overwrite the previous pixels otherwise.
    No depth testing is performed, it is not necessary, but each pixel that writes a color also writes depth.

    NOTE - currently we only support one sprite per cell, so technically we only need MAX_STEPS, without the *2
*/
sprite_cache_entry per_thread_sprite_cache[NUM_THREADS][MAX_STEPS*2];

void draw_first_person_level_inner(
    u8* output, edit_wall_id* edit_id_buffer, float* z_buffer,
    int start_x, int end_x, 
    int flash_frame, 
    level* this_level, 
    float ray_origin_x, float ray_origin_y, float ray_origin_z, float cam_ang, float pitch,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side,
    u8* visited_cell_bitmap, sprite_cache_entry* sprite_cache
) {
    


    //u8* cur_level = this_level;
    u8* cur_level_floor = this_level->floor;
    u8* cur_level_ceil = this_level->ceil;
    u8* cur_level_upper_floor = this_level->upper_floor;
    u8* cur_level_upper_ceil = this_level->upper_ceil;

    float start_cam_dir_x = cosf(cam_ang);
    float start_cam_dir_y = sinf(cam_ang);

    
    for(int ix = start_x; ix < end_x; ix++) {

        float cam_dir_x = start_cam_dir_x;
        float cam_dir_y = start_cam_dir_y;
        int screen_x = (FP_SCREEN_WIDTH-1)-ix;
        float cam_x = 2.0f * (ix) / (float)FP_SCREEN_WIDTH - 1.0f; // -1 to 1
        float ray_dir_x = cosf(cam_ang) + cam_x * -sinf(cam_ang);
        float ray_dir_y = sinf(cam_ang) + cam_x * cosf(cam_ang);



        /* DRAW SKYBOX */
        {
            float ray_ang = atan2f(ray_dir_y, ray_dir_x);
            if(ray_ang < 0.0f) {
                ray_ang += 6.28f;
            }

            u8* skybox = textures[SKYBOX_TEX_IDX];

            float u = 1024.0f* (0.5f + ray_ang / (2.0f * 3.14159));
            float flt_u = (u+skybox_u_offset);
            float subtex_u = flt_u - floorf(flt_u);
            int int_u = ((int)(u+skybox_u_offset))&(SKYBOX_TEX_WIDTH-1);
            for(int y = 0; y < FP_SCREEN_HEIGHT-1; y++) {
                int v = (SKYBOX_TEX_HEIGHT/4+(int)(SKYBOX_V_PER_PIX*(y+(-pitch*(float)FP_SCREEN_HEIGHT))))&(SKYBOX_TEX_HEIGHT-1);
                u32 texell = *(u32*)(&skybox[(int_u*SKYBOX_TEX_HEIGHT+v)*4]);

                *(u32*)(&output[(screen_x*FP_SCREEN_HEIGHT+y)*4]) = texell;
            }
        }
        int num_sprites_hit = 0;
        
        int rem_steps = MAX_STEPS;

        float perp_dist = NEAR_PLANE_DIST;

        int prev_drawn_top = 0;
        int prev_drawn_bot = FP_SCREEN_HEIGHT;

        int start_map_x = floorf(ray_origin_x);
        int start_map_y = floorf(ray_origin_y);
        
        // length of ray from one x/y side to the next x/y side
        float delta_dist_x = fabsf(1.0f / ray_dir_x);
        float delta_dist_y = fabsf(1.0f / ray_dir_y);

        int map_x = floorf(ray_origin_x);
        int map_y = floorf(ray_origin_y);
        

        float def_exit_u = (ray_dir_x >= 0) ? 1.0f : 0.0f;
        float def_exit_v = (ray_dir_y >= 0) ? 1.0f : 0.0f;
        float def_start_u = (ray_dir_x >= 0) ? 0.0f : 1.0f;
        float def_start_v = (ray_dir_y >= 0) ? 0.0f : 1.0f;
        
        float flat_u = ray_origin_x - floorf(ray_origin_x);
        float flat_v = ray_origin_y - floorf(ray_origin_y);           // the u,v position of where we enter the next cell (which we use on the next iteration)

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
            if(!in_start_cell) {
                // PROJECT SPRITE?
                sprite_info spr = this_level->sprite_index[map_idx];
                int spr_idx = spr.index;
                sprite_cache[num_sprites_hit].
                if(spr_idx != EMPTY_SPRITE_INDEX) {
                    if(side == HORIZONTAL_SIDE) {
                        if(ray_dir_x > 0) {
                            // west side
                            if(spr.loc == W)
                        } else {
                            if()
                        }
                    }
                }
            }

            set_byte_in_bitmap(visited_cell_bitmap, map_idx);

            int selected_cur_map_idx = editor_selected_map_idx == map_idx;
            cell_types upper_cell_type = this_level->upper_cell_types[map_idx];
            cell_types lower_cell_type = this_level->lower_cell_types[map_idx];


            int proj_zero_height = project_to_screen(0, perp_dist, pitch, ray_origin_z);
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

            if(side == VERTICAL_SIDE) {
                wall_u = hit_y - floorf(hit_y);
                flat_u = in_start_cell ? (ray_origin_x - floorf(ray_origin_x)) : def_start_u;
                flat_v = in_start_cell ? (ray_origin_y - floorf(ray_origin_y)) : wall_u;

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
                flat_u = in_start_cell ? (ray_origin_x - floorf(ray_origin_x)) : wall_u;
                flat_v = in_start_cell ? (ray_origin_y - floorf(ray_origin_y)) : def_start_v;
                if(ray_dir_y < 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_SOUTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_SOUTH;
                    wall_u = 1.0f - wall_u;
                    upper_wall_tex = this_level->ustex[map_idx];
                    lower_wall_tex = this_level->lstex[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_NORTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_NORTH;
                    upper_wall_tex = this_level->untex[map_idx];
                    lower_wall_tex = this_level->lntex[map_idx];
                }
            }
            if(next_side == VERTICAL_SIDE) {
                exit_flat_u = def_exit_u;
                exit_flat_v = next_hit_y - floorf(next_hit_y);

            } else {
                exit_flat_u = next_hit_x - floorf(next_hit_x);
                exit_flat_v = def_exit_v;

            }

            
            u8 lower_diag_wall_tex = this_level->ldtex[map_idx];
            u8 floor_texture = this_level->ftex[map_idx];
            u8 upper_floor_texture = this_level->uftex[map_idx];
            u8 upper_diag_wall_tex = this_level->udtex[map_idx];
            u8 ceil_texture = this_level->ctex[map_idx];
            u8 upper_ceil_texture = this_level->uctex[map_idx];

            float cell_light_level = light_level_mults[this_level->light[map_idx]];

            {
                int first_floor_height = floor_height;
                int second_floor_height = upper_floor_height;
                u8 first_floor_texture = floor_texture;
                u8 second_floor_texture = upper_floor_texture;
                editor_selected_thing first_floor_side = WALL_SIDE_TOP;
                editor_selected_thing second_floor_side = WALL_SIDE_UPPER_TOP;

                int first_ceil_height = ceil_height;
                int second_ceil_height = upper_ceil_height;
                u8 first_ceil_texture = ceil_texture;
                u8 second_ceil_texture = upper_ceil_texture;
                editor_selected_thing first_ceil_side = WALL_SIDE_BOTTOM;
                editor_selected_thing second_ceil_side = WALL_SIDE_UPPER_BOTTOM;

                

                // miscellaneous stuff for diagonal draw order sorting 
                float subx = ray_origin_x - floorf(ray_origin_x);
                float suby = ray_origin_y - floorf(ray_origin_y);
                int in_top_right = (subx >= suby);
                //int in_bottom_left = !in_top_right;
                int in_top_left = (subx < (1.0f - suby));
                //int in_bottom_right = !in_top_left;

                int enters_right_side = (step_x == -1) && (side == VERTICAL_SIDE);
                int enters_left_side = (step_x == 1) && (side == VERTICAL_SIDE);
                //int enters_bot_side = (step_y == -1) && (side == HORIZONTAL_SIDE);
                int enters_top_side = (step_y == 1) && (side == HORIZONTAL_SIDE);
                //int in_bottom_half = 
                // floor

                if(lower_cell_type == NE_TO_SW_DIAG || lower_cell_type ==  NW_TO_SE_DIAG) { //} || lower_cell_type == DOOR_Y) {
                    int draw_upper_first;
                    if(lower_cell_type == NE_TO_SW_DIAG) {
                        draw_upper_first = (in_start_cell ? in_top_left : (enters_left_side || enters_top_side));
                    } else { // NW_TO_SE_DIAG
                        draw_upper_first = (in_start_cell ? in_top_right : (enters_right_side || enters_top_side));
                    }

                    if(draw_upper_first) {
                        first_floor_height = upper_floor_height;
                        second_floor_height = floor_height;
                        first_floor_texture = upper_floor_texture;
                        second_floor_texture = floor_texture;
                        first_floor_side = WALL_SIDE_UPPER_TOP;
                        second_floor_side = WALL_SIDE_TOP;
                    }
                }

                if(upper_cell_type == NE_TO_SW_DIAG || upper_cell_type ==  NW_TO_SE_DIAG) {
                    // handle diagonal stuff
                    int draw_upper_first;
                    if(upper_cell_type == NE_TO_SW_DIAG) {
                        draw_upper_first = (in_start_cell ? in_top_left : (enters_left_side || enters_top_side));
                    } else { // NW_TO_SE_DIAG
                        draw_upper_first = (in_start_cell ? in_top_right : (enters_right_side || enters_top_side));
                    }

                    if(draw_upper_first) {
                        first_ceil_height = upper_ceil_height;
                        second_ceil_height = ceil_height;
                        first_ceil_texture = upper_ceil_texture;
                        second_ceil_texture = ceil_texture;
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
                int proj_ceil_first_step_height = project_to_screen(first_ceil_height, perp_dist, pitch, ray_origin_z);

                if(!in_start_cell && !lower_step_slope && proj_floor_first_step_height < prev_drawn_bot) {
                    draw_lit_fogged_clipped_textured_wall(
                        output, z_buffer,
                        ((lower_wall_tex&0xF) == SKYBOX_TEX_IDX),
                        get_texture_column(textures[lower_wall_tex&0xF], wall_u),
                        get_texture_column(decals[lower_wall_tex>>4], wall_u), 
                        ((lower_wall_tex>>4) != BLANK_DECAL_IDX),
                        screen_x, proj_floor_first_step_height, proj_zero_height,
                        first_floor_height, 0, BOTTOM_PEGGED,
                        prev_drawn_top, prev_drawn_bot, perp_dist, light_factor * cell_light_level, 
                        FOG_COL
                    );

                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_floor_first_step_height, proj_zero_height, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, lower_intersect_wall_side
                        );
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
                    float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - floorf(ray_origin_y)) : hit_y - floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - floorf(ray_origin_x)) : hit_x - floorf(hit_x);
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

                    
                    if(!in_start_cell && proj_slope_start_height < prev_drawn_bot) {      

                        // draw wall up to start of slope    
                        draw_lit_fogged_clipped_textured_wall(
                            output, z_buffer,
                            ((lower_wall_tex&0xF) == SKYBOX_TEX_IDX),
                            get_texture_column(textures[lower_wall_tex&0xF], wall_u),
                            get_texture_column(decals[lower_wall_tex>>4], wall_u), ((lower_wall_tex>>4) != BLANK_DECAL_IDX),
                            screen_x, proj_slope_start_height, proj_zero_height,
                            slope_start_height, 0, BOTTOM_PEGGED,
                            prev_drawn_top, prev_drawn_bot, perp_dist, light_factor * cell_light_level, 
                            FOG_COL
                        );
                        
                        if(editor_mode_enabled) {
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_slope_start_height, proj_zero_height, 
                                prev_drawn_top, prev_drawn_bot,
                                map_idx, lower_intersect_wall_side
                            );
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
                    draw_lit_fogged_clipped_textured_wall(
                        output, z_buffer,
                        ((upper_wall_tex&0xF) == SKYBOX_TEX_IDX),
                        get_texture_column(textures[upper_wall_tex&0xF], wall_u),
                        get_texture_column(decals[upper_wall_tex>>4], wall_u),((lower_wall_tex>>4) != BLANK_DECAL_IDX),
                        screen_x, proj_max_height, proj_ceil_first_step_height,
                        MAX_WALL_HEIGHT, first_ceil_height, TOP_PEGGED,
                        prev_drawn_top, prev_drawn_bot, perp_dist, light_factor * cell_light_level, 
                        FOG_COL
                    );

                    
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            edit_id_buffer, screen_x,
                            proj_max_height, proj_ceil_first_step_height, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, upper_intersect_wall_side
                        );
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
                     float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - floorf(ray_origin_y)) : hit_y - floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - floorf(ray_origin_x)) : hit_x - floorf(hit_x);
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


                    if(proj_slope_start_height > prev_drawn_top) {      
                        // draw wall up to start of slope    
                        draw_lit_fogged_clipped_textured_wall(
                            output, z_buffer,
                            ((upper_wall_tex&0xF) == SKYBOX_TEX_IDX),
                            get_texture_column(textures[upper_wall_tex&0xF], wall_u),
                            get_texture_column(decals[upper_wall_tex>>4], wall_u),((lower_wall_tex>>4) != BLANK_DECAL_IDX),
                            screen_x, 
                            proj_max_height, proj_slope_start_height,
                            MAX_WALL_HEIGHT, slope_start_height, TOP_PEGGED,
                            prev_drawn_top, prev_drawn_bot, perp_dist, light_factor * cell_light_level, 
                            FOG_COL
                        );
                        
                        if(editor_mode_enabled) {
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_max_height, proj_slope_start_height, 
                                prev_drawn_top, prev_drawn_bot,
                                map_idx, upper_intersect_wall_side
                            );
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
                    float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - floorf(ray_origin_y)) : hit_y - floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - floorf(ray_origin_x)) : hit_x - floorf(hit_x);
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
                            output, z_buffer, textures[upper_floor_texture&0xF], decals[upper_floor_texture>>4],
                            screen_x, proj_slope_end_height, proj_slope_start_height, 
                            next_perp_dist, perp_dist, 
                            exit_flat_u, exit_flat_v, flat_u, flat_v,
                            prev_drawn_top, prev_drawn_bot, FLOOR_LIGHT_FACTOR, FOG_COL
                        );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_slope_end_height, proj_slope_start_height, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, WALL_SIDE_UPPER_TOP
                                );
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
                            output, z_buffer, textures[first_floor_texture&0xF], decals[first_floor_texture>>4],
                            screen_x, proj_step_next_height, proj_floor_first_step_height, next_perp_dist, perp_dist,
                            exit_flat_u, exit_flat_v, flat_u, flat_v, prev_drawn_top, prev_drawn_bot,  FLOOR_LIGHT_FACTOR * cell_light_level,
                            FOG_COL
                        );
                        if(editor_mode_enabled) {
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_step_next_height, proj_floor_first_step_height, 
                                prev_drawn_top, prev_drawn_bot,
                                map_idx, WALL_SIDE_TOP
                            );
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
                } else if(lower_cell_type == NW_TO_SE_DIAG || lower_cell_type == NE_TO_SW_DIAG || lower_cell_type == DOOR_Y) {
                    diag_intersect lower_diag_intersect;
                    int lower_hits_diag;
                    float door_open_amount = this_level->parameter[map_idx]/255.0f;
                    if(lower_cell_type == DOOR_Y) { 
                        lower_hits_diag = calc_door_hit(&lower_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, ray_origin_x, ray_origin_y, map_x, map_y, perp_dist, door_open_amount);
                        // use flat exit UV coords for floor/ceiling next to door, when the door is fully open
                        if(lower_cell_type == DOOR_Y && door_open_amount >= 1.0f-EPSILON) {
                            lower_diag_intersect.mid_flat_u = exit_flat_u;
                            lower_diag_intersect.mid_flat_v = exit_flat_v;
                        }
                    } else {
                        lower_hits_diag =calc_diag_hit(&lower_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, ray_origin_x, ray_origin_y, map_x, map_y, perp_dist, lower_cell_type);
                    }
                    if(lower_hits_diag && lower_diag_intersect.diag_perp_dist > NEAR_PLANE_DIST) {                        

                        // draw first floor
                        int proj_first_height_diag = project_to_screen(first_floor_height, lower_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_first_height_diag < prev_drawn_bot) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_floor_texture&0xF], decals[first_floor_texture>>4],
                                screen_x, proj_first_height_diag, proj_floor_first_step_height, lower_diag_intersect.diag_perp_dist, perp_dist,
                                lower_diag_intersect.mid_flat_u, lower_diag_intersect.mid_flat_v, flat_u, flat_v, prev_drawn_top, prev_drawn_bot,  FLOOR_LIGHT_FACTOR * cell_light_level,
                                FOG_COL
                            );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_first_height_diag, proj_floor_first_step_height, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, first_floor_side
                                );
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
                        int proj_zero_height_diag = project_to_screen(0, lower_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_second_height_diag < prev_drawn_bot) {
                            draw_lit_fogged_clipped_textured_wall(
                                output, z_buffer,
                                ((lower_diag_wall_tex&0xF) == SKYBOX_TEX_IDX),
                                get_texture_column(textures[lower_diag_wall_tex&0xF], lower_diag_intersect.diag_wall_u),
                                get_texture_column(decals[lower_diag_wall_tex>>4], lower_diag_intersect.diag_wall_u),
                                ((lower_diag_wall_tex>>4) != BLANK_DECAL_IDX),
                                screen_x, proj_second_height_diag, proj_zero_height_diag,
                                second_floor_height, 0, BOTTOM_PEGGED,
                                prev_drawn_top, prev_drawn_bot, lower_diag_intersect.diag_perp_dist, DIAG_LIGHT_FACTOR * cell_light_level, 
                                FOG_COL
                            );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_second_height_diag, proj_zero_height_diag, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, WALL_SIDE_LOWER_DIAG
                                );
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
                        if(lower_cell_type != DOOR_Y && proj_second_height_next < prev_drawn_bot) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[second_floor_texture&0xF], decals[second_floor_texture>>4],
                                screen_x, proj_second_height_next, proj_second_height_diag, next_perp_dist, lower_diag_intersect.diag_perp_dist,
                                exit_flat_u, exit_flat_v, lower_diag_intersect.mid_flat_u, lower_diag_intersect.mid_flat_v, prev_drawn_top, prev_drawn_bot,  
                                FLOOR_LIGHT_FACTOR * cell_light_level,
                                FOG_COL
                            );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_second_height_next, proj_second_height_diag, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, second_floor_side
                                );
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


                    } else {

                        // just draw regular floor
                        int proj_step_next_height = project_to_screen(first_floor_height, next_perp_dist, pitch, ray_origin_z);
                        if(proj_step_next_height < prev_drawn_bot) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_floor_texture&0xF], decals[first_floor_texture>>4],
                                screen_x, proj_step_next_height, proj_floor_first_step_height, next_perp_dist, perp_dist,
                                exit_flat_u, exit_flat_v, flat_u, flat_v, prev_drawn_top, prev_drawn_bot,  FLOOR_LIGHT_FACTOR * cell_light_level,
                                FOG_COL
                            );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_step_next_height, proj_floor_first_step_height, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, first_floor_side
                                );
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
                    float y_exit = next_map_y > map_y ? 1.0f : next_map_y < map_y ? 0.0f : next_hit_y-floorf(next_hit_y);
                    float y_start = in_start_cell ? (ray_origin_y - floorf(ray_origin_y)) : hit_y - floorf(hit_y);
                    if(!in_start_cell && side == HORIZONTAL_SIDE) {
                        if(step_y == -1) {
                            y_start = 1.0f;
                        } else {
                            y_start = 0.0f;
                        }
                    }
                    float x_exit = next_map_x > map_x ? 1.0f : next_map_x < map_x ? 0.0f : next_hit_x-floorf(next_hit_x);
                    float x_start = in_start_cell ? (ray_origin_x - floorf(ray_origin_x)) : hit_x - floorf(hit_x);
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
                            output, z_buffer, textures[upper_ceil_texture&0xF], decals[upper_ceil_texture>>4],
                            screen_x, proj_slope_start_height, proj_slope_end_height, 
                            perp_dist, next_perp_dist, 
                            flat_u, flat_v, exit_flat_u, exit_flat_v, 
                            prev_drawn_top, prev_drawn_bot, CEIL_LIGHT_FACTOR, FOG_COL
                        );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_slope_start_height, proj_slope_end_height, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, WALL_SIDE_UPPER_BOTTOM
                                );
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
                            output, z_buffer, textures[first_ceil_texture&0xF], decals[first_ceil_texture>>4],
                            screen_x, proj_ceil_first_step_height, proj_step_next_height, perp_dist, next_perp_dist,
                            flat_u, flat_v, exit_flat_u, exit_flat_v, prev_drawn_top, prev_drawn_bot,  CEIL_LIGHT_FACTOR * cell_light_level,
                            FOG_COL
                        );
                        if(editor_mode_enabled) {
                            draw_edit_vline(
                                edit_id_buffer, screen_x,
                                proj_ceil_first_step_height, proj_step_next_height, 
                                prev_drawn_top, prev_drawn_bot,
                                map_idx, WALL_SIDE_BOTTOM
                            );
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
                } else if(upper_cell_type == NW_TO_SE_DIAG || upper_cell_type == NE_TO_SW_DIAG) {
                    diag_intersect upper_diag_intersect;
                    int upper_hits_diag = calc_diag_hit(&upper_diag_intersect, ray_dir_x, ray_dir_y, cam_dir_x, cam_dir_y, ray_origin_x, ray_origin_y, map_x, map_y, perp_dist, upper_cell_type);    
                    if(upper_hits_diag && upper_diag_intersect.diag_perp_dist > NEAR_PLANE_DIST) {                        

                        // draw first ceil
                        int proj_first_height_diag = project_to_screen(first_ceil_height, upper_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_first_height_diag > prev_drawn_top) {
                            draw_lit_fogged_tex_flat(
                                output, z_buffer, textures[first_ceil_texture&0xF], decals[first_ceil_texture>>4],
                                screen_x, proj_ceil_first_step_height, proj_first_height_diag, perp_dist, upper_diag_intersect.diag_perp_dist,
                                flat_u, flat_v,  upper_diag_intersect.mid_flat_u, upper_diag_intersect.mid_flat_v, prev_drawn_top, prev_drawn_bot, CEIL_LIGHT_FACTOR * cell_light_level,
                                FOG_COL
                            );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_ceil_first_step_height, proj_first_height_diag, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, first_ceil_side
                                );
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
                        int proj_max_height_diag = project_to_screen(MAX_WALL_HEIGHT, upper_diag_intersect.diag_perp_dist, pitch, ray_origin_z);
                        if(proj_second_height_diag > prev_drawn_top) {
                            draw_lit_fogged_clipped_textured_wall(
                                output, z_buffer,
                                ((upper_diag_wall_tex&0xF) == SKYBOX_TEX_IDX),
                                get_texture_column(textures[upper_diag_wall_tex&0xF], upper_diag_intersect.diag_wall_u),
                                get_texture_column(decals[upper_diag_wall_tex>>4], upper_diag_intersect.diag_wall_u),
                                ((upper_diag_wall_tex>>4) != BLANK_DECAL_IDX),
                                screen_x, proj_max_height_diag, proj_second_height_diag,
                                MAX_WALL_HEIGHT, second_ceil_height, TOP_PEGGED,
                                prev_drawn_top, prev_drawn_bot, upper_diag_intersect.diag_perp_dist, DIAG_LIGHT_FACTOR * cell_light_level, 
                                FOG_COL
                            );

                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_max_height, proj_second_height_diag, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, WALL_SIDE_UPPER_DIAG
                                );
                                if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_UPPER_DIAG) {
                                    draw_tint_vline(
                                        output, screen_x, 
                                        proj_max_height, proj_second_height_diag, 
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
                                output, z_buffer, textures[second_ceil_texture&0xF], decals[second_ceil_texture>>4],
                                screen_x, proj_second_height_diag, proj_second_height_next, upper_diag_intersect.diag_perp_dist, next_perp_dist, 
                                upper_diag_intersect.mid_flat_u, upper_diag_intersect.mid_flat_v, exit_flat_u, exit_flat_v, prev_drawn_top, prev_drawn_bot,  
                                CEIL_LIGHT_FACTOR * cell_light_level,
                                FOG_COL
                            );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_second_height_diag, proj_second_height_next, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, second_ceil_side
                                );
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
                                output, z_buffer, textures[first_ceil_texture&0xF], decals[first_ceil_texture>>4],
                                screen_x, proj_ceil_first_step_height, proj_step_next_height, perp_dist, next_perp_dist,
                                flat_u, flat_v, exit_flat_u, exit_flat_v, prev_drawn_top, prev_drawn_bot, CEIL_LIGHT_FACTOR * cell_light_level,
                                FOG_COL
                            );
                            if(editor_mode_enabled) {
                                draw_edit_vline(
                                    edit_id_buffer, screen_x,
                                    proj_ceil_first_step_height, proj_step_next_height, 
                                    prev_drawn_top, prev_drawn_bot,
                                    map_idx, first_ceil_side
                                );
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


        }
    

    }

    

    return; 
}

typedef struct {
    float x0, x1;
    float left_y0, left_y1; // top bottom
    float right_y0, right_y1; // top bottom
    float z0, z1; // z between left and right sides
    float z_center;
    int map_idx;
    int fixed_rotation;
    int image_idx;
} transformed_sprite;
// tranformed into camera space: x, y0, y1, z (y0 and y1 are not actually transformed)
transformed_sprite transformed_sprites[MAP_SIZE*MAP_SIZE];

int num_trans_sprites = 0;


void draw_transformed_sprites(u8* output, edit_wall_id* edit_id_buffer, float* z_buffer, int flash_frame, float camera_z, int start_x, int end_x,
                              int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_thg
) {
    for(int spr = 0; spr < num_trans_sprites; spr++) {
        transformed_sprite sprite = transformed_sprites[spr];

        float screen_x0 = sprite.x0;
        float screen_x1 = sprite.x1;
        float screen_left_y0 = sprite.left_y0;
        float screen_left_y1 = sprite.left_y1;
        float screen_right_y0 = sprite.right_y0;
        float screen_right_y1 = sprite.right_y1;
        float rot_z0 = sprite.z0;
        float rot_z1 = sprite.z1;
        float center_z = sprite.z_center;
        int is_fixed_rotation = transformed_sprites[spr].fixed_rotation;
        int spr_idx = sprite.image_idx;

        if(screen_x0 > screen_x1) {
            float tmp = screen_x0;
            screen_x0 = screen_x1;
            screen_x1 = tmp;

            tmp = screen_left_y0;
            screen_left_y0 = screen_right_y0;
            screen_right_y0 = tmp;

            tmp = screen_left_y1;
            screen_left_y1 = screen_right_y1;
            screen_right_y1 = tmp;

            tmp = MIN(rot_z0, rot_z1);
            rot_z1 = MAX(rot_z0, rot_z1);
            rot_z0 = tmp;
            //tmp = rot_z0;
            //rot_z0 = rot_z1;
            //rot_z1 = tmp;
        }
        int map_idx = sprite.map_idx;
        float inv_z0 = 1.0f / rot_z0;
        float inv_z1 = 1.0f / rot_z1;
        float dinvz = (inv_z1 - inv_z0) / (screen_x1-screen_x0);

        float u_over_z = 0.0f * inv_z0;
        float d_u_over_z = ((1.0f * inv_z1) - u_over_z) / (screen_x1-screen_x0);

        float top_dy = (screen_right_y0 - screen_left_y0)/(screen_x1-screen_x0);
        float bot_dy = (screen_right_y1 - screen_left_y1)/(screen_x1-screen_x0);
        int clipped_sprite_left_x = MAX(start_x, (int)screen_x0);
        int clipped_sprite_right_x = MIN(end_x-1, (int)screen_x1);
        int min_z = MIN(rot_z0, rot_z1);

        for(int x = clipped_sprite_left_x; x <= clipped_sprite_right_x; x++) {
            float screen_y0 = screen_left_y0 + top_dy*(x-screen_x0);
            float screen_y1 = screen_left_y1 + bot_dy*(x-screen_x0);
            if(screen_y1 > 0 && screen_y0 < FP_SCREEN_HEIGHT-1) {
                if(!is_fixed_rotation) {
                    
                    draw_lit_fogged_textured_z_buffered_sprite(
                        output, z_buffer,
                        get_texture_column(sprites[spr_idx], ((float)x-screen_x0)/(screen_x1-screen_x0)),
                        x, screen_y0, screen_y1,
                        TOP_PEGGED, center_z, 1.0f, FOG_COL
                    );
                    if(editor_mode_enabled) {
                        draw_z_buffered_alpha_edit_vline(
                            edit_id_buffer, z_buffer, 
                            get_texture_column(sprites[spr_idx], ((float)x-screen_x0)/(screen_x1-screen_x0)),
                            x, screen_y0, screen_y1, center_z, map_idx, CELL_SPRITE
                        );
                        if(flash_frame && editor_selected_map_idx == map_idx && editor_selected_thg == CELL_SPRITE) {
                            draw_z_buffered_alpha_tint_vline(
                                output, z_buffer, 
                                get_texture_column(sprites[spr_idx], ((float)x-screen_x0)/(screen_x1-screen_x0)),
                                x, screen_y0, screen_y1, center_z
                            );
                        
                        }
                    }
                    
                } else {
                    float inv_z = inv_z1 + (dinvz*(x-screen_x0));
                    float z = 1.0f / inv_z;
                    if(z < NEAR_PLANE_DIST) { continue; }
                    float u = (u_over_z + (d_u_over_z*(x-screen_x0))) * z;
                    draw_lit_fogged_textured_z_buffered_sprite(
                        output, z_buffer,
                        get_texture_column(sprites[spr_idx], u),
                        x, screen_y0, screen_y1,
                        TOP_PEGGED, min_z, 1.0f, FOG_COL
                    );
                    if(editor_mode_enabled) {
                        draw_z_buffered_alpha_edit_vline(
                            edit_id_buffer, z_buffer, 
                            get_texture_column(sprites[spr_idx], u),
                            x, screen_y0, screen_y1, min_z, map_idx, CELL_SPRITE
                        );
                        if(flash_frame && editor_selected_map_idx == map_idx && editor_selected_thg == CELL_SPRITE) {
                            draw_z_buffered_alpha_tint_vline(
                                output, z_buffer, 
                                get_texture_column(sprites[spr_idx], u),
                                x, screen_y0, screen_y1, min_z
                            );
                        
                        }
                    }
                }
            }
        }
    }
}


#ifndef PLATFORM_WEB
thread_pool_function(raycast_wrapper, arg_var)
{
    thread_params* tp = (thread_params*)arg_var;
    draw_first_person_level_inner(
        tp->output, tp->edit_id_buffer, tp->z_buffer, 
        tp->start_x, tp->end_x,
        tp->flash_frame, tp->this_level, tp->player_x, tp->player_y, tp->player_z,
        tp->player_ang, tp->pitch, 
        tp->editor_mode_enabled, tp->editor_selected_map_idx, tp->editor_selected_thg,
        tp->visited_cell_bitmap
    );
    InterlockedIncrement64(&tp->finished);
}
thread_pool_function(draw_sprites_wrapper, arg_var)
{
    thread_params* tp = (thread_params*)arg_var;
    draw_transformed_sprites(tp->output, tp->edit_id_buffer, tp->z_buffer, tp->flash_frame, tp->player_z, tp->start_x, tp->end_x, tp->editor_mode_enabled, tp->editor_selected_map_idx, tp->editor_selected_thg);
    InterlockedIncrement64(&tp->finished);
}
#endif

u8 visited_cells[NUM_THREADS][MAP_SIZE*MAP_SIZE/8];

void draw_first_person_level(
    u8* output, edit_wall_id* edit_id_buffer, float* z_buffer,
    int start_x, int end_x, 
    int flash_frame, 
    level* this_level, 
    float player_x, float player_y, float player_z, float player_ang, float pitch,
    int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side) {





#ifndef PLATFORM_WEB
    static int tp_created = 0;
    static thread_pool* tp;
    //return;
    if(!tp_created) {
        tp_created = 1;
        tp = thread_pool_create(NUM_THREADS);
    }
#endif

    memset(visited_cells, 0, sizeof(NUM_THREADS*MAP_SIZE*MAP_SIZE/8));
    thread_params parms[NUM_THREADS];
    for(int i = 0; i < NUM_THREADS; i++) {
        parms[i].output = output;
        parms[i].edit_id_buffer = edit_id_buffer;
        parms[i].z_buffer = z_buffer,
        parms[i].start_x = (i == 0) ? 0 : parms[i-1].end_x, //i*FP_SCREEN_WIDTH/NUM_THREADS;
        parms[i].end_x = parms[i].start_x + FP_SCREEN_WIDTH/NUM_THREADS;
        parms[i].flash_frame = flash_frame;
        parms[i].this_level = this_level;
        parms[i].player_x = player_x;
        parms[i].player_y = player_y;
        parms[i].player_z = player_z;
        parms[i].player_ang = player_ang;
        parms[i].pitch = pitch;
        parms[i].editor_mode_enabled = editor_mode_enabled;
        parms[i].editor_selected_map_idx = editor_selected_map_idx;
        parms[i].editor_selected_thg = editor_selected_side;
        parms[i].finished = 1;
        parms[i].visited_cell_bitmap = &visited_cells[i][0];
    }

    
#ifndef PLATFORM_WEB
    for(int i = 0; i < NUM_THREADS; i++) {
        parms[i].finished = 0;
        thread_pool_add_work(tp, raycast_wrapper, &parms[i]);
    }

    while(1) {
        int finished = 0;

            for(int i = 0; i < NUM_THREADS; i++) { 
            if(parms[i].finished) {
                finished++;
            }
        }
        if(finished == NUM_THREADS) {
            break;
        }
    }
#else
    draw_first_person_level_inner(
        output, edit_id_buffer, z_buffer, 
        start_x, end_x,
        flash_frame, this_level, player_x, player_y, player_z,
        player_ang, pitch, 
        editor_mode_enabled, editor_selected_map_idx, editor_selected_side, visited_cell_bitmap[0]

    );
#endif 
    // x and y offsets for sprite locations
    // relative to top left of map cell
    // ONLY for fixed rotation sprites
    float offsets[4][4] = {
         // x0,x1, y0,y1
         {0.0f, 1.0f, 0.0f, 0.0f}, // N 
         {1.0f, 1.0f, 0.0f, 1.0f}, // E
         {0.0f, 1.0f, 1.0f, 1.0f}, // S
         {0.0f, 0.0f, 0.0f, 1.0f}, // W
    };

    {
        num_trans_sprites = 0;
        float sinA = sin(player_ang);
        float cosA = cos(player_ang);

        // Forward
        float forwardX = cosA;
        float forwardY = sinA;

        // Right (90° clockwise from forward)
        float rightX = -sinA;
        float rightY = cosA;
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
                int map_y = sector/32;
                int map_x = sector - (map_y*32);
                u8 idx = this_level->sprite_index[sector].index;
                if(idx == EMPTY_SPRITE_INDEX) {
                    continue;
                }

                int is_fixed_rotation = this_level->sprite_index[sector].is_fixed_rotation;
                int image_idx = this_level->sprite_index[sector].index;

                float sprite_world_x, sprite_world_y;
                float sprite_world_z1 = get_height_at_point(map_x+0.5f, map_y+0.5f, 0);
                float sprite_world_z0 = sprite_world_z1+10.0f;
                
                int loc = this_level->sprite_index[sector].loc;

                float rel_x0, rel_x1, rel_x2;
                float rel_y0, rel_y1, rel_y2;
                if(is_fixed_rotation) {
                    float sprite_world_x = map_x;
                    float sprite_world_y = map_y;
                    rel_x0 = (sprite_world_x +  offsets[loc][0]) - player_x; // 2d x axis
                    rel_x2 = (sprite_world_x + offsets[loc][1]) - player_x;
                    rel_x1 = (rel_x0+rel_x2)/2.0f;//(sprite_world_x) - player_x;
                    rel_y0 = (sprite_world_y + offsets[loc][2]) - player_y; // 2d y axis
                    rel_y2 = (sprite_world_y + offsets[loc][3]) - player_y; // 2d y axis
                    rel_y1 = (rel_y0+rel_y2)/2.0f;
                } else {
                    float sprite_world_x = map_x+0.5f;
                    float sprite_world_y = map_y+0.5f;
                    rel_x0 = (sprite_world_x-0.5f) - player_x; // 2d x axis
                    rel_x1 = (sprite_world_x) - player_x;
                    rel_x2 = (sprite_world_x+0.5f) - player_x;
                    rel_y0 = sprite_world_y - player_y; // 2d y axis
                    rel_y1 = sprite_world_y - player_y; // 2d y axis
                    rel_y2 = sprite_world_y - player_y; // 2d y axis

                }



                float rot_z0;
                float rot_z1;
                float rot_z2;

                float rot_x0;
                float rot_x1;
                if(is_fixed_rotation) {

                    rot_z0 = rel_x0 * forwardX + rel_y0 * forwardY;
                    rot_z1 = rel_x1 * forwardX + rel_y1 * forwardY;
                    rot_z2 = rel_x2 * forwardX + rel_y2 * forwardY;

                    rot_x0 = rel_x0 * rightX   + rel_y0 * rightY;
                    rot_x1 = rel_x2 * rightX   + rel_y2 * rightY;
                } else {
                    rot_z1 = rel_x1 * forwardX + rel_y0 * forwardY;
                    rot_z0 = rot_z1;
                    rot_z2 = rot_z1;

                    rot_x0 = rel_x1 * rightX   + rel_y0 * rightY;
                    rot_x1 = rel_x1 * rightX   + rel_y0 * rightY;
                }

                //if(this_level)

                if(rot_z0 < NEAR_PLANE_DIST && rot_z2 < NEAR_PLANE_DIST) { continue; }
                float screen_x0, screen_x1;
                if(is_fixed_rotation) {
                    screen_x0 = FP_SCREEN_WIDTH/2.0f - (rot_x0*FOCAL_LENGTH/rot_z0);
                    screen_x1 = FP_SCREEN_WIDTH/2.0f - (rot_x1*FOCAL_LENGTH/rot_z2);
                } else {
                    screen_x0 = FP_SCREEN_WIDTH/2.0f - ((rot_x0-0.62f)*FOCAL_LENGTH/rot_z0);
                    screen_x1 = FP_SCREEN_WIDTH/2.0f - ((rot_x1+0.62f)*FOCAL_LENGTH/rot_z2);
                }

                if(MAX(screen_x0, screen_x1) < 0 || MIN(screen_x0, screen_x1) > FP_SCREEN_WIDTH-1) {
                    continue;
                }


                float screen_left_y0 = project_to_screen(sprite_world_z0, rot_z0, pitch, player_z);
                float screen_left_y1 = project_to_screen(sprite_world_z1, rot_z0, pitch, player_z);
                float screen_right_y0 = project_to_screen(sprite_world_z0, rot_z2, pitch, player_z);
                float screen_right_y1 = project_to_screen(sprite_world_z1, rot_z2, pitch, player_z);

                // find the correct place to put this transformed sprite

                int i = num_trans_sprites++;
                for(int j = i-1; j >= 0; j--) {
                    if(transformed_sprites[j].z_center >= rot_z1) {
                        break;
                    }
                    transformed_sprites[i] = transformed_sprites[j];
                    i = j;
                }
                transformed_sprites[i].x0 = screen_x0;
                transformed_sprites[i].x1 = screen_x1;
                transformed_sprites[i].left_y0 = screen_left_y0;
                transformed_sprites[i].left_y1 = screen_left_y1;
                transformed_sprites[i].right_y0 = screen_right_y0;
                transformed_sprites[i].right_y1 = screen_right_y1;
                transformed_sprites[i].z0 = rot_z0;
                transformed_sprites[i].z1 = rot_z2;
                transformed_sprites[i].z_center = rot_z1;
                transformed_sprites[i].map_idx = (map_y*MAP_SIZE+map_x);
                transformed_sprites[i].fixed_rotation = is_fixed_rotation;
                transformed_sprites[i].image_idx = image_idx;
                //}
            }
        }
    }
    printf("%i sprites: \n", num_trans_sprites);


#ifndef PLATFORM_WEB
    for(int i = 0; i < NUM_THREADS; i++) {
        parms[i].finished = 0;
        thread_pool_add_work(tp, draw_sprites_wrapper, &parms[i]);
    }    
    while(1) {
        int finished = 0;

        for(int i = 0; i < NUM_THREADS; i++) { 
            if(parms[i].finished) {
                finished++;
            }
        }
        if(finished == NUM_THREADS) {
            break;
        }
    }

#else 
    draw_transformed_sprites(output, edit_id_buffer, z_buffer, flash_frame, player_z, start_x, end_x);
#endif





    return;
        
}