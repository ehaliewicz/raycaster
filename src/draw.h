#ifndef DRAW_H
#define DRAW_H

#include "common.h"

typedef enum {
    TOP_PEGGED,
    BOTTOM_PEGGED,
} pegging_type;

#define DO_DEPTH_TEST 1
#define NO_DEPTH_TEST 0
#define DO_ALPHA_TEST 1
#define NO_ALPHA_TEST 0
#define DO_ALPHA_BLEND 1
#define NO_ALPHA_BLEND 0
#define REPEAT_TEX 1
#define NO_REPEAT_TEX 0

void draw_skybox_vline(u32* output, u32 *skybox_column, int x, int y0, int y1);

void draw_z_buffered_alpha_tint_vline(u32* output, float* z_buffer, u32 *tex_column, int x, int y0, int y1, float z, int prev_drawn_top, int prev_drawn_bot, int do_depth_test, int do_alpha_test);

#define draw_tint_vline(output,  x,  y0,  y1,  prev_drawn_top,  prev_drawn_bot) draw_z_buffered_alpha_tint_vline(output, NULL, NULL, x, y0, y1, 0.0f, prev_drawn_top, prev_drawn_bot, 0, 0)
#define draw_alpha_tint_vline(output,  tex_column,  x,  y0,  y1,  prev_drawn_top,  prev_drawn_bot) draw_z_buffered_alpha_tint_vline(output, NULL, tex_column, x, y0, y1, 0.0f, prev_drawn_top, prev_drawn_bot, 0, 1)


void draw_z_buffered_alpha_edit_vline(edit_wall_id* edit_id_buffer, float* z_buffer, u32 *tex_column, int x, float y0, float y1, float z, int prev_drawn_top, int prev_drawn_bot, int cell_idx, editor_selected_thing side, int do_depth_test, int do_alpha_test);

#define draw_alpha_edit_vline(edit_id_buffer, tex_column, x, y0, y1, prev_drawn_top, prev_drawn_bot, cell_idx, side) draw_z_buffered_alpha_edit_vline(edit_id_buffer, NULL, tex_column, x, y0, y1, 0.0f, prev_drawn_top, prev_drawn_bot, cell_idx, side, 0, 1)
#define draw_edit_vline(edit_id_buffer, x, y0, y1, prev_drawn_top, prev_drawn_bot, cell_idx, side) draw_z_buffered_alpha_edit_vline(edit_id_buffer, NULL, NULL, x, y0, y1, 0.0f, prev_drawn_top, prev_drawn_bot, cell_idx, side, 0, 0)

// horizontal sprites
void draw_lit_fogged_textured_z_buffered_blended_flat_sprite(
    u32* output, float* z_buffer, u32* texture, u32* skybox, int x, int y0, int y1, float z0, float z1, float start_u, 
    float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, 
    float light_factor, int face_light_level_idx, u32 fog_col, u8 do_alpha_blend);

// horizontal floors/ceils
#define draw_lit_fogged_tex_flat(output, z_buffer, texture, skybox_col, x, y0, y1, z0, z1, start_u, start_v, end_u, end_v, prev_drawn_top, prev_drawn_bot, light_factor, face_light_level_idx, fog_col) \
    draw_lit_fogged_textured_z_buffered_blended_flat_sprite(output, z_buffer, texture, skybox_col, x, y0, y1, z0, z1, start_u, start_v, end_u, end_v, prev_drawn_top, prev_drawn_bot, light_factor, face_light_level_idx, fog_col, NO_ALPHA_BLEND)

// vertical sprites
void draw_lit_fogged_textured_z_buffered_blended_sprite(
    u32* output, float* z_buffer,
    int draw_skybox,
    u32 *tex_column, u32* skybox, 
    int x,
    float y0, float y1, 
    float world_y0, float world_y1, 
    pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float world_z, float light_factor, int face_light_level_idx, u32 fog_col, u8 repeat_tex, u8 do_alpha_blend, u8 do_depth_test);

//vertical walls
#define draw_lit_fogged_clipped_textured_wall(output, z_buffer, draw_skybox, tex_column, skybox_col, x, y0, y1, world_y0, world_y1, peg_type, prev_drawn_top, prev_drawn_bot, world_z, light_factor, face_light_level_idx, fog_col, repeat_tex) \
    draw_lit_fogged_textured_z_buffered_blended_sprite(output, z_buffer, draw_skybox, tex_column, skybox_col, x, y0, y1, world_y0, world_y1, peg_type, prev_drawn_top, prev_drawn_bot, world_z, light_factor, face_light_level_idx, fog_col, repeat_tex, NO_ALPHA_BLEND, NO_DEPTH_TEST)

    

u32* get_texture_column(u32* texture, float wall_u);

#endif 