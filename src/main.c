#include <assert.h>
#if defined(PLATFORM_WEB)
#include "emscripten.h"
#include <emscripten/html5.h>
#endif
//#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_defs.h"
#include "platform_win.h"
//#include "raylib.h"

#include "common.h"
#include "entity.h"
#include "lz.h"
#include "network.h"
#include "raycast.h"

typedef enum {
    PIXEL_BUFFER = 0,
    EDITOR_BUFFER = 1,
    Z_BUFFER = 2,
} draw_mode;
#define NUM_RENDER_MODES 3

int editor_mode_enabled = 0;
int editor_selected_map_idx = -1;
editor_selected_thing editor_selected_side;

const int resolutions[NUM_RESOLUTIONS][2] = {
    {640, 480},
    {800, 600},
    {1024, 768},
    {1280, 1024},
    {1280, 720},
    {1920, 1080}
};
const int res_is_wide[NUM_RESOLUTIONS] = {
    0,0,0,0,1,1
};

int cur_render_res_idx = -1;
int requested_render_res = 3;
int cur_render_scale = 1;
int requested_render_scale = 1;
int cur_output_width;
int cur_output_height;
int cur_render_width;
int cur_render_height;
int use_vsync = 1;
int requested_use_vsync = 1;
int requested_fullscreen = 0;
int fullscreen = 0;
float cur_fov = 85.0f;


edit_wall_id *edit_id_buffer = NULL; //[FP_SCREEN_WIDTH*FP_SCREEN_HEIGHT];

void handle_click(int render_x, int render_y) {
    edit_wall_id id = edit_id_buffer[(FP_SCREEN_WIDTH-1-render_x)*FP_SCREEN_HEIGHT+(render_y)];
    editor_selected_map_idx = (id) & 0xFFFF; //id.cell_idx;

    editor_selected_side = (id>>16)&0xFF;// id.side;
}


//void log(char* )
//const char buf[80];
void console_log(const char *format, ...) {
    va_list arg;
    int cnt;

    //va_start(arg, format);
    //vsdebug_printf((char * __restrict__)buf, format, arg);
    //va_end(arg);
    // Analyze cnt and check for stream errors here
    return; //(uintmax_t)cnt;
}

void* my_malloc(long long unsigned int bytes, char* for_str) {
    debug_printf("Allocating %llu bytes for %s\n", bytes, for_str);
    return malloc(bytes);
}

void* my_calloc(long long unsigned int bytes, char* for_str) {
    debug_printf("Allocating %llu bytes for %s\n", bytes, for_str);
    return calloc(bytes, 1);
}



float running_time = 0.0f;

float get_running_time() {
    return running_time;
}


long long int rseed = 0x853c49e6748fea9bULL;

u32 lcg(long long int a, long long int inc) {
    u64 old_state = rseed;
    rseed = (a * rseed + inc);
    u32 xorshifted = ((old_state >> 18u) ^ old_state) >> 27u;
    u32 rot = old_state >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32 urand() {
    return lcg(6364136223846793005ULL, 0xda3e39cb94b95bdbULL);
}

// levels are what, 32x32 tile?

// textures are what... 32x32? :)



u32** textures;
u32* skybox;
u32** sprites;//[NUM_SPRITES];




level *levels = NULL;

float player_x;
float player_y;
float player_z;
float player_ang;
float pitch = 0;
int cur_level_idx;
int disable_collision = 0;



#define PLAYER_RADIUS (0.25f)

#define DOOR_FULLY_OPEN  200

float get_height_at_point(float px, float py, float pz, int return_ceil, int check_middle_sprite) {
    int map_x = px;
    int map_y = py;
    float subx = px - my_floorf(px);
    float suby = py - my_floorf(py);
    int in_top = suby < 0.5f;
    int in_left = subx < 0.5f;
    int in_right = !in_left;
    int in_top_left = in_top && in_left;
    int in_top_right = in_top && in_right;
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
       (check_cell_type == NW_TO_SE_DIAG && in_top_right)) {
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
    int map_x = px;
    int map_y = py;
    float subx = px - my_floorf(px);
    float suby = py - my_floorf(py);
    int in_top = suby < 0.5f;
    int in_left = subx < 0.5f;
    int in_right = !in_left;
    int in_top_left = in_top && in_left;
    int in_top_right = in_top && in_right;
    int map_idx = map_y*MAP_SIZE + map_x;
    level* this_level = &levels[cur_level_idx];
    cell_types floor_cell_type = this_level->lower_cell_types[map_idx];
    cell_types ceil_cell_type = this_level->upper_cell_types[map_idx];
    cell_types check_cell_type = return_ceil ? ceil_cell_type : floor_cell_type;
    int floor = this_level->floor[map_idx];
    int upper_floor = this_level->upper_floor[map_idx];
    int ceil = this_level->ceil[map_idx];
    int upper_ceil = this_level->upper_ceil[map_idx];

    if((check_cell_type == NE_TO_SW_DIAG && in_top_left) || 
       (check_cell_type == NW_TO_SE_DIAG && in_top_right)) {
            return return_ceil ? this_level->upper_ceil[map_idx] : this_level->upper_floor[map_idx];
    } else if(check_cell_type == SLOPE_Y) {
        float first_height = floor;
        float second_height = upper_floor;
        if(return_ceil) {
            first_height = ceil;
            second_height = upper_ceil;
        }
        float height = first_height + (suby * (second_height - first_height));

        return height; 
    } else if (check_cell_type == SLOPE_X) { 
        float first_height = floor;
        float second_height = upper_floor;
        if(return_ceil) {
            first_height = ceil;
            second_height = upper_ceil;
        }
        float height = first_height + (subx * (second_height - first_height));

        return height; 
    } else if (check_cell_type == DOOR_Y) {
        if(this_level->parameter[map_idx] >= DOOR_FULLY_OPEN || suby >= 0.25f) {
            return return_ceil ? ceil : floor;
        } else {
            return return_ceil ? upper_ceil : upper_floor;
        }
    } else {
        return return_ceil ? ceil : floor;
    }
}

int collides(float spx, float spy, float px, float py, float pz, level this_level) {
    if (disable_collision) { return 0; }
    if(editor_mode_enabled) { return 0; }
    
    int source_map_x = my_floorf(spx);
    int source_map_y = my_floorf(spy);
    int dst_map_x = my_floorf(px);
    int dst_map_y = my_floorf(py);
    int has_sprite = 0;

    int src_map_idx = source_map_y*MAP_SIZE+source_map_x;
    int dst_map_idx = dst_map_y*MAP_SIZE+dst_map_x;
    if(source_map_x > dst_map_x) {
        has_sprite = (levels[cur_level_idx].w_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) || 
         (levels[cur_level_idx].e_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX);
    } else if (source_map_x < dst_map_x) {
        has_sprite = (levels[cur_level_idx].e_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) || 
         (levels[cur_level_idx].w_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX);
    }
    if(source_map_y > dst_map_y) {
        has_sprite = (levels[cur_level_idx].n_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) || 
         (levels[cur_level_idx].s_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX);
    } else if  (source_map_y < dst_map_y) {
        has_sprite = (levels[cur_level_idx].s_sprite_index[src_map_idx] != EMPTY_SPRITE_INDEX) || 
         (levels[cur_level_idx].n_sprite_index[dst_map_idx] != EMPTY_SPRITE_INDEX);
    }

    float floor_height = get_height_at_point(px, py, pz, 0, 1);
    float ceil_height = get_height_at_point(px, py, pz, 1, 1);
    if(has_sprite) {
        floor_height += 8.0f;
    }
    if(ceil_height < player_z+2 || ceil_height < (floor_height + PLAYER_HEIGHT + 2)) {
        return 1;
    }
    if(floor_height >= (player_z-PLAYER_HEIGHT)+2.5) {
        return 1;
    }
    return 0;
}

int door_timer_running = 0;
int timer_door = 0; // the map idx of the door we're opening
float start_open_time; // one second to open, one second open, one second to close?


void update_player(float frame_time, Vector2 mouse_delta) {
    float y = my_sinf(-player_ang);
    float x = my_cosf(-player_ang);
    float strafe_right_x = -y;
    float strafe_right_y = x;
    float strafe_left_x = y;
    float strafe_left_y = -x;
    float move_speed = .04f * frame_time / 16.0f;
    level cur_level = levels[cur_level_idx];
    float r = PLAYER_RADIUS;

    int door_closing = 0;
    if(door_timer_running) {
        float cur_time = get_running_time();
        float open_time = cur_time-start_open_time;

        int int_open_amount = 0;
        if(open_time <= 1.0f) {
            int_open_amount = open_time*255.0f;
        } else if (open_time <= 2.0f) {
            int_open_amount = 255;
        } else if (open_time <= 3.0f) {
            door_closing = 1;
            int_open_amount = (1.0f-(open_time-2.0f))*255.0f;
        } else {
            door_closing = 1;
            door_timer_running = 0;
        }

        int_open_amount = CLAMP(int_open_amount, 0, 255);

        levels[cur_level_idx].parameter[timer_door] = int_open_amount;
        int pmx = player_x;
        int pmy = player_y;
        if(pmy*MAP_SIZE+pmx == timer_door) {
            // we are *INSIDE* the door
            if(door_closing) {
                float suby = player_y - my_floorf(player_y);
                if(suby >= 0.9f) {
                    player_y = my_floorf(player_y+1.0f)+PLAYER_RADIUS+0.1f;
                } else if(int_open_amount >= DOOR_FULLY_OPEN) {
                    float max_y_in_cell = (((float)int_open_amount-DOOR_FULLY_OPEN)/(255-DOOR_FULLY_OPEN));
                    //debug_printf("open %i max y %f\n", int_open_amount, max_y_in_cell);
                    if(suby > max_y_in_cell) {
                        player_y = my_floorf(player_y) + max_y_in_cell;

                        if((int)player_y != pmy) {
                            player_y = my_floorf(player_y)+(PLAYER_RADIUS-0.1f);
                        }
                    }
                } else {
                    player_y = my_floorf(player_y)- (PLAYER_RADIUS+0.1f);
                }
            }

        }
    }

    if(platform_is_key_down(KEY_ENTER)) {
        if(!door_timer_running) {
            int map_x = player_x;
            int map_y = player_y;
            int start_x = MAX(0, map_x-1);
            int end_x = MIN(MAP_SIZE-1, map_x+1);
            int start_y = MAX(0, map_y-1);
            int end_y = MIN(MAP_SIZE-1, map_y+1);
            for(int y = start_y; y <= end_y; y++) {
                for(int x = start_x; x <= end_x; x++) {
                    int map_idx = y*MAP_SIZE+x;
                    int lower_cell_type = levels[cur_level_idx].lower_cell_types[map_idx];
                    if(lower_cell_type == DOOR_Y || lower_cell_type == DOOR_X) {
                            door_timer_running = 1;
                            timer_door = map_idx;
                            start_open_time = get_running_time();
                    }
                }
            }
        }
    }

    if (platform_is_key_down(KEY_W)) {
        float vel_x = move_speed*x;
        float vel_y = move_speed*y;
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);

        if((collides(player_x, player_y, probe_x, player_y - r, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, probe_x, player_y + r, player_z, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x, player_y, player_x - r, probe_y, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, player_x + r, probe_y, player_z, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    if(platform_is_key_down(KEY_A)) {
        float vel_x = move_speed*strafe_left_x;
        float vel_y = move_speed*strafe_left_y;
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);
        if((collides(player_x, player_y, probe_x, player_y - r, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, probe_x, player_y + r, player_z, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x, player_y, player_x - r, probe_y, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, player_x + r, probe_y, player_z, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    if(platform_is_key_down(KEY_D)) {
        float vel_x = move_speed*strafe_right_x;
        float vel_y = move_speed*strafe_right_y;
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);
        if((collides(player_x, player_y, probe_x, player_y - r, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, probe_x, player_y + r, player_z, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x, player_y, player_x - r, probe_y, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, player_x + r, probe_y, player_z, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    if (platform_is_key_down(KEY_S)) {
        float vel_x = - move_speed*x;
        float vel_y = - move_speed*y; 
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);    
        if((collides(player_x, player_y, probe_x, player_y - r, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, probe_x, player_y + r, player_z, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x, player_y, player_x - r, probe_y, player_z, cur_level) == 0) && 
            (collides(player_x, player_y, player_x + r, probe_y, player_z, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    int map_x = my_floorf(player_x);
    int map_y = my_floorf(player_y);
    int map_idx = map_y*MAP_SIZE+map_x;
    int floor = levels[cur_level_idx].floor[map_idx];
    if(levels[cur_level_idx].lower_cell_types[map_idx] != NORMAL_CELL) {
        floor = MAX(floor, levels[cur_level_idx].upper_floor[map_idx]);
    }
    
    float lf_height = get_height_at_point(player_x-PLAYER_RADIUS, player_y, player_z, 0, 1);
    float rt_height = get_height_at_point(player_x+PLAYER_RADIUS, player_y, player_z, 0, 1);
    float tp_height = get_height_at_point(player_x, player_y-PLAYER_RADIUS, player_z, 0, 1);
    float bt_height = get_height_at_point(player_x, player_y+PLAYER_RADIUS, player_z, 0, 1);


    //float exact_height = get_height_at_point(player_x, player_y, 0);
    float player_contact_height = player_z-PLAYER_HEIGHT;
    float max_takeable_step = MAX(lf_height, MAX(rt_height, MAX(tp_height, bt_height)));
   
    float target_height = max_takeable_step+PLAYER_HEIGHT;
    if(editor_mode_enabled) {
        //if(target_height > player_z) {
        //    float dh = target_height-player_z;
        //    player_z += (target_height - player_z)*0.15;
        //}
    } else {
        //if(got_takeable_step) {
            player_z += (target_height - player_z)*0.15;
        //}
    }
    player_z = MIN(MAX_WALL_HEIGHT, player_z);

    if(platform_is_key_down(KEY_LEFT) || platform_is_key_down(KEY_U)) {
        player_ang += 0.0035f*frame_time;
    }
    if(platform_is_key_down(KEY_RIGHT) || platform_is_key_down(KEY_O)) {
        player_ang -= 0.0035f*frame_time;
    }
    if(!editor_mode_enabled) {
        if(mouse_delta.y != 0) {
            pitch -= .0015f*mouse_delta.y;
        }
        if(mouse_delta.x != 0) {
            player_ang -= mouse_delta.x*.0017f;
        }
    }
    // cleanup angle
    if(player_ang < 0.0f) {
        player_ang += 6.28f;
    } else if (player_ang > 6.28f) {
        player_ang -= 6.28f;
    }

    if (platform_is_key_down(KEY_I)) {
        pitch += .0015f*frame_time;
    } else if (platform_is_key_down(KEY_K)) {
        pitch -= .0015f*frame_time;
    } else if (platform_is_key_pressed(KEY_SPACE)) {
        pitch = 0;
    }
    pitch = CLAMP(pitch, -0.5f, 0.5f);

}

#define SCALE_FACTOR 32

void draw_topdown_level() {
    //level cur_level = levels[cur_level_idx];
    for(int y = 0; y < MAP_SIZE; y++) {
        for(int x = 0; x < MAP_SIZE; x++) {
            //DrawRectangle(x*SCALE_FACTOR, y*SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR, color_lut[cur_level.ttex[y*MAP_SIZE+x]]);
        }
    }
}


void copy_32_map_to_64() {
    // 
}


void init_level(int fresh_map) {
    player_ang =  levels[cur_level_idx].start_ang;
    player_x = levels[cur_level_idx].start_x;
    player_y = levels[cur_level_idx].start_y;
    player_z = levels[cur_level_idx].start_z;

    if(fresh_map) {  
        player_x = 16;
        player_y = 16;
        my_memset(levels, 0, sizeof(level)*NUM_LEVELS);
        for(int level = 0; level < NUM_LEVELS; level++) {
            for(int y = 0; y < MAP_SIZE; y++) {
                for(int x = 0; x < MAP_SIZE; x++) {
                    int idx = y*MAP_SIZE+x;
                    if(x == 0 || y == 0 || x == MAP_SIZE-1 || y == MAP_SIZE-1) { 
                        levels[level].ceil[idx] = MAX_WALL_HEIGHT/2;
                        levels[level].floor[idx] = MAX_WALL_HEIGHT/2;
                        levels[level].upper_ceil[idx] = MAX_WALL_HEIGHT/2;
                        levels[level].upper_floor[idx] = MAX_WALL_HEIGHT/2;
                    } else {
                        levels[level].ceil[idx] = MAX_WALL_HEIGHT;
                        levels[level].floor[idx] = 0;
                        levels[level].upper_ceil[idx] = MAX_WALL_HEIGHT;
                        levels[level].upper_floor[idx] = 0;

                        levels[level].ctex[idx] = SKYBOX_TEX_IDX;
                    }
                    levels[level].sprite_index[idx] = EMPTY_SPRITE_INDEX;
                    levels[level].n_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                    levels[level].e_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                    levels[level].s_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                    levels[level].w_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                    levels[level].m_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                    levels[level].parameter[idx] = 0;
                }
            }
            levels[level].start_x = 16;
            levels[level].start_y = 16;
        }
    }
    
    for(int i = 0; i < MAP_SIZE*MAP_SIZE; i++) {
        // clear floor sprites
        //levels[cur_level_idx].m_sprite_index[i] = EMPTY_SPRITE_INDEX;
        //levels[cur_level_idx].m_sprite_offset[i] = 12;
        //levels[cur_level_idx].c_sprite_index[i] = EMPTY_SPRITE_INDEX;
        //if(levels[cur_level_idx].upper_cell_types[i] == NORMAL_CELL && levels[cur_level_idx].ctex[i] == SKYBOX_TEX_IDX) {
        //    levels[cur_level_idx].ceil[i] = MAX_WALL_HEIGHT;
        //}

        // set anchors
        //levels[cur_level_idx].floor_anchor[i] = 0;
        //levels[cur_level_idx].ceil_anchor[i] = MAX_WALL_HEIGHT;

        if(levels[cur_level_idx].sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].sprite_index[i] = EMPTY_SPRITE_INDEX;
        }
        if(levels[cur_level_idx].n_sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].n_sprite_index[i] = EMPTY_SPRITE_INDEX;
        }
        if(levels[cur_level_idx].e_sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].e_sprite_index[i] = EMPTY_SPRITE_INDEX;
        }
        if(levels[cur_level_idx].s_sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].s_sprite_index[i] = EMPTY_SPRITE_INDEX;
        }
        if(levels[cur_level_idx].w_sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].w_sprite_index[i] = EMPTY_SPRITE_INDEX;
        }
        if(levels[cur_level_idx].f_sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].f_sprite_index[i] = EMPTY_SPRITE_INDEX;
        }
        if(levels[cur_level_idx].c_sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].c_sprite_index[i] = EMPTY_SPRITE_INDEX;
        }
        if(levels[cur_level_idx].m_sprite_index[i] >= NUM_SPRITES) {
            levels[cur_level_idx].m_sprite_index[i] = EMPTY_SPRITE_INDEX;
        }

        //levels[cur_level_idx].lntex[i] &= 0xF;
        //levels[cur_level_idx].letex[i] &= 0xF;
        //levels[cur_level_idx].lstex[i] &= 0xF;
        //levels[cur_level_idx].lwtex[i] &= 0xF;
        //levels[cur_level_idx].untex[i] &= 0xF;
        //levels[cur_level_idx].uetex[i] &= 0xF;
        //levels[cur_level_idx].ustex[i] &= 0xF;
        //levels[cur_level_idx].uwtex[i] &= 0xF;
        //levels[cur_level_idx].udtex[i] &= 0xF;
        //levels[cur_level_idx].ldtex[i] &= 0xF;
        //levels[cur_level_idx].ftex[i] &= 0xF;
        //levels[cur_level_idx].uftex[i] &= 0xF;
        //levels[cur_level_idx].ctex[i] &= 0xF;
        //levels[cur_level_idx].uctex[i] &= 0xF;

        //levels[cur_level_idx].m_sprite_index[i] = EMPTY_SPRITE_INDEX;
        
    }
    
    player_z = get_height_at_point(player_x, player_y, player_z, 0, 1) + PLAYER_HEIGHT;

}



void draw_player() {
    //float y = 15*my_sinf(player_ang);
    //float x = 15*my_cosf(player_ang);
    //DrawCircle(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 5, RED);
    //DrawLine(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 
    //    player_x*SCALE_FACTOR+x, player_y*SCALE_FACTOR+y, BLUE);
}

//Font font;

typedef enum {
    TEXTURE,
    SPRITE
} asset_type;
typedef struct {
    const char* name;
    const asset_type type;
} asset;

u32* camera_texture = NULL;


const char* texture_assets[] = {
    "flat_tex0",
    "flat_tex1",
    "wall_tex0",
    "wall_tex1",
    "bookshelf",
    "grass",
    "church"
};
const char* sprite_assets[] = {
    "tree",
    "moss",
    "chandelier",
    "glass_window2",
    "glass_window3",
    "fence",
    "bush",
    "glass_window4",
    "wall_tex0",
    "GATO1",
    "GATO2",
    "GATO3",
    "GATO4",
    "GATO5",
    "GATO6",
    "GATO7",
    "GATO8",
    "GATO9",
    "GATO10",
    "fox1",
    "fox2",
};

void load_resources() {
    //font = LoadFont("C:/Windows/Fonts/courbd.ttf");

    const int num_sprite_assets = ((sizeof(sprite_assets)) / sizeof(char*));
    const int num_texture_assets = ((sizeof(texture_assets)) / sizeof(char*));
    const int num_assets = num_sprite_assets+num_texture_assets;


    int tex_idx = 0;
    int sprite_idx = 0;
    size_t tex_num_bytes = sizeof(u8)*4*TEX_SIZE*TEX_SIZE;
    size_t tex_num_pixels = sizeof(u32)*TEX_SIZE*TEX_SIZE;
    
    textures = my_malloc(sizeof(u32*)*16, "texture pointer array");
    sprites = my_malloc(sizeof(u32*)*NUM_SPRITES, "sprite pointer array");

    //u32* backing_texture_data = my_calloc(sizeof(u32)*tex_num_pixels*(NUM_TEXTURES+NUM_SPRITES), "assets");
    char buf[64] = {'r','e','s','o','u','r','c','e','s','/'};
    for(int asset_idx = 0; asset_idx < num_assets; asset_idx++) {
        int is_texture = (asset_idx < num_texture_assets);
        const char* asset_name = is_texture ? texture_assets[asset_idx] : sprite_assets[asset_idx-num_texture_assets];
        int i = 0;
        while(asset_name[i] != '\0') { 
            buf[10+i] = asset_name[i]; i++;
        };
        buf[10+i++] = '.';
        buf[10+i++] = 't';
        buf[10+i++] = 'g';
        buf[10+i++] = 'a';
        buf[10+i++] = '\0';



        debug_printf("Loading %s...\n", buf);
        u8* tex_data = platform_load_image(buf, 32, 32);
        if(tex_data == NULL) {
            debug_printf("ERROR LOADING ASSET %s\n", buf);
            exit(1);
        }
        debug_printf("Loaded.\n");

        //u32* data_ptr = backing_texture_data+(tex_num_pixels*asset_idx);
        //my_memcpy(data_ptr, tex_data, tex_num_bytes);

        debug_printf("Copied\n");

        
        //platform_unload_image(tex_data);
        if(is_texture) {
            textures[tex_idx++] = (u32*)tex_data;//data_ptr;
        } else {
            sprites[sprite_idx++] = (u32*)tex_data;//data_ptr;
        }
    }

    camera_texture = my_calloc(32*32*sizeof(u32), "camera texture");
    textures[tex_idx++] = camera_texture;


    debug_printf("Loading skybox tga\n");
    u8* skybox_tex_data = platform_load_image("resources/skybox.tga", SKYBOX_TEX_HEIGHT, SKYBOX_TEX_WIDTH);
    //skybox = my_calloc(4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT, "skybox");
    //my_memcpy(skybox, skybox_tex_data, 4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT);
    textures[SKYBOX_TEX_IDX] = (u32*)skybox_tex_data;
    //platform_unload_image(skybox_tex_data);
    debug_printf("Loaded\n");
    //Image height_tex = LoadImage("resources/flat_tex_heightmap.tga");
    //my_memcpy(heightmap, height_tex.data, 32*32);
    //UnloadImage(height_tex);
}


void handle_editor() {
    int dy = 0;
    int dx = 0;
    if (platform_is_key_down(KEY_X)) {
        player_z -= 0.1f;
    } else if (platform_is_key_down(KEY_C)) {
        player_z += 0.1f;
    }
    int key = platform_get_key_pressed();
    if(key == 0) {
        return;
    }
    if(key == KEY_DOWN) {
        dy = -1;
    } else if (key == KEY_UP) {
        dy = 1;
    }
    if(key == KEY_LEFT) {
        dx = -1;
    } else if (key == KEY_RIGHT) {
        dx = 1;
    }
    if(dy != 0 || dx != 0) {
        u8* height_ptr = NULL;
        u8* anchor_ptr = NULL;
        u8* spr_ptr = NULL;
        cell_types lower_cell_type = levels[cur_level_idx].lower_cell_types[editor_selected_map_idx];
        cell_types upper_cell_type = levels[cur_level_idx].upper_cell_types[editor_selected_map_idx];
            switch(editor_selected_side) {
            case WALL_SIDE_BOTTOM:
                height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_map_idx];
                break;

            case WALL_SIDE_UPPER_NORTH: do {
                if(upper_cell_type != NORMAL_CELL) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                }
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_map_idx];
            } while(0);
                break;
            case WALL_SIDE_UPPER_EAST: do {
                if(upper_cell_type == NW_TO_SE_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                }
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_map_idx];
            } while(0);
                break;
            case WALL_SIDE_UPPER_SOUTH:
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_WEST: do {
                if(upper_cell_type == NE_TO_SW_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_map_idx];
                }
            } while(0);
                break;
            case WALL_SIDE_UPPER_DIAG:
            case WALL_SIDE_UPPER_BOTTOM:
                height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_map_idx];
                break;


            case WALL_SIDE_LOWER_NORTH: do {
                if(lower_cell_type != NORMAL_CELL) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                }
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_map_idx];
            } while(0);
                break;
            case WALL_SIDE_LOWER_EAST: do {
                if(lower_cell_type == NW_TO_SE_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                }
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_map_idx];
            } while(0);
                break;
            case WALL_SIDE_LOWER_SOUTH:
                height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_WEST: do {
                if(lower_cell_type == NE_TO_SW_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                }
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_map_idx];
            } while(0);
                break;


            case WALL_SIDE_TOP:   
                height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_TOP:  
            case WALL_SIDE_LOWER_DIAG:
                height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_map_idx];
                break;
            case FLOOR_SPRITE:
                spr_ptr = &levels[cur_level_idx].f_sprite_index[editor_selected_map_idx];
                break;
            case CEIL_SPRITE:
                spr_ptr = &levels[cur_level_idx].c_sprite_index[editor_selected_map_idx];
                break;
            case MIDDLE_SPRITE:
                spr_ptr = &levels[cur_level_idx].m_sprite_index[editor_selected_map_idx];
                height_ptr = &levels[cur_level_idx].m_sprite_offset[editor_selected_map_idx];
                break;
            case CELL_SPRITE:
                spr_ptr = &levels[cur_level_idx].sprite_index[editor_selected_map_idx];
                break;
            case N_SPRITE:
                spr_ptr = &levels[cur_level_idx].n_sprite_index[editor_selected_map_idx];
                break;
            case E_SPRITE:
                spr_ptr = &levels[cur_level_idx].e_sprite_index[editor_selected_map_idx];
                break;
            case S_SPRITE:
                spr_ptr = &levels[cur_level_idx].s_sprite_index[editor_selected_map_idx];
                break;
            case W_SPRITE:
                spr_ptr = &levels[cur_level_idx].w_sprite_index[editor_selected_map_idx];
                break;
        }
        if(anchor_ptr != NULL && platform_is_key_down(KEY_CONTROL)) {
            int nval = *anchor_ptr+dy;
            nval = CLAMP(nval, 0, MAX_WALL_HEIGHT);
            *anchor_ptr = nval;
        } else if(height_ptr != NULL) {
            int nval = *height_ptr+dy;
            nval = CLAMP(nval, 0, MAX_WALL_HEIGHT);
            *height_ptr = nval;
        }

        if(spr_ptr != NULL && (!platform_is_key_down(VK_SHIFT))) {

            if(dy == -1) { 
                if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side == CEIL_SPRITE)) {
                    // move to ceil position
                    editor_selected_side = MIDDLE_SPRITE;
                    levels[cur_level_idx].m_sprite_index[editor_selected_map_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side != FLOOR_SPRITE)) {
                    // move to middle position
                    editor_selected_side = FLOOR_SPRITE;
                    levels[cur_level_idx].f_sprite_index[editor_selected_map_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if ((!platform_is_key_down(KEY_CONTROL)) && editor_selected_side != N_SPRITE) {
                    editor_selected_side = N_SPRITE;
                    levels[cur_level_idx].n_sprite_index[editor_selected_map_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                }
            } else if (dy == 1) {
                 if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side == FLOOR_SPRITE)) {
                    // move to middle position
                    editor_selected_side = MIDDLE_SPRITE;
                    levels[cur_level_idx].m_sprite_index[editor_selected_map_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side != CEIL_SPRITE)) {
                    // move to floor position
                    editor_selected_side = CEIL_SPRITE;
                    levels[cur_level_idx].c_sprite_index[editor_selected_map_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if ((!platform_is_key_down(KEY_CONTROL)) && editor_selected_side != S_SPRITE) {
                    editor_selected_side = S_SPRITE;
                    levels[cur_level_idx].s_sprite_index[editor_selected_map_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                }
             } else if (dx == -1 && editor_selected_side != W_SPRITE) {
                editor_selected_side = W_SPRITE;
                levels[cur_level_idx].w_sprite_index[editor_selected_map_idx] = *spr_ptr;
                *spr_ptr = EMPTY_SPRITE_INDEX;
            } else if (dx == 1 && editor_selected_side != E_SPRITE) {
                levels[cur_level_idx].e_sprite_index[editor_selected_map_idx] = *spr_ptr;
                editor_selected_side = E_SPRITE;
                *spr_ptr = EMPTY_SPRITE_INDEX;
            }
        }

    } else if (key == KEY_P) {
        int idx = editor_selected_map_idx;
        //int y = idx / 32;
        //int x = (idx- (y*32));
        u8* spr_ptr = NULL;
        switch(editor_selected_side) {
            case MIDDLE_SPRITE:
                spr_ptr = &levels[cur_level_idx].m_sprite_index[idx];
                break;
            case CEIL_SPRITE:
                spr_ptr = &levels[cur_level_idx].c_sprite_index[idx];
                break;
            case FLOOR_SPRITE:
                spr_ptr = &levels[cur_level_idx].f_sprite_index[idx];
                break;
            case CELL_SPRITE:
            default:
                spr_ptr = &levels[cur_level_idx].sprite_index[idx];
                break;
            case N_SPRITE:
                spr_ptr = &levels[cur_level_idx].n_sprite_index[idx];
                break;
            case E_SPRITE:
                spr_ptr = &levels[cur_level_idx].e_sprite_index[idx];
                break;
            case S_SPRITE:
                spr_ptr = &levels[cur_level_idx].s_sprite_index[idx];
                break;
            case W_SPRITE:
                spr_ptr = &levels[cur_level_idx].w_sprite_index[idx];
                break;
        }

        if(*spr_ptr == EMPTY_SPRITE_INDEX) {
            *spr_ptr = 0;
        } else {
            *spr_ptr = EMPTY_SPRITE_INDEX;
        }

    } else if (key == KEY_T) {
        u8* type_ptr = NULL;
        switch(editor_selected_side) {
            case WALL_SIDE_BOTTOM:
            case WALL_SIDE_UPPER_NORTH:
            case WALL_SIDE_UPPER_EAST:
            case WALL_SIDE_UPPER_SOUTH:
            case WALL_SIDE_UPPER_WEST:
            case WALL_SIDE_UPPER_DIAG:
            case WALL_SIDE_UPPER_BOTTOM:
                type_ptr = &levels[cur_level_idx].upper_cell_types[editor_selected_map_idx];
                break;
            case WALL_SIDE_TOP:   
            case WALL_SIDE_LOWER_NORTH:
            case WALL_SIDE_LOWER_EAST:
            case WALL_SIDE_LOWER_SOUTH:
            case WALL_SIDE_LOWER_WEST:
            case WALL_SIDE_UPPER_TOP:  
            case WALL_SIDE_LOWER_DIAG:
                type_ptr = &levels[cur_level_idx].lower_cell_types[editor_selected_map_idx];
                break;
            default:
                break;
        }
        if(type_ptr != NULL) {
            u8 nval = *type_ptr + 1;
            if(nval >= NUM_CELL_TYPES) {
                nval = 0;
            }
            *type_ptr = nval;
        }
    } else if (key == KEY_L) {
        u8* light_ptr = NULL;
        switch(editor_selected_side) { 
            case WALL_SIDE_BOTTOM:
                light_ptr = &levels[cur_level_idx].c_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_BOTTOM:
                light_ptr = &levels[cur_level_idx].uc_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_TOP:
                light_ptr = &levels[cur_level_idx].f_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_TOP:
                light_ptr = &levels[cur_level_idx].uf_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_NORTH:
                light_ptr = &levels[cur_level_idx].ln_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_NORTH:
                light_ptr = &levels[cur_level_idx].un_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_EAST:
                light_ptr = &levels[cur_level_idx].le_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_EAST:
                light_ptr = &levels[cur_level_idx].ue_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_SOUTH:
                light_ptr = &levels[cur_level_idx].ls_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_SOUTH:
                light_ptr = &levels[cur_level_idx].us_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_WEST:
                light_ptr = &levels[cur_level_idx].lw_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_WEST:
                light_ptr = &levels[cur_level_idx].uw_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_DIAG:
                light_ptr = &levels[cur_level_idx].ud_light[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_DIAG:
                light_ptr = &levels[cur_level_idx].ld_light[editor_selected_map_idx];
                break;
            default:
                break;
        }
        if(light_ptr != NULL) {
            *light_ptr = *light_ptr+1;
            if(*light_ptr >= NUM_LIGHT_LEVELS) {
                *light_ptr = 0;
            }
        }
    } else if ((key == KEY_R) || (key >= KEY_KP_0 && key <= KEY_KP_9) || (key >= '0' && key <= '9')) {
        u8* tex_ptr = NULL;
        u8* spr_ptr = NULL;
        switch(editor_selected_side) {
            case WALL_SIDE_BOTTOM:
                tex_ptr = &levels[cur_level_idx].ctex[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_BOTTOM:
                tex_ptr = &levels[cur_level_idx].uctex[editor_selected_map_idx];
                break;
            case WALL_SIDE_TOP:
                tex_ptr = &levels[cur_level_idx].ftex[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_TOP:
                tex_ptr = &levels[cur_level_idx].uftex[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_NORTH:
                tex_ptr = &levels[cur_level_idx].lntex[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_NORTH:
                tex_ptr = &levels[cur_level_idx].untex[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_EAST:
                tex_ptr = &levels[cur_level_idx].letex[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_EAST:
                tex_ptr = &levels[cur_level_idx].uetex[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_SOUTH:
                tex_ptr = &levels[cur_level_idx].lstex[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_SOUTH:
                tex_ptr = &levels[cur_level_idx].ustex[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_WEST:
                tex_ptr = &levels[cur_level_idx].lwtex[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_WEST:
                tex_ptr = &levels[cur_level_idx].uwtex[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_DIAG:
                tex_ptr = &levels[cur_level_idx].udtex[editor_selected_map_idx];
                break;
            case WALL_SIDE_LOWER_DIAG:
                tex_ptr = &levels[cur_level_idx].ldtex[editor_selected_map_idx];
                break;
            case MIDDLE_SPRITE:
                spr_ptr = &levels[cur_level_idx].m_sprite_index[editor_selected_map_idx];
                break;
            case CEIL_SPRITE:
                spr_ptr = &levels[cur_level_idx].c_sprite_index[editor_selected_map_idx];
                break;
            case FLOOR_SPRITE:
                spr_ptr = &levels[cur_level_idx].f_sprite_index[editor_selected_map_idx];
                break;
            case CELL_SPRITE:
                spr_ptr = &levels[cur_level_idx].sprite_index[editor_selected_map_idx];
                break;
            case N_SPRITE:
                spr_ptr = &levels[cur_level_idx].n_sprite_index[editor_selected_map_idx];
                break;
            case E_SPRITE:
                spr_ptr = &levels[cur_level_idx].e_sprite_index[editor_selected_map_idx];
                break;
            case S_SPRITE:
                spr_ptr = &levels[cur_level_idx].s_sprite_index[editor_selected_map_idx];
                break;
            case W_SPRITE:
                spr_ptr = &levels[cur_level_idx].w_sprite_index[editor_selected_map_idx];
                break;
        }
        if(tex_ptr != NULL) {
            if (key == KEY_R) {
                u8 ntex_idx = ((*tex_ptr)&0xF)+1;
                if(ntex_idx > SKYBOX_TEX_IDX) {
                    ntex_idx = 0;
                }
                if(ntex_idx >= NUM_TEXTURES) {
                    ntex_idx = SKYBOX_TEX_IDX;
                }
                *tex_ptr &= 0xF0;
                *tex_ptr |= ntex_idx;
            } else {
                u8 ntex_idx = ((*tex_ptr)&0xF);
                for(int i = 0; i < NUM_TEXTURES; i++) {
                    if ((key == KEY_KP_0+i) || (key == ('0'+i))) {
                        ntex_idx = i;
                        *tex_ptr &= 0xF0;
                        *tex_ptr |= ntex_idx;
                        break;
                    }
                }
            }
        }
        if(spr_ptr != NULL && key == KEY_R) {
            if(*spr_ptr == NUM_SPRITES-1) {
                *spr_ptr = EMPTY_SPRITE_INDEX;
            } else if (*spr_ptr == EMPTY_SPRITE_INDEX) {
                *spr_ptr = 0;
            } else {
                *spr_ptr = *spr_ptr+1;
            }
        }
    }
}

#ifdef CRT
float row_lum_buf1[1921], row_lum_buf2[1921];

void crt_shader(u32* fb) {
    float *prev_row_buf = row_lum_buf1;
    float *cur_row_buf = row_lum_buf2;

    for(int x = 0; x < FP_SCREEN_WIDTH; x++) {
        prev_row_buf[x] = 1.0f;
    }

    for(int y = 0; y < FP_SCREEN_HEIGHT; y++) {
        if((y&1) == 0) {
            for(int x = 0; x < FP_SCREEN_WIDTH; x++) {
                fb[x*FP_SCREEN_HEIGHT+y] = 0xFF000000;
            }
        } else {
            cur_row_buf[0] = 1.0f;
            for(int x = 0; x < FP_SCREEN_WIDTH; x++) {
                u32 pix = fb[x*FP_SCREEN_HEIGHT+y];
                u32 up_pix = fb[x*FP_SCREEN_HEIGHT+y-1];
                float r = ((pix>>16)&0xFF)/255.0f;
                float g = ((pix>>8)&0xFF)/255.0f;
                float b = ((pix>>0)&0xFF)/255.0f;
                float up_r = ((up_pix>>16)&0xFF)/255.0f;
                float up_g = ((up_pix>>8)&0xFF)/255.0f;
                float up_b = ((up_pix>>0)&0xFF)/255.0f;
                float quarter_up_lum = prev_row_buf[x+1];
                float quarter_ul_lum = prev_row_buf[x];
                float quarter_l_lum = cur_row_buf[x];
                r = (r*0.75f) + (up_r*0.75f);
                g = (g*0.75f) + (up_g*0.75f);
                b = (b*0.75f) + (up_b*0.75f);

                float cur_lum = (0.2126*r + 0.7152*g + 0.0722*b)*1.50f;
                float quarter_cur_lum = cur_lum * 0.25f;
                float actual_lum = CLAMP((quarter_up_lum + quarter_ul_lum + quarter_l_lum + quarter_cur_lum)/cur_lum, 0.0f, 1.0f);
                r *= actual_lum;
                g *= actual_lum;
                b *= actual_lum;
                u32 intr = (r*255.0f);
                u32 intg = (g*255.0f);
                u32 intb = (b*255.0f);

                cur_row_buf[x+1] = quarter_cur_lum;
                fb[x*FP_SCREEN_HEIGHT+y] = 0xFF000000 | (intr<<16) | (intg << 8) | (intb << 0);
            }
            float* tmp = cur_row_buf;
            prev_row_buf = cur_row_buf;
            cur_row_buf = tmp;
        }
    }
}
#endif

//Image draw_img;
//Texture2D draw_tex;
int draw_tex;
int frame;

u32* draw_pix = NULL;
float* z_buffer = NULL;
void change_resolution() {
    cur_render_res_idx = requested_render_res;
    cur_render_scale = requested_render_scale;
    cur_output_width = resolutions[cur_render_res_idx][0];
    cur_output_height = resolutions[cur_render_res_idx][1];
    cur_render_width = cur_output_width / cur_render_scale;
    cur_render_height = cur_output_height / cur_render_scale;
    cur_fov = res_is_wide[cur_render_res_idx] ? 100.0f : 85.0f;
    
    int prev_use_vsync = use_vsync;
    int prev_fullscreen = fullscreen;
    use_vsync = requested_use_vsync;
    fullscreen = requested_fullscreen;
    if(prev_use_vsync != use_vsync) {
        platform_set_vsync(use_vsync);
    } 

    //SetConfigFlags(FLAG_VSYNC_HINT);


    platform_set_window_size(OUTPUT_WIDTH, OUTPUT_HEIGHT);

    if(prev_fullscreen != fullscreen) {
        if(fullscreen) {
            platform_set_fullscreen();
        } else {
            platform_set_windowed();
        }
    }
    if(draw_pix != NULL) {
        free(draw_pix);
        free(edit_id_buffer);
        //UnloadImage(draw_img);
        //UnloadTexture(draw_tex);
    }
    draw_pix = my_malloc(sizeof(u32)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "framebuffer");
    edit_id_buffer = my_malloc(sizeof(edit_wall_id)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "edit-buffer");
    z_buffer = my_malloc(sizeof(float)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "z-buffer");

    /*
    draw_img = (Image){
        .data = draw_pix,
        .width = FP_SCREEN_HEIGHT,
        .height = FP_SCREEN_WIDTH,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    draw_tex = LoadTextureFromImage(draw_img);
    */
    draw_tex = platform_create_texture(FP_SCREEN_HEIGHT, FP_SCREEN_WIDTH);

    // janky framebuffer texture :)
    textures[NUM_TEXTURES-1] = draw_pix;
}


void* udp_conn = NULL;

float prev_frame_time = 0;
draw_mode render_mode = PIXEL_BUFFER;


float skybox_u_offset;

float get_abs_time() {
#ifdef PLATFORM_WEB 
    return emscripten_get_now()/1000.0f;
#else
    return platform_get_time();
#endif
}

void run_game() {
#ifndef PLATFORM_WEB
    if(!platform_is_window_focused()) {
        platform_begin_drawing();
        platform_end_drawing();
        return;
    }
#endif
    float frame_start_time = get_abs_time();

    Vector2 mouse_delta = platform_get_mouse_delta();

#ifdef PLATFORM_WEB
    if(editor_mode_enabled) {
        emscripten_exit_pointerlock();
        ShowCursor();
        
    } else {
        emscripten_request_pointerlock("#canvas", EM_TRUE);
    }
#else
    if(editor_mode_enabled) {
        platform_show_cursor();
    } else {
        platform_hide_cursor();
        platform_set_mouse_position(OUTPUT_WIDTH/2, OUTPUT_HEIGHT/2);
    }
#endif


    if(platform_is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_pos = platform_get_mouse_position();
        handle_click(FP_SCREEN_WIDTH*mouse_pos.x/OUTPUT_WIDTH, FP_SCREEN_HEIGHT*mouse_pos.y/OUTPUT_HEIGHT);
    }
    if (platform_is_key_pressed(KEY_E)) {
        editor_mode_enabled = !editor_mode_enabled;
    }
    if (platform_is_key_pressed(KEY_Z)) {
        render_mode++;
        if(render_mode >= NUM_RENDER_MODES) {
            render_mode = 0;
        }
    }


    if(editor_mode_enabled) {
        // clear editor buffer?
        
        handle_editor();
    } else if (platform_is_key_pressed(KEY_R)) {
        
        if(platform_is_key_down(KEY_SHIFT)) { // | platform_is_key_down(KEY_RSHIFT)) {
            requested_render_scale <<= 1;
            if(requested_render_scale > 4) {
                requested_render_scale = 1;
            }
        } else {
            requested_render_res++;
            if(requested_render_res >= NUM_RESOLUTIONS) {
                requested_render_res = 0;
            }
        }
        
    } else if (platform_is_key_pressed(KEY_V)) {
        requested_use_vsync = !requested_use_vsync;
    } else if (platform_is_key_pressed(KEY_F)) {
        requested_fullscreen = !requested_fullscreen;
    }

    float frame_time_ms = platform_get_frame_time()*1000.0f;
    if(requested_render_res != cur_render_res_idx || requested_render_scale != cur_render_scale || requested_use_vsync != use_vsync || requested_fullscreen != fullscreen) {
        float prev_pitch = pitch;
        change_resolution();
        pitch = prev_pitch;
    } else {
        update_player(frame_time_ms, mouse_delta);
    }

    static float last_other_player_x, last_other_player_y, last_other_player_z;
    static int got_other_player_pos;
    // send and receive every 30 frames
    float position[4] = {player_x, player_y, player_z, player_ang};
    float other_position[4];
    //if(udp_frame(udp_conn, position, other_position, 1, ((frame&0b11)==0))) {
    //    got_other_player_pos = 1;
    //    last_other_player_x = other_position[0];
    //    last_other_player_y = other_position[1];
    //    last_other_player_z = other_position[2];
    //    // we got a packet baybee
    //}

    

    step_entities(player_x, player_y, player_z);
    if(got_other_player_pos) {
        request_draw_sprite(last_other_player_x, last_other_player_y, last_other_player_z-PLAYER_HEIGHT, 20);
    }

    float seconds = get_running_time();
    float quarter_seconds = seconds*4;
    int iquarter_seconds = quarter_seconds;

    skybox_u_offset = (seconds*2); // scrolls every 2 seconds

    //skybox_u_offset &= SKYBOX_TEX_WIDTH-1;
    int flash_frame = iquarter_seconds&0b1;
    platform_begin_drawing(); {   
        //if(sixteenth_seconds&1) {
        //    
        //}

        for(int i = 0; i < FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH; i++) {
            z_buffer[i] = DARK_DIST;
        }
        //my_memset(draw_img.data, 0xFFFFFFFF, FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH*4);
        //z_buffer[(screen_x*FP_SCREEN_HEIGHT+y)] = 1024.0f;

        // fixes a bug at angle zero
        // the DDA algorithm had equal distance for both x and y steps
        // and picking one by the normal rules causes a bug in certain places
        // this removes any need for special casing inside the raycast function
        //if(fabsf(player_ang) < 0.01f) {
        //    player_ang = 0.01f;
        //}

        draw_first_person_level(draw_pix, edit_id_buffer, z_buffer,
            0, FP_SCREEN_WIDTH, flash_frame, 
            &levels[cur_level_idx], player_x, player_y, player_z, -player_ang, pitch,
            editor_mode_enabled, editor_selected_map_idx, editor_selected_side
        );

        switch(render_mode) {
            case EDITOR_BUFFER:             

                platform_update_texture(draw_tex, (u32*)edit_id_buffer, FP_SCREEN_HEIGHT, FP_SCREEN_WIDTH);
                break;
            case PIXEL_BUFFER:
                //if(platform_is_key_down(KEY_X)) {
                //    crt_shader(draw_pix);
                //}
                platform_update_texture(draw_tex, (u32*)draw_pix, FP_SCREEN_HEIGHT, FP_SCREEN_WIDTH);
                break;
            case Z_BUFFER:

                for(int i = 0; i < FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH; i++) {
                    float z = z_buffer[i];
                    float normalized = (z-NEAR_PLANE_DIST)/(DARK_DIST-NEAR_PLANE_DIST);
                    int byte_z = normalized*255;
                    ((u32*)z_buffer)[i] = (0xFF000000 | (byte_z<<16) | (byte_z<<8) | byte_z);
                }
                platform_update_texture(draw_tex, (u32*)z_buffer, FP_SCREEN_HEIGHT, FP_SCREEN_WIDTH);
                break;

        }
        float scale = ((float)OUTPUT_WIDTH/((float)FP_SCREEN_WIDTH));
        //platform_draw_texture(draw_tex, (Vector2){.x=OUTPUT_WIDTH,.y=0}, 90.0f, scale, FP_SCREEN_HEIGHT, FP_SCREEN_WIDTH);
        platform_draw_texture(draw_tex, (Vector2){.x=OUTPUT_WIDTH/2,.y=OUTPUT_HEIGHT/2}, 90.0f, scale, FP_SCREEN_HEIGHT, FP_SCREEN_WIDTH);


        float avg_frame_time = (prev_frame_time + frame_time_ms)/2.0f;
        prev_frame_time = frame_time_ms;
        //char buf[80]; 
        //debug_printf(buf, "%i %i -> %i %i FOV%.0f %s", cur_render_width, cur_render_height, cur_output_width, cur_output_height, cur_fov, use_vsync ? "vsync" : "");
        //platform_draw_text(buf, (Vector2){.x = 5, .y = 5}, 18, 1, RED);
        //platform_draw_text(buf, (Vector2){.x = 5, .y = 20}, 18, 1, RED);
        //debug_printf(buf, "%.2f %.2f %.2f %.2f\n", player_x, player_y, player_z, player_ang*RAD2DEG);
        //platform_draw_text(buf, (Vector2){.x = 5, .y = 35}, 18, 1, RED);
        //debug_printf("p %f %f %f\n", player_x, player_y, player_z);
        debug_printf("%4.0f fps\n", 1000.0f/avg_frame_time);
    } platform_end_drawing();

    int scale_y = FP_SCREEN_HEIGHT/32;
    int scale_x = FP_SCREEN_WIDTH/32;
    


    if(0) {
        for(int y = 0; y < 32; y++) {
            for(int x = 0; x < 32; x++) {
                int cr = 0;
                int cg = 0;
                int cb = 0;
                for(int sy = 0; sy < scale_y; sy++) {
                    for(int sx = 0; sx < scale_x; sx++) {
                        int fb_y = y*scale_y+sy;
                        int fb_x = x*scale_x+sx;
                        u32 sample = draw_pix[fb_x*FP_SCREEN_HEIGHT+fb_y];
                        float r = ((sample>>16)&0xFF);
                        float g = ((sample>>8)&0xFF);
                        float b = ((sample>>0)&0xFF);
                        cr += r;
                        cg += g;
                        cb += b;
                    }
                }
                cr /= (scale_y*scale_x);
                cg /= (scale_y*scale_x);
                cb /= (scale_y*scale_x);
                u32 intr = CLAMP(cr, 0, 255);
                u32 intg = CLAMP(cg, 0, 255);
                u32 intb = CLAMP(cb, 0, 255);

                camera_texture[((31-x)*32)+y] = (intr<<16)|(intg<<8)|(intb<<0);
            }
        }
    }
    
    
    

    frame++;
    float frame_end_time = get_abs_time();
    running_time += (frame_end_time - frame_start_time);
}


#define MAP_SAVE_FILE "map_save"
void init_game() {

    load_resources();
    
    int num_loaded_bytes;
    u8* loaded_bytes = platform_load_file_data(MAP_SAVE_FILE, &num_loaded_bytes);
    if(num_loaded_bytes == sizeof(level)*NUM_LEVELS) {
        levels = (level*)loaded_bytes;
        init_level(0);
    } else if(num_loaded_bytes > 0) {
        compressed* comp = (compressed*)loaded_bytes;

        if(comp->uncompressed_size != sizeof(level)*NUM_LEVELS) {
            //puts("error loading map!!!!!\n");
            if(num_loaded_bytes == sizeof(level)*NUM_LEVELS) {

            }
            //exit(1);
        }
        //u8* decompressed = decompress(comp);
        levels = (level*)decompress(comp);
        //levels = my_malloc(sizeof(level)*NUM_LEVELS, "level data");
        //my_memcpy(levels, decompressed, comp->uncompressed_size);
        //free(decompressed);
        debug_printf("Loaded map data\n");
        init_level(0);
        //levels = (level*)decompress(comp);
    } else {
        levels = my_malloc(sizeof(level)*NUM_LEVELS, "level data");
        init_level(1);
    }
    //levels = my_malloc(sizeof(level)*NUM_LEVELS, "levels");


    //init_level(1);
    //if(num_loaded_bytes == sizeof(level)*NUM_LEVELS) {
        //my_memcpy(levels, loaded_bytes, num_loaded_bytes); //sizeof(level)*NUM_LEVELS);
    //} else {
    //    puts("Initializing new map data");
    //    init_level(1);
    //}     
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow);


#ifdef DEBUG
int main(int argc, char** argv) {
#else
int atexit(void (*fn)(void)) { return 0; }
int mainCRTStartup(void) { 
#endif
    HINSTANCE hInst = GetModuleHandleA(NULL);
    ExitProcess(WinMain(hInst, NULL, NULL, SW_SHOW));
//int main(int argc, char** argv) {
    /*
    if(argc > 1) {
        if(strcmp(argv[1], "--client") == 0) {
            if(argc != 3) {
                debug_printf("use `raycast.exe --client [ip]`\n");
                exit(1);
            }
            udp_conn = setup_udp(argv[2], 0);
        } else if (strcmp(argv[1], "--server") ==0) {
            udp_conn = setup_udp("", 1);;
        }
    }
    */

    //debug_printf("SIZEOF LEVELS %zu\n", sizeof(levels));
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    platform_init_threadpool(NUM_THREADS);
    init_raycast_module();
    init_entities_module();
    init_game();
    platform_init_window(OUTPUT_WIDTH, OUTPUT_HEIGHT, "raycaster");
    //change_resolution();
    frame = 0;
    int entities_woke = 0;

    platform_hide_cursor();
#ifdef PLATFORM_WEB
    emscripten_set_main_loop(run_game, 0, 1);
#else 

    if(0) {
        int ticks = 1;
        for(int x = 1; x < 16; x++) {
            for(int y = 1; y < 31; y++) {
                spawn_entity(FOX, x, y, 8.5f, 0.0f, ticks++);
                spawn_entity(FOX, x+0.5f, y, 8.5f, 0.0f, ticks++);
                spawn_entity(FOX, x, y+0.5f, 8.5f, 0.0f, ticks++);
                spawn_entity(FOX, x+0.5f, y+0.5f, 8.5f, 0.0f, ticks++);
            }
        }
    }
    
    while(!platform_window_should_close()) {
        if(platform_is_key_pressed(KEY_B)) {
            //spawn_entity(FOX, player_x, player_y, player_z, 0, 2);
        }
        if(platform_is_key_pressed(KEY_ENTER) && !entities_woke) {
            wakeup_entities(player_x, player_y, player_z);
            entities_woke = 1;
        }
        if(frame == 0) {
            //spawn_entity(GATO, 12, 12, 13.5, 0);
            //spawn_entity(FOX, 12, 13, 13.5, 0);
        } else if (frame == 10) {
            //spawn_entity(FOX, 3, 14, 15.5, 0);
            //spawn_entity(GATO, 3, 13, 13.5, 0);
        } else if (frame == 20) {
            //spawn_entity(GATO, 3, 24, 15.5, 0);
            //spawn_entity(FOX, 3, 23, 15.5, 0);
        }else if (frame == 30) {
            //spawn_entity(FOX, 14, 14, 15.5, 0);
            //spawn_entity(GATO, 13, 14, 15.5, 0);
        }
        run_game();
    }
#endif


    levels[cur_level_idx].start_x = player_x;
    levels[cur_level_idx].start_y = player_y;
    levels[cur_level_idx].start_z = player_z;
    levels[cur_level_idx].start_ang = player_ang;



    size_t level_size = sizeof(level)*NUM_LEVELS;
    u8* level_data = (u8*)levels;
    //compressed* comp = compress(level_data, sizeof(level)*NUM_LEVELS);
    //size_t comp_size_bytes = ((comp->num_opcodes+7)>>3)+comp->num_operand_bytes;
    //debug_printf("Compressed %llu down to %llu bytes\n", comp->uncompressed_size, sizeof(compressed)+comp_size_bytes);

    //u8* decomp = decompress(comp);

    //for(size_t i = 0; i < level_size; i++) {
    //    if(level_data[i] != decomp[i]) {
    //        debug_printf("miscompare at %i\n", i);
    //        exit(1);
    //    }
    //}

    //if(!platform_save_file_data(MAP_SAVE_FILE, comp, sizeof(compressed)+comp_size_bytes)) {
    if(!platform_save_file_data(MAP_SAVE_FILE, levels, sizeof(level)*NUM_LEVELS)) {
        debug_printf("Error saving file :(\n");
    }



}