#include <stdlib.h>
#include "common.h"
#include "draw.h"
#include "my_defs.h"

const float light_level_mults[4] = {1.0f, 0.50f, 1.5f, 1.5f};


void draw_z_buffered_alpha_tint_vline(u32* output, float* z_buffer, u32 *tex_column, int x, int y0, int y1, float z, int prev_drawn_top, int prev_drawn_bot, int do_depth_test, int do_alpha_test) {
    float tex_per_pix = 32.0f / (y1-y0);
    
    int clipped_y0 = MAX(y0, prev_drawn_top);
    int clipped_y1 = MIN(y1, prev_drawn_bot);
    for(int y = clipped_y0; y < clipped_y1; y++) {
        int dy = y-y0;
        int idx = (int)(dy*tex_per_pix)&31;
        if(do_depth_test) {
            float pix_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
            if(pix_z < z) {
                continue;
            }
        }
        if(do_alpha_test) {
            u32 texel_a = ((tex_column[idx] >> 24) & 0xFF);
            if(texel_a == 0) { continue; }
        }
        u32 pix = output[x*FP_SCREEN_HEIGHT+y];
        u32 r = ((pix>>16)&0xFF)>>1;
        u32 g = ((pix>>8)&0xFF)>>1;
        u32 b = ((pix>>0)&0xFF)>>1;
        u32 tinted = (pix & 0xFF000000) | (r << 16) | (g << 8) | b;
        output[x*FP_SCREEN_HEIGHT+y] = tinted;
    }
}

void draw_z_buffered_alpha_edit_vline(edit_wall_id* edit_id_buffer, float* z_buffer, u32 *tex_column, int x, float y0, float y1, float z, int prev_drawn_top, int prev_drawn_bot, int cell_idx, editor_selected_thing side, int do_depth_test, int do_alpha_test) {
    edit_wall_id id = 0xFF000000 | (side<<16) | (cell_idx << 0); 
    float tex_per_pix = 32.0f / (y1-y0);
    
    int clipped_y0 = MAX(y0, prev_drawn_top);
    int clipped_y1 = MIN(y1, prev_drawn_bot);
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
            float pix_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
            if(pix_z < z) {
                continue;
            }
        }
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}


/*
    FOR BILLBOARD SPRITES
    DEPTH TEST
    ALPHA BLEND
*/
/*
void draw_lit_fogged_textured_z_buffered_sprite(
    u32* output, float* z_buffer,
    u32 *tex_column,
    int x,
    float y0, float y1, 
    int world_y0, int world_y1, 
    pegging_type peg_type,
    float z, float light_factor, u32 fog_col) {
    //return;
    int units = my_fabsf(world_y1 - world_y0);
    float depth_scale = CLAMP(z / DARK_DIST, 0.0f, 1.0f);
    float inv_depth_scale = (1.0f - depth_scale);
    float mult = inv_depth_scale * light_factor;

    //float tex_per_pix = 32.0f / (y1-y0);
    float tex_per_pix = units * 4.0f / (y1-y0);

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
*/

/*
    FOR RAYCAST FLAT SPRITES
    DEPTH WRITE
    ALPHA TEST
*/
void draw_lit_fogged_textured_z_buffered_blended_flat_sprite(
    u32* output, float* z_buffer, u32* texture, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, int face_light_level_idx, u32 fog_col, u8 do_alpha_blend) {

    if(texture == textures[SKYBOX_TEX_IDX]) {
        return;
    }
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

    float mult = light_factor * light_level_mults[face_light_level_idx];


    for(int y = clipped_y0; y < clipped_y1; y++) {
        float old_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
        u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
        
        float cur_inv_z = (inv_z0 + d_one_over_z*(y-y0));
        float cur_z = 1.0f / cur_inv_z;
        float cur_u = CLAMP((u_over_z+d_u_over_z*(y-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
        float cur_v = CLAMP((v_over_z+d_v_over_z*(y-y0)) * cur_z * 32.0f, 0.0f, 31.0f);

        float depth_scale = (CLAMP(cur_z/DARK_DIST, 0.0f, 1.0f)) * light_factor;
        float inv_depth_scale = 1.0f - depth_scale;
        u32 scaled_fog_r = (depth_scale * fog_r);
        u32 scaled_fog_g = (depth_scale * fog_g);
        u32 scaled_fog_b = (depth_scale * fog_b);

        //depth_scale *= depth_scale;
        int u = (int)my_floorf(cur_u);
        int v = (int)my_floorf(cur_v);

        int idx = (v<<5)+u;

        u32 texel = texture[idx];

        u32 texel_a = ((texel >> 24) & 0xFF);
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        u32 old_r = (old_pix >> 16) & 0xFF;
        u32 old_g = (old_pix >> 8) & 0xFF;
        u32 old_b = (old_pix >> 0) & 0xFF;

        float tex_a = texel_a/255.0f;
        float inv_tex_a = (1.0f-tex_a);
        float r = texel_r;
        float g = texel_g;
        float b = texel_b;

        r *= mult * inv_depth_scale;
        g *= mult * inv_depth_scale;
        b *= mult * inv_depth_scale;
        r = (r * inv_depth_scale) + scaled_fog_r;// + (old_r*(1-a)); 
        g = (g * inv_depth_scale) + scaled_fog_g; //+ (old_g*(1-a));
        b = (b * inv_depth_scale) + scaled_fog_b;//+ (old_b*(1-a));
        if(do_alpha_blend) {
            r *= tex_a;
            r += (old_r * inv_tex_a);
            g *= tex_a;
            g += (old_g * inv_tex_a);
            b *= tex_a;
            b += (old_b * inv_tex_a);
        }
        u32 intr = CLAMP((int)r, 0, 0xFF);
        u32 intg = CLAMP((int)g, 0, 0xFF);
        u32 intb = CLAMP((int)b, 0, 0xFF);
        u32 lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        float new_z = (texel_a == 0 && cur_z <= old_z) ? old_z : cur_z;
        u32 new_pixel = (texel_a == 0 && cur_z <= old_z) ? old_pix : lit_texel;
        output[x*FP_SCREEN_HEIGHT+y] = 0xFF000000|(intr<<16)|(intg<<8)|intb;
        z_buffer[x*FP_SCREEN_HEIGHT+y] = cur_z;
        //cur_inv_z = next_inv_z;
        //cur_z = next_z;
        //cur_u = next_u;
        //cur_v = next_v;
    }
}


void draw_lit_fogged_textured_z_buffered_blended_sprite(
    u32* output, float* z_buffer,
    int draw_skybox,
    u32 *tex_column,
    int x,
    float y0, float y1, 
    float world_y0, float world_y1, 
    pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float world_z, float light_factor, int face_light_level_idx, u32 fog_col, u8 repeat_tex, u8 do_alpha_blend, u8 do_depth_test) {

    if(draw_skybox) {
        return;
    }
    float units = repeat_tex ? my_fabsf(world_y1 - world_y0) : 8.0f;
    float depth_scale = CLAMP(world_z / DARK_DIST, 0.0f, 1.0f);
    float inv_depth_scale = (1.0f - depth_scale);
    float mult = inv_depth_scale * light_factor * light_level_mults[face_light_level_idx]; //depth_scale * light_factor;

    float start_v = 0.0f;
    float tex_per_pix = (units * 4.0f) / (y1-y0);
    

    if(peg_type == BOTTOM_PEGGED) {
        //float end_v = 31.0f;
        start_v = 32.0f - 4.0f * units;
        //start_v += 0.01f;
    }
    while(start_v < 0) {
        start_v += 32.0f;
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
        u32 texel_a = ((texel >> 24) & 0xFF);
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        float r = texel_r * mult + scaled_fog_r;
        float g = texel_g * mult + scaled_fog_g;
        float b = texel_b * mult + scaled_fog_b;
        float old_z;
        if(do_depth_test) {
            old_z = z_buffer[x*FP_SCREEN_HEIGHT+y];
            if(old_z < world_z) {
                continue;
            }
        }
        if(do_alpha_blend) {
            u32 old_pix = output[x*FP_SCREEN_HEIGHT+y];
            u32 old_r = ((old_pix >> 16) & 0xFF);
            u32 old_g = ((old_pix >> 8) & 0xFF);
            u32 old_b = ((old_pix >> 0) & 0xFF);
            float tex_a = texel_a/255.0f;
            float inv_tex_a = 1.0f-tex_a;
            r *= tex_a;
            r += (old_r * inv_tex_a);
            g *= tex_a;
            g += (old_g * inv_tex_a);
            b *= tex_a;
            b += (old_b * inv_tex_a);
            if(tex_a == 0) {
                continue;
            }
        }

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