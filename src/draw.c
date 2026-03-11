#include <stdlib.h>
#include "common.h"

#include "draw.h"
#include "my_defs.h"
#include "resources.h"

const float light_level_mults[4] = {1.0f, 0.50f, 1.5f, 1.5f};
const float night_light_level_mults[4] = {.50f, 0.25f, .75f, .75f};

void draw_skybox_vline(u32* output, u32 *skybox_column, int x, int y0, int y1) {
    for(int y = y0; y < y1; y++) {
        int v = (int)(SKYBOX_TEX_HEIGHT*.5f+(SKYBOX_V_PER_PIX*(y+(-pitch*(float)FP_SCREEN_HEIGHT))))&(SKYBOX_TEX_HEIGHT-1);
        u32 texell = skybox_column[v];

        output[x*FP_SCREEN_HEIGHT+y] = texell;
    }
}

/*

    tint the pixels selected, we check if the object we're drawing is the one selected from the edit buffer
*/
void draw_z_buffered_alpha_tint_vline(u32* output, u16* z_buffer, u32 *tex_column, int x, int y0, int y1, float new_z, int prev_drawn_top, int prev_drawn_bot, int do_depth_test, int do_alpha_test) {
    float tex_per_pix = 32.0f / (y1-y0);
    
    int clipped_y0 = max_int32(y0, prev_drawn_top);
    int clipped_y1 = min_int32(y1, prev_drawn_bot);
    u16 fix_z = new_z*FIXED_POINT_MULT;
    for(int y = clipped_y0; y < clipped_y1; y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        
        if(do_alpha_test) {
            u32 texel_a = ((tex_column[idx] >> 24) & 0xFF);
            if(texel_a == 0) { continue; }
        }
        if(do_depth_test) {
            float old_z = (z_buffer[x*FP_SCREEN_HEIGHT+y]); // 10.6 fixed point depth?
            if(old_z < fix_z) {
                continue;
            }
        }
        u32 pix = output[x*FP_SCREEN_HEIGHT+y];
        u32 r = ((pix>>16)&0xFF)>>1;
        u32 g = ((pix>>8)&0xFF)>>1;
        u32 b = ((pix>>0)&0xFF)>>1;
        u32 tinted = (pix & 0xFF000000) | (r << 16) | (g << 8) | b;
        output[x*FP_SCREEN_HEIGHT+y] = tinted;
    }
}

/*
    draws an identifier into the edit buffer
    if the user clicks on a pixel, we look up the object via this edit buffer
*/
void draw_z_buffered_alpha_edit_vline(edit_wall_id* edit_id_buffer, u16* z_buffer, u32 *tex_column, int x, float y0, float y1, float new_z, int prev_drawn_top, int prev_drawn_bot, int cell_idx, editor_selected_thing side, int do_depth_test, int do_alpha_test) {
    edit_wall_id id = 0xFF000000 | (side<<16) | (cell_idx << 0); 
    float tex_per_pix = 32.0f / (y1-y0);
    
    int clipped_y0 = max_int32(y0, prev_drawn_top);
    int clipped_y1 = min_int32(y1, prev_drawn_bot);
    u16 fix_z = new_z*FIXED_POINT_MULT;
    for(int y = clipped_y0; y < clipped_y1; y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        if(do_alpha_test) {
            u32 texel = tex_column[idx];
            u32 texel_a = ((texel >> 24) & 0xFF);
            int a = texel_a != 0 ? 1 : 0;
            if(a == 0) { continue; }
        }
        if(do_depth_test){
            u16 old_z = ((float)z_buffer[x*FP_SCREEN_HEIGHT+y]); 
            if(old_z < new_z) {
                continue;
            }
        }
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}


/*
    FOR RAYCAST FLAT SPRITES
    DEPTH WRITE
    ALPHA TEST
*/
void draw_lit_fogged_textured_z_buffered_blended_flat_sprite(
    u32* output, u16* z_buffer, u32* texture,  u32* skybox_col, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, int face_light_level_idx, u32 fog_col, u8 do_alpha_blend) {

    int clipped_y0 = max_int32(y0, prev_drawn_top);
    int clipped_y1 = min_int32(y1, prev_drawn_bot);
    if(texture == textures[SKYBOX_TEX_IDX]) {
        draw_skybox_vline(output, skybox_col, x, clipped_y0, clipped_y1);
        return;
    }
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    float d_one_over_z = ((1.0f/z1) - inv_z0) / (y1-y0);
    float u_over_z = start_u * inv_z0;
    float v_over_z = start_v * inv_z0;
    float d_u_over_z = ((end_u * inv_z1) - u_over_z) / (y1-y0);
    float d_v_over_z = ((end_v * inv_z1) - v_over_z) / (y1-y0);
    
    
    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;

    float mult = light_factor * light_level_mults[face_light_level_idx];


    float cur_z = 1.0f/(inv_z0 + d_one_over_z * (clipped_y0-y0));
    float cur_u = CLAMP((u_over_z + d_u_over_z * (clipped_y0-y0)) * cur_z, 0.0f, 0.999f);
    float cur_v = CLAMP((v_over_z + d_v_over_z * (clipped_y0-y0)) * cur_z, 0.0f, 0.999f);
    float cur_depth_scale = cur_z*RECIP_DARK_DIST;

    #define SPAN_LENGTH 8

    int num_pixels = (clipped_y1-clipped_y0);
    int num_pixel_chunks = num_pixels/SPAN_LENGTH;
    
    for(int i = 0; i < num_pixel_chunks; i++) {
        int base_y = clipped_y0 + i*SPAN_LENGTH;
        int next_y = base_y + SPAN_LENGTH;
        float next_z = 1.0f/(inv_z0 + d_one_over_z * (next_y-y0));
        float next_u = (u_over_z + d_u_over_z * (next_y-y0)) * next_z;
        float next_v = (v_over_z + d_v_over_z * (next_y-y0)) * next_z;
        next_u = CLAMP(next_u, 0.0f, 0.999f);
        next_v = CLAMP(next_v, 0.0f, 0.999f);
        float z_per_pix = (next_z-cur_z)/SPAN_LENGTH;
        float u_per_pix = (next_u-cur_u)/SPAN_LENGTH;
        float v_per_pix = (next_v-cur_v)/SPAN_LENGTH;

        u16 fix_z = cur_z*FIXED_POINT_MULT;
        int fix_u = cur_u*65536.0f;
        int fix_v = cur_v*65536.0f;
        int fix_z_per_pix = z_per_pix*FIXED_POINT_MULT;
        int fix_u_per_pix = u_per_pix*65536.0f;
        int fix_v_per_pix = v_per_pix*65536.0f;

        float next_depth_scale = next_z*RECIP_DARK_DIST;
        float depth_scale_per_pix = (next_depth_scale-cur_depth_scale)/SPAN_LENGTH;

        for(int j = 0; j < SPAN_LENGTH; j++) {
            int y = base_y + j;
            float depth_scale = cur_depth_scale;
            float inv_depth_scale = 1.0f - depth_scale;
            float mult_by_inv_depth = mult * inv_depth_scale;

            u32 scaled_fog_r = (depth_scale * fog_r);
            u32 scaled_fog_g = (depth_scale * fog_g);
            u32 scaled_fog_b = (depth_scale * fog_b);

            // for v, we want the top 5 bits shifted into bits 5-through 9
            int idx = ((fix_v>>6)&0b1111100000)|((fix_u>>11)&0b11111);

            u32 texel = texture[idx];

            u32 texel_a = ((texel >> 24) & 0xFF);
            u32 texel_r = ((texel >> 16) & 0xFF);
            u32 texel_g = ((texel >> 8) & 0xFF);
            u32 texel_b = ((texel >> 0) & 0xFF);

            float tex_a = texel_a/255.0f;
            float inv_tex_a = (1.0f-tex_a);
            float r = texel_r;
            float g = texel_g;
            float b = texel_b;

            
            r = (r * mult_by_inv_depth);
            g = (g * mult_by_inv_depth);
            b = (b * mult_by_inv_depth);
            if(do_alpha_blend) {
                r += scaled_fog_r * tex_a;
                g += scaled_fog_g * tex_a;
                b += scaled_fog_b * tex_a;
                u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
                u32 old_r = (old_pix >> 16) & 0xFF;
                u32 old_g = (old_pix >> 8) & 0xFF;
                u32 old_b = (old_pix >> 0) & 0xFF;
                r += (old_r * inv_tex_a);
                g += (old_g * inv_tex_a);
                b += (old_b * inv_tex_a);
            } else {
                r += scaled_fog_r;
                g += scaled_fog_g;
                b += scaled_fog_b;
            }

            u32 intr = CLAMP((int)r, 0, 0xFF);
            u32 intg = CLAMP((int)g, 0, 0xFF);
            u32 intb = CLAMP((int)b, 0, 0xFF);
            u32 lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;
            output[x*FP_SCREEN_HEIGHT+y] = lit_texel;
            z_buffer[x*FP_SCREEN_HEIGHT+y] = fix_z;
            cur_z += z_per_pix;
            fix_z += fix_z_per_pix;
            fix_u += fix_u_per_pix;
            fix_v += fix_v_per_pix;
            cur_depth_scale += depth_scale_per_pix;
        }
        
        cur_z = next_z;
        cur_u = next_u;
        cur_v = next_v;

    }


    int rem_pixels = (num_pixels-(num_pixel_chunks*SPAN_LENGTH));
    
    int base_y = clipped_y0 + num_pixel_chunks*SPAN_LENGTH;
    float next_z = 1.0f/(inv_z0 + d_one_over_z * (clipped_y1-y0));
    float next_u = CLAMP((u_over_z + d_u_over_z * (clipped_y1-y0)) * next_z, 0.0f, 0.999f);
    float next_v = CLAMP((v_over_z + d_v_over_z * (clipped_y1-y0)) * next_z, 0.0f, 0.999f);

    u16 fix_z = cur_z*FIXED_POINT_MULT;
    float z_per_pix = (next_z-cur_z)/(clipped_y1-base_y);
    float u_per_pix = (next_u-cur_u)/(clipped_y1-base_y);
    float v_per_pix = (next_v-cur_v)/(clipped_y1-base_y);
    int fix_z_per_pix = z_per_pix*FIXED_POINT_MULT;
    float next_depth_scale = next_z*RECIP_DARK_DIST;
    float depth_scale_per_pix = (next_depth_scale-cur_depth_scale)/(clipped_y1-base_y);


    int fix_u = cur_u*65536.0f;
    int fix_v = cur_v*65536.0f;
    int fix_u_per_pix = u_per_pix*65536.0f;
    int fix_v_per_pix = v_per_pix*65536.0f;


    for(int j = 0; j < rem_pixels; j++) {

        int y = base_y + j;
        float depth_scale = cur_depth_scale;
        float inv_depth_scale = 1.0f - depth_scale;
        float mult_by_inv_depth = mult * inv_depth_scale;

        u32 scaled_fog_r = (depth_scale * fog_r);
        u32 scaled_fog_g = (depth_scale * fog_g);
        u32 scaled_fog_b = (depth_scale * fog_b);

        int idx = ((fix_v>>6)&0b1111100000)|((fix_u>>11)&0b11111);

        u32 texel = texture[idx];

        u32 texel_a = ((texel >> 24) & 0xFF);
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);

        float tex_a = texel_a/255.0f;
        float inv_tex_a = (1.0f-tex_a);
        float r = texel_r;
        float g = texel_g;
        float b = texel_b;

        r = (r * mult_by_inv_depth);
        g = (g * mult_by_inv_depth);
        b = (b * mult_by_inv_depth);
        if(do_alpha_blend) {
            r += scaled_fog_r * tex_a;
            g += scaled_fog_g * tex_a;
            b += scaled_fog_b * tex_a;
            u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
            u32 old_r = (old_pix >> 16) & 0xFF;
            u32 old_g = (old_pix >> 8) & 0xFF;
            u32 old_b = (old_pix >> 0) & 0xFF;
            r += (old_r * inv_tex_a);
            g += (old_g * inv_tex_a);
            b += (old_b * inv_tex_a);
        } else {
            r += scaled_fog_r;
            g += scaled_fog_g;
            b += scaled_fog_b;
        }

        u32 intr = CLAMP((int)r, 0, 0xFF);
        u32 intg = CLAMP((int)g, 0, 0xFF);
        u32 intb = CLAMP((int)b, 0, 0xFF);
        u32 lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        output[x*FP_SCREEN_HEIGHT+y] = lit_texel;
        z_buffer[x*FP_SCREEN_HEIGHT+y] = fix_z;
        
        cur_z += z_per_pix;
        fix_z += fix_z_per_pix;
        fix_u += fix_u_per_pix;
        fix_v += fix_v_per_pix;
        cur_depth_scale += depth_scale_per_pix;
    }


}


void draw_lit_fogged_textured_z_buffered_blended_sprite(
    u32* output, u16* z_buffer,
    int draw_skybox, 
    u32 *tex_column, int top_skip, u32* skybox_col,
    int x,
    float y0, float y1, 
    float world_y0, float world_y1, 
    pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float world_z, float light_factor, int face_light_level_idx, u32 fog_col, u8 repeat_tex, u8 do_alpha_blend, u8 do_depth_test) {

    //return;
    int clipped_y0 = max_int32(y0, prev_drawn_top);
    int clipped_y1 = min_int32(y1, prev_drawn_bot);
    if(draw_skybox) {
        draw_skybox_vline(output, skybox_col, x, clipped_y0, clipped_y1);
        return;
    }
    float units = repeat_tex ? my_fabsf(world_y1 - world_y0) : 8.0f;
    float depth_scale = CLAMP(world_z / DARK_DIST, 0.0f, 1.0f);
    float inv_depth_scale = (1.0f - depth_scale);
    float mult = inv_depth_scale * light_factor * light_level_mults[face_light_level_idx]; //depth_scale * light_factor;

    float start_v = 0.0f;
    float tex_per_pix = (units * 4.0f) / (y1-y0);
    

    if(peg_type == BOTTOM_PEGGED) {
        start_v = 32.0f - 4.0f * units;
    }

    u32 fog_r = (fog_col >> 16)&0xFF;
    u32 fog_g = (fog_col >> 8)&0xFF;
    u32 fog_b = (fog_col >> 0)&0xFF;
    u32 scaled_fog_r = (depth_scale * fog_r);
    u32 scaled_fog_g = (depth_scale * fog_g);
    u32 scaled_fog_b = (depth_scale * fog_b);

    int y = clipped_y0;

    top_skip -= (clipped_y0-y0);
    
    if(top_skip > 0) {
        int top_skip_pixels = top_skip / tex_per_pix;
        y += top_skip_pixels;
    }
    
    int dy = y-y0;

    int fix_tex_per_pix = (int)(tex_per_pix *64.0f* 65536.0f); // 5.16
    int fix_idx = (int)(start_v*64.0f*65536.0f) + dy*fix_tex_per_pix;
    u16 fix_z = world_z * FIXED_POINT_MULT;
    

    for(;y < clipped_y1; y++) {

        u32 texel = tex_column[(fix_idx>>22)&31];
        fix_idx += fix_tex_per_pix;

        u32 texel_a = ((texel >> 24) & 0xFF);
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        float r = texel_r * mult;
        float g = texel_g * mult;
        float b = texel_b * mult;
        float old_z;
        if(do_depth_test) {
            old_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
            if(old_z < fix_z) {
                continue;
            }
        }
        
        if(do_alpha_blend) {

            u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
            u32 old_r = ((old_pix >> 16) & 0xFF);
            u32 old_g = ((old_pix >> 8) & 0xFF);
            u32 old_b = ((old_pix >> 0) & 0xFF);
            float tex_a = texel_a/255.0f;
            r += scaled_fog_r*tex_a;
            g += scaled_fog_g*tex_a;
            b += scaled_fog_b*tex_a;
            //if(texel_a != 255) {
            //    continue;
            //} else { tex_a == 1.0f; }
            float inv_tex_a = 1.0f-tex_a;
            r += (old_r * inv_tex_a);
            g += (old_g * inv_tex_a);
            b += (old_b * inv_tex_a);
            if(tex_a == 0) {
                continue;
            }
        } else {
            r += scaled_fog_r;
            g += scaled_fog_g;
            b += scaled_fog_b;
        }

        u32 intr = CLAMP((int)r, 0, 0xFF);
        u32 intg = CLAMP((int)g, 0, 0xFF);
        u32 intb = CLAMP((int)b, 0, 0xFF);


        output[x*FP_SCREEN_HEIGHT+y] = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        z_buffer[x*FP_SCREEN_HEIGHT+y] = fix_z;
    }
}


u32* get_texture_column(u32* texture, float wall_u) {
    float u_scaled_to_tex_size = CLAMP(wall_u * 32.0f, 0.0f, 31.0f);
    int int_u = (int)(u_scaled_to_tex_size);
    return &texture[int_u*TEX_SIZE];
}