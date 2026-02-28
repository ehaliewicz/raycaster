#ifndef DRAW_H
#define DRAW_H

#include "common.h"

typedef enum {
    TOP_PEGGED,
    BOTTOM_PEGGED,
} pegging_type;

void draw_tint_vline(u32* output, int x, int y0, int y1, int prev_drawn_top, int prev_drawn_bot);
void draw_z_buffered_alpha_tint_vline(u32* output, float* z_buffer, u32 *tex_column, int x, int y0, int y1, float z);
void draw_z_buffered_tint_vline(u32* output, float* z_buffer, int x, int y0, int y1, float z);
void draw_alpha_tint_vline(u32* output, u32 *tex_column, int x, int y0, int y1, int prev_drawn_top, int prev_drawn_bot);
void draw_solid_vline(u32* output, float* z_buffer, int x, int y0, int y1, float world_z, u32 col, int prev_drawn_top, int prev_drawn_bot);
void draw_lit_fogged_tex_flat(
    u32* output, float* z_buffer, u32* texture, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, int face_light_level_idx, u32 fog_col);
void draw_edit_vline(edit_wall_id* edit_id_buffer, int x, float y0, float y1, int prev_drawn_top, int prev_drawn_bot, int cell_idx, editor_selected_thing side);
void draw_z_buffered_alpha_edit_vline(edit_wall_id* edit_id_buffer, float* z_buffer, u32 *tex_column, int x, float y0, float y1, float z, int cell_idx, editor_selected_thing side);
void draw_alpha_edit_vline(
    edit_wall_id* edit_id_buffer, u32 *tex_column, 
    int x, float y0, float y1, int prev_drawn_top, int prev_drawn_bot,
    int cell_idx, editor_selected_thing side);
void draw_lit_fogged_textured_z_buffered_sprite(
    u32* output, float* z_buffer,
    u32 *tex_column,
    int x,
    float y0, float y1, 
    pegging_type peg_type,
    float z, float light_factor, u32 fog_col);
void draw_lit_fogged_textured_z_buffered_blended_sprite_no_depth_test(
    u32* output, float* z_buffer,
    u32 *tex_column,
    int x,
    float y0, float y1, 
    float v0, float v1,
    int prev_drawn_top, int prev_drawn_bot,
    pegging_type peg_type,
    float z, float light_factor, u32 fog_col); 

void draw_lit_fogged_textured_z_buffered_blended_flat_sprite(
    u32* output, float* z_buffer, u32* texture, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, int face_light_level_idx, u32 fog_col);

void draw_lit_fogged_clipped_textured_wall(
    u32* output, float* z_buffer,
    int draw_skybox,
    u32 *tex_column,
    int x,
    float y0, float y1, 
    int world_y0, int world_y1, 
    pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float world_z, float light_factor, int face_light_level_idx, u32 fog_col);

u32* get_texture_column(u32* texture, float wall_u);

#endif 