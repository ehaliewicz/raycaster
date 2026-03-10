
#include "common.h"
#include "collision.h"
#include "my_defs.h"
#include "resources.h"


float get_height_at_point(float px, float py, float pz, int return_ceil, int check_middle_sprite) {
    int map_x = my_floorf(px);
    int map_y = my_floorf(py);
    float subx = px - my_floorf(px);
    float suby = py - my_floorf(py);
    int in_top_left = subx < (1.0f-suby);//in_top && in_left;
    int in_top_right = subx >= suby; //in_top && in_right;
    int in_right = subx >= 0.5f;
    int in_top = suby < 0.5f;
    int map_idx = map_y*MAP_SIZE + map_x;
    level* this_level = &levels[cur_level_idx];
    cell_types floor_cell_type = this_level->lower_cell_types[map_idx];
    cell_types ceil_cell_type = this_level->upper_cell_types[map_idx];
    cell_types check_cell_type = return_ceil ? ceil_cell_type : floor_cell_type;
    int floor = this_level->floor[map_idx];
    int upper_floor = this_level->upper_floor[map_idx];
    int ceil = this_level->ceil[map_idx];
    int upper_ceil = this_level->upper_ceil[map_idx];
    float ret_val;

    if((check_cell_type == NE_TO_SW_DIAG && in_top_left) || 
       (check_cell_type == NW_TO_SE_DIAG && in_top_right) || 
        (check_cell_type == THIN_WALL_X && in_right) ||
        (check_cell_type == THIN_WALL_Y && in_top)) {
            ret_val = return_ceil ? this_level->upper_ceil[map_idx] : this_level->upper_floor[map_idx];
    } else if(check_cell_type == SLOPE_Y) {
        float first_height = floor;
        float second_height = upper_floor;
        if(return_ceil) {
            first_height = ceil;
            second_height = upper_ceil;
        }
        ret_val = first_height + (suby * (second_height - first_height));

    } else if (check_cell_type == SLOPE_X) { 
        float first_height = floor;
        float second_height = upper_floor;
        if(return_ceil) {
            first_height = ceil;
            second_height = upper_ceil;
        }
        ret_val = first_height + (subx * (second_height - first_height));

    } else if (check_cell_type == DOOR_Y) {
        if(this_level->parameter[map_idx] >= DOOR_FULLY_OPEN || suby >= 0.25f) {
            ret_val = return_ceil ? ceil : floor;
        } else {
            ret_val = return_ceil ? upper_ceil : upper_floor;
        }
    } else if (check_cell_type == DOOR_X) {
        if(this_level->parameter[map_idx] >= DOOR_FULLY_OPEN || subx >= 0.25f) {
            ret_val = return_ceil ? ceil : floor;
        } else {
            ret_val = return_ceil ? upper_ceil : upper_floor;
        }
    } else {
        ret_val = return_ceil ? ceil : floor;
    }
    if(check_middle_sprite && this_level->m_sprite_index[map_idx] != EMPTY_SPRITE_INDEX) {
        int mid_sprite_height = floor + this_level->m_sprite_offset[map_idx];
        if((!return_ceil) && mid_sprite_height > ret_val && mid_sprite_height <= pz) {
            ret_val = mid_sprite_height;
        } else if(return_ceil && mid_sprite_height > ret_val && mid_sprite_height >= pz) {
            ret_val = mid_sprite_height;
        }
    }
    return ret_val;
}

float get_height_at_point_for_sprites(float px, float py, int return_ceil) {
    return get_height_at_point(px, py, -1, // pz is unused unless you tell it to check against middle sprites
        0, 0
    );
}


int collides(
    float start_px, float start_py, float start_pz, float px, float py, float pz, level this_level,
    int disable_collision, int editor_mode_enabled) {
    if (disable_collision || editor_mode_enabled) { return 0; }
    if(px < 0 || px >= MAP_SIZE || py < 0 || py >= MAP_SIZE) { 
        return 1; 
    }
    
    int source_map_x = my_floorf(start_px);
    int source_map_y = my_floorf(start_py);
    int dst_map_x = my_floorf(px);
    int dst_map_y = my_floorf(py);
    int has_sprite = 0;

    int src_map_idx = source_map_y*MAP_SIZE+source_map_x;
    int dst_map_idx = dst_map_y*MAP_SIZE+dst_map_x;

    float sprite_pos = 0.0f;
    float src_floor_height = get_height_at_point(start_px, start_py, pz, 0, 1);
    float floor_height = get_height_at_point(px, py, pz, 0, 1);
    
    float ceil_height = get_height_at_point(px, py, pz, 1, 1);
    if(source_map_x > dst_map_x) {
        // if we've moved left
        if(levels[cur_level_idx].w_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = src_floor_height;
        } else if (levels[cur_level_idx].e_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = get_height_at_point(dst_map_x+0.999f, start_py, pz, 0, 0);
        }
    } else if (source_map_x < dst_map_x) {
        // moved right
        if(levels[cur_level_idx].e_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = src_floor_height;
        } else if (levels[cur_level_idx].w_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = get_height_at_point(dst_map_x, start_py, pz, 0, 0);

        }
    }
    if(source_map_y > dst_map_y) {
        // moved up
        if(levels[cur_level_idx].n_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = src_floor_height;

        } else if (levels[cur_level_idx].s_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = get_height_at_point(start_px, dst_map_y+0.99f, pz, 0, 0);

        }
    } else if (source_map_y < dst_map_y) {
        // moved down
        if (levels[cur_level_idx].s_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = src_floor_height;
        } else if (levels[cur_level_idx].n_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX) {
            has_sprite = 1;
            sprite_pos = get_height_at_point(start_px, dst_map_y, pz, 0, 0);
        }
    }

    if(has_sprite) {
        floor_height = sprite_pos + 8.0f;
    }
    if(ceil_height < player_z+2 || ceil_height < (floor_height + PLAYER_HEIGHT + 2)) {
        return 1;
    }
    if(floor_height >= (player_z-PLAYER_HEIGHT)+MAX_STEP_HEIGHT+0.001f) {
        return 1;
    }
    return 0;
}
