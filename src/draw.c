#include <math.h>
#include "common.h"
#include "draw.h"

const float light_level_mults[4] = {1.0f, 0.50f, 1.5f, 1.5f};

u32 tint_pixel(u32 pix) {
    u32 r = ((pix>>16)&0xFF)>>1;
    u32 g = ((pix>>8)&0xFF)>>1;
    u32 b = ((pix>>0)&0xFF)>>1;
    return (pix & 0xFF000000) | (r << 16) | (g << 8) | b;
}
void draw_tint_vline(u32* output, int x, int y0, int y1, int prev_drawn_top, int prev_drawn_bot) {
    for(int y = MAX(y0, prev_drawn_top); y < MIN(y1, prev_drawn_bot); y++) {
        output[x*FP_SCREEN_HEIGHT+y] = tint_pixel(output[x*FP_SCREEN_HEIGHT+y]);
    }
}

void draw_z_buffered_alpha_tint_vline(u32* output, float* z_buffer, u32 *tex_column, int x, int y0, int y1, float z) {
    float tex_per_pix = 32.0f / (y1-y0);
    for(int y = CLAMP(y0, 0, FP_SCREEN_HEIGHT-1); y < CLAMP(y1, 0, FP_SCREEN_HEIGHT); y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        float pix_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
        if(pix_z < z) {
            continue;
        }
        u32 texel = tex_column[idx];
        u32 texel_a = ((texel >> 24) & 0xFF);
        int a = texel_a != 0 ? 1 : 0;
        if(a == 0) { continue; }
        output[x*FP_SCREEN_HEIGHT+y] = tint_pixel(output[x*FP_SCREEN_HEIGHT+y]);
    }
}

void draw_z_buffered_tint_vline(u32* output, float* z_buffer, int x, int y0, int y1, float z) {
    float tex_per_pix = 32.0f / (y1-y0);
    for(int y = CLAMP(y0, 0, FP_SCREEN_HEIGHT-1); y < CLAMP(y1, 0, FP_SCREEN_HEIGHT); y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        float pix_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
        if(pix_z < z) {
            continue;
        }
        output[x*FP_SCREEN_HEIGHT+y] = tint_pixel(output[x*FP_SCREEN_HEIGHT+y]);
    }
}

void draw_alpha_tint_vline(u32* output, u32 *tex_column, int x, int y0, int y1, int prev_drawn_top, int prev_drawn_bot) {
    float tex_per_pix = 32.0f / (y1-y0);
    for(int y = MAX(y0, prev_drawn_top); y < MIN(y1, prev_drawn_bot); y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;

        u32 texel = tex_column[idx];
        u32 texel_a = ((texel >> 24) & 0xFF);
        int a = texel_a != 0 ? 1 : 0;
        if(a == 0) { continue; }
        output[x*FP_SCREEN_HEIGHT+y] = tint_pixel(output[x*FP_SCREEN_HEIGHT+y]);
    }
}

void draw_solid_vline(u32* output, float* z_buffer, int x, int y0, int y1, float world_z, u32 col, int prev_drawn_top, int prev_drawn_bot) {
    for(int y = MAX(y0,prev_drawn_top); y < MIN(y1, prev_drawn_bot); y++) {
        output[x*FP_SCREEN_HEIGHT+y] = col;
        z_buffer[x*FP_SCREEN_HEIGHT+y] = world_z;
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
        output[x*FP_SCREEN_HEIGHT+y*4+0] = col.r * scale;
        output[x*FP_SCREEN_HEIGHT+y*4+1] = col.g * scale;
        output[x*FP_SCREEN_HEIGHT+y*4+2] = col.b * scale;
    }
}
*/

// draws a textured flat surface
void draw_lit_fogged_tex_flat(
    u32* output, float* z_buffer, u32* texture, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, int face_light_level_idx, u32 fog_col) {
       // return;
    if(texture == textures[SKYBOX_TEX_IDX]) {
        return;
    }

    //if(y0 > prev_drawn_bot) {
    //    return;
    //}
    //if(y1 < prev_drawn_top) {
    //    return;
    //}    
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    float d_one_over_z = ((1.0f/z1) - inv_z0) / (y1-y0);
    float u_over_z = start_u * inv_z0;
    float v_over_z = start_v * inv_z0;
    float d_u_over_z = ((end_u * inv_z1) - u_over_z) / (y1-y0);
    float d_v_over_z = ((end_v * inv_z1) - v_over_z) / (y1-y0);
    
    int clipped_y0 = MAX(y0, prev_drawn_top);
    int clipped_y1 = MIN(y1, prev_drawn_bot);
    
    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    float cur_inv_z = (inv_z0 + d_one_over_z*(clipped_y0-y0));
    float cur_z = 1.0f / cur_inv_z;
    float cur_u = CLAMP((u_over_z+d_u_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    float cur_v = CLAMP((v_over_z+d_v_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    float mult = light_factor * light_level_mults[face_light_level_idx];


    for(int y = clipped_y0; y < clipped_y1; y++) {
        
        float next_inv_z = (inv_z0 + d_one_over_z*(y+1-y0));
        float next_z = 1.0f / next_inv_z;
        float next_u = CLAMP((u_over_z+d_u_over_z*(y+1-y0)) * next_z * 32.0f, 0.0f, 31.0f);
        float next_v = CLAMP((v_over_z+d_v_over_z*(y+1-y0)) * next_z * 32.0f, 0.0f, 31.0f);

        float depth_scale = (CLAMP(cur_z/DARK_DIST, 0.0f, 1.0f)) * light_factor;
        float inv_depth_scale = 1.0f - depth_scale;
        u32 scaled_fog_r = (depth_scale * fog_r);
        u32 scaled_fog_g = (depth_scale * fog_g);
        u32 scaled_fog_b = (depth_scale * fog_b);

        //depth_scale *= depth_scale;
        int u = (int)floorf(cur_u);
        int v = (int)floorf(cur_v);

        int idx = (v<<5)+u;

        u32 texel = texture[idx];
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        float r = texel_r * mult;
        float g = texel_g * mult;
        float b = texel_b * mult;
        r = ((r * inv_depth_scale) + scaled_fog_r);
        g = ((g * inv_depth_scale) + scaled_fog_g);
        b = ((b * inv_depth_scale) + scaled_fog_b);
        u32 intr = CLAMP((int)r, 0, 0xFF);
        u32 intg = CLAMP((int)g, 0, 0xFF);
        u32 intb = CLAMP((int)b, 0, 0xFF);
        output[x*FP_SCREEN_HEIGHT+y] = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        z_buffer[x*FP_SCREEN_HEIGHT+y] = cur_z;
        cur_inv_z = next_inv_z;
        cur_z = next_z;
        cur_u = next_u;
        cur_v = next_v;
    }
}


void draw_edit_vline(edit_wall_id* edit_id_buffer, int x, float y0, float y1, int prev_drawn_top, int prev_drawn_bot, int cell_idx, editor_selected_thing side) {
    //edit_wall_id id = 0xFF000000 | (cell_idx << 8) | side; //{.alpha = 0xFF, .cell_idx = cell_idx, .side = side};
    
    edit_wall_id id = 0xFF000000 | (side<<16) | (cell_idx << 0); 

    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}

void draw_z_buffered_alpha_edit_vline(edit_wall_id* edit_id_buffer, float* z_buffer, u32 *tex_column, int x, float y0, float y1, float z, int cell_idx, editor_selected_thing side) {
    //edit_wall_id id = {.alpha = 0xFF, .cell_idx = cell_idx, .side = side};
    edit_wall_id id = 0xFF000000 | (side<<16) | (cell_idx << 0); 
    float tex_per_pix = 32.0f / (y1-y0);
    for(int y = CLAMP(y0, 0, FP_SCREEN_HEIGHT-1); y < CLAMP(y1, 0, FP_SCREEN_HEIGHT-1); y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        u32 texel = tex_column[idx];
        u32 texel_a = ((texel >> 24) & 0xFF);
        int a = texel_a != 0 ? 1 : 0;
        if(a == 0) { continue; }
        
        float pix_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
        if(pix_z < z) {
            continue;
        }
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}

void draw_alpha_edit_vline(
    edit_wall_id* edit_id_buffer, u32 *tex_column, 
    int x, float y0, float y1, int prev_drawn_top, int prev_drawn_bot,
    int cell_idx, editor_selected_thing side) {
    //edit_wall_id id = {.alpha = 0xFF, .cell_idx = cell_idx, .side = side};
    edit_wall_id id = 0xFF000000 | (side<<16) | (cell_idx << 0); 
    float tex_per_pix = 32.0f / (y1-y0);
    for(int y = MAX(y0, prev_drawn_top); y < MIN(y1, prev_drawn_bot); y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        u32 texel = tex_column[idx];
        u32 texel_a = ((texel >> 24) & 0xFF);
        int a = texel_a != 0 ? 1 : 0;
        if(a == 0) { continue; }
        
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}


void draw_lit_fogged_textured_z_buffered_sprite(
    u32* output, float* z_buffer,
    u32 *tex_column,
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
            u32 texel = tex_column[idx];
            u32 texel_a = ((texel >> 24) & 0xFF);
            float old_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
            u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
            int use_new_pix = (texel_a != 0 && z < old_z);
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
            u32 use_pix = use_new_pix ? lit_texel : old_pix;//old_pix;
            z_buffer[x*FP_SCREEN_HEIGHT+y] = new_z;
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
            output[x*FP_SCREEN_HEIGHT+y] = use_pix;// 0xFF000000|(intr<<16)|(intg<<8)|intb;
    }
}


void draw_lit_fogged_textured_z_buffered_blended_sprite_no_depth_test(
    u32* output, float* z_buffer,
    u32 *tex_column,
    int x,
    float y0, float y1, 
    float v0, float v1,
    int prev_drawn_top, int prev_drawn_bot,
    pegging_type peg_type,
    float z, float light_factor, u32 fog_col) {
    //return;
    float depth_scale = CLAMP(z / DARK_DIST, 0.0f, 1.0f);
    float inv_depth_scale = (1.0f - depth_scale);
    float mult = inv_depth_scale * light_factor;
    
    float dv = (32.0f * (v1-v0));
    float tex_per_pix = dv / (y1-y0);

    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    u32 scaled_fog_r = (depth_scale * fog_r);
    u32 scaled_fog_g = (depth_scale * fog_g);
    u32 scaled_fog_b = (depth_scale * fog_b);

    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
            float old_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
            u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
            int dy = y-y0;
            int idx = (int)(v0+dy*tex_per_pix)&31;
            u32 texel = tex_column[idx];
            u32 texel_a = ((texel >> 24) & 0xFF);
            float a = texel_a/255.0f;
            u32 old_r = (old_pix >> 16) & 0xFF;
            u32 old_g = (old_pix >> 8) & 0xFF;
            u32 old_b = (old_pix >> 0) & 0xFF;

            u32 texel_r = (((texel >> 16) & 0xFF) * mult + scaled_fog_r) + (old_r*(1-a));
            u32 texel_g = (((texel >> 8) & 0xFF) * mult + scaled_fog_g) + (old_g*(1-a));
            u32 texel_b = (((texel >> 0) & 0xFF) * mult + scaled_fog_b) + (old_b*(1-a));
            
            u32 intr = CLAMP((int)texel_r, 0, 0xFF);
            u32 intg = CLAMP((int)texel_g, 0, 0xFF);
            u32 intb = CLAMP((int)texel_b, 0, 0xFF);
            u32 lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;
            float new_z = (texel_a == 0) ? old_z : z;
            u32 new_pixel = (texel_a == 0) ? old_pix : lit_texel;
            output[x*FP_SCREEN_HEIGHT+y] = new_pixel;
            z_buffer[x*FP_SCREEN_HEIGHT+y] = new_z;
    }
}

void draw_lit_fogged_textured_z_buffered_blended_flat_sprite(
    u32* output, float* z_buffer, u32* texture, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, int face_light_level_idx, u32 fog_col) {
       // return;

  
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    float d_one_over_z = ((1.0f/z1) - inv_z0) / (y1-y0);
    float u_over_z = start_u * inv_z0;
    float v_over_z = start_v * inv_z0;
    float d_u_over_z = ((end_u * inv_z1) - u_over_z) / (y1-y0);
    float d_v_over_z = ((end_v * inv_z1) - v_over_z) / (y1-y0);
    
    int clipped_y0 = MAX(y0, prev_drawn_top);
    int clipped_y1 = MIN(y1, prev_drawn_bot);
    
    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    float cur_inv_z = (inv_z0 + d_one_over_z*(clipped_y0-y0));
    float cur_z = 1.0f / cur_inv_z;
    float cur_u = CLAMP((u_over_z+d_u_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    float cur_v = CLAMP((v_over_z+d_v_over_z*(clipped_y0-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
    float mult = light_factor * light_level_mults[face_light_level_idx];


    for(int y = clipped_y0; y < clipped_y1; y++) {
        float old_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
        u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
        
        float next_inv_z = (inv_z0 + d_one_over_z*(y+1-y0));
        float next_z = 1.0f / next_inv_z;
        float next_u = CLAMP((u_over_z+d_u_over_z*(y+1-y0)) * next_z * 32.0f, 0.0f, 31.0f);
        float next_v = CLAMP((v_over_z+d_v_over_z*(y+1-y0)) * next_z * 32.0f, 0.0f, 31.0f);

        float depth_scale = (CLAMP(cur_z/DARK_DIST, 0.0f, 1.0f)) * light_factor;
        float inv_depth_scale = 1.0f - depth_scale;
        u32 scaled_fog_r = (depth_scale * fog_r);
        u32 scaled_fog_g = (depth_scale * fog_g);
        u32 scaled_fog_b = (depth_scale * fog_b);

        //depth_scale *= depth_scale;
        int u = (int)floorf(cur_u);
        int v = (int)floorf(cur_v);

        int idx = (v<<5)+u;

        u32 texel = texture[idx];

        u32 texel_a = ((texel >> 24) & 0xFF);
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        u32 old_r = (old_pix >> 16) & 0xFF;
        u32 old_g = (old_pix >> 8) & 0xFF;
        u32 old_b = (old_pix >> 0) & 0xFF;

        float a = texel_a/255.0f;
        float r = texel_r;
        float g = texel_g;
        float b = texel_b;

        r *= mult * inv_depth_scale;
        g *= mult * inv_depth_scale;
        b *= mult * inv_depth_scale;
        r = (r * inv_depth_scale) + scaled_fog_r + (old_r*(1-a)); 
        g = (g * inv_depth_scale) + scaled_fog_g + (old_g*(1-a));
        b = (b * inv_depth_scale) + scaled_fog_b + (old_b*(1-a));
        u32 intr = CLAMP((int)r, 0, 0xFF);
        u32 intg = CLAMP((int)g, 0, 0xFF);
        u32 intb = CLAMP((int)b, 0, 0xFF);
        u32 lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        float new_z = (texel_a == 0 && cur_z <= old_z) ? old_z : cur_z;
        u32 new_pixel = (texel_a == 0 && cur_z <= old_z) ? old_pix : lit_texel;
        output[x*FP_SCREEN_HEIGHT+y] = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        z_buffer[x*FP_SCREEN_HEIGHT+y] = cur_z;
        cur_inv_z = next_inv_z;
        cur_z = next_z;
        cur_u = next_u;
        cur_v = next_v;
    }
}


void draw_lit_fogged_clipped_textured_wall(
    u32* output, float* z_buffer,
    int draw_skybox,
    u32 *tex_column,
    int x,
    float y0, float y1, 
    int world_y0, int world_y1, 
    pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float world_z, float light_factor, int face_light_level_idx, u32 fog_col) {

    if(draw_skybox) {
        return;
    }
    if(y0 > prev_drawn_bot) {
        return;
    }
    if(y1 < prev_drawn_top) {
        return;
    }

    int units = abs(world_y1 - world_y0);
    float depth_scale = CLAMP(world_z / DARK_DIST, 0.0f, 1.0f);
    float inv_depth_scale = (1.0f - depth_scale);
    float mult = light_factor * light_level_mults[face_light_level_idx]; //depth_scale * light_factor;

    float start_v = 0.0f;
    float tex_per_pix = units * 4.0f / (y1-y0);
    

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

    for(int y = MAX(y0, prev_drawn_top); y < MIN(y1, prev_drawn_bot); y++) {
        int dy = y-y0;

        int idx = (int)(start_v + dy*tex_per_pix)&31;

        u32 texel = tex_column[idx];
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


        output[x*FP_SCREEN_HEIGHT+y] = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        z_buffer[x*FP_SCREEN_HEIGHT+y] = world_z;
    }
}


u32* get_texture_column(u32* texture, float wall_u) {
    float u_scaled_to_tex_size = CLAMP(wall_u * 32.0f, 0.0f, 31.0f);
    int int_u = (int)(u_scaled_to_tex_size);
    return &texture[int_u*TEX_SIZE];
}