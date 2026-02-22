#include "assert.h"
#if defined(PLATFORM_WEB)
#include "emscripten.h"
#include <emscripten/html5.h>
#endif
#include "math.h"   
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

#include "common.h"

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

int resolutions[NUM_RESOLUTIONS][2] = {
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
int requested_render_res = 2;
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

void* my_malloc(long long unsigned int bytes, char* for_str) {
    printf("Allocating %llu bytes for %s\n", bytes, for_str);
    return malloc(bytes);
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


Color color_lut[5] = {
    {255,255,255,255},
    {255,0,0,255},
    {0,255,0,255},
    {0,255,255,255},
    {255,0,255,255},
};

u8* flat_textures[6];

u8* textures[16];
u8* decals[NUM_DECALS];
u8* skybox;
u8* sprites[NUM_SPRITES];



level levels[1] = {
    {
        .start_x = 2,
        .start_y = 2,
        .start_z = 2,
    }
};

level levels[1];

float player_x;
float player_y;
float player_z;
float player_ang;
float pitch = 0;
int cur_level_idx;
int disable_collision = 0;



#define PLAYER_RADIUS (0.25f)

#define DOOR_FULLY_OPEN  200

float get_height_at_point(float px, float py, int return_ceil) {
    int map_x = px;
    int map_y = py;
    float subx = px - floorf(px);
    float suby = py - floorf(py);
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

int collides(float px, float py, level this_level) {
    if (disable_collision) { return 0; }
    if(editor_mode_enabled) { return 0; }
    //int x = px;
    //int y = py;
    //int idx = y*MAP_SIZE+x;

    float floor_height = get_height_at_point(px, py, 0);
    float ceil_height = get_height_at_point(px, py, 1);

    //if(this_level.lower_cell_types[idx] == NE_TO_SW_DIAG || this_level.lower_cell_types[idx] == NW_TO_SE_DIAG) {
    //    floor = MAX(floor, this_level.upper_floor[idx]);
    //};
    //int ceil = this_level.ceil[idx];
    //if(this_level.lower_cell_types[idx] == NE_TO_SW_DIAG || this_level.upper_cell_types[idx] == NW_TO_SE_DIAG) {
    //    ceil = MIN(ceil, this_level.upper_ceil[idx]);
    //}
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
    float y = sin(-player_ang);
    float x = cos(-player_ang);
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
        levels[cur_level_idx].parameter[timer_door] = int_open_amount;
        int pmx = player_x;
        int pmy = player_y;
        if(pmy*MAP_SIZE+pmx == timer_door) {
            // we are *INSIDE* the door
            if(door_closing) {
                float suby = player_y - floorf(player_y);
                if(suby >= 0.9f) {
                    player_y = floorf(player_y+1.0f)+PLAYER_RADIUS+0.1f;
                } else if(int_open_amount >= DOOR_FULLY_OPEN) {
                    float max_y_in_cell = (((float)int_open_amount-DOOR_FULLY_OPEN)/(255-DOOR_FULLY_OPEN));
                    printf("open %i max y %f\n", int_open_amount, max_y_in_cell);
                    if(suby > max_y_in_cell) {
                        player_y = floorf(player_y) + max_y_in_cell;

                        if((int)player_y != pmy) {
                            player_y = floorf(player_y)+(PLAYER_RADIUS-0.1f);
                        }
                    }
                } else {
                    player_y = floorf(player_y)- (PLAYER_RADIUS+0.1f);
                }
            }

                //player_y = floorf(player_y)-(PLAYER_RADIUS+.1);
                //float lerped_close_position = (float)int_open_amount/DOOR_FULLY_OPEN;
                //printf("PUSH PLAYER OUT! door y pos: %f\n", lerped_close_position);
                //float max_y_pos = MAX(0.0f, lerped_close_position-1.0f);
                //if(player_y > max_y_pos) {
                //    player_y = max_y_pos;
                //}
        }
    }

    if(IsKeyDown(KEY_ENTER)) {
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
                    if(levels[cur_level_idx].lower_cell_types[map_idx] == DOOR_Y) {
                            door_timer_running = 1;
                            timer_door = map_idx;
                            start_open_time = get_running_time();
                    }
                }
            }
        }
    }



    if (IsKeyDown(KEY_W)) {
        float vel_x = move_speed*x;
        float vel_y = move_speed*y;
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);

        if((collides(probe_x, player_y - r, cur_level) == 0) && 
            (collides(probe_x, player_y + r, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x - r, probe_y, cur_level) == 0) && 
            (collides(player_x + r, probe_y, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    if(IsKeyDown(KEY_A)) {
        float vel_x = move_speed*strafe_left_x;
        float vel_y = move_speed*strafe_left_y;
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);
        if((collides(probe_x, player_y - r, cur_level) == 0) && 
            (collides(probe_x, player_y + r, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x - r, probe_y, cur_level) == 0) && 
            (collides(player_x + r, probe_y, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    if(IsKeyDown(KEY_D)) {
        float vel_x = move_speed*strafe_right_x;
        float vel_y = move_speed*strafe_right_y;
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);
        if((collides(probe_x, player_y - r, cur_level) == 0) && 
            (collides(probe_x, player_y + r, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x - r, probe_y, cur_level) == 0) && 
            (collides(player_x + r, probe_y, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    if (IsKeyDown(KEY_S)) {
        float vel_x = - move_speed*x;
        float vel_y = - move_speed*y; 
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);    
        if((collides(probe_x, player_y - r, cur_level) == 0) && 
            (collides(probe_x, player_y + r, cur_level) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x - r, probe_y, cur_level) == 0) && 
            (collides(player_x + r, probe_y, cur_level) == 0)) {
            player_y = new_y;
        }
    }
    int map_x = (int)player_x;
    int map_y = (int)player_y;
    int floor = levels[cur_level_idx].floor[map_y*MAP_SIZE + map_x];
    if(levels[cur_level_idx].lower_cell_types[map_y*MAP_SIZE+map_x] != NORMAL_CELL) {
        floor = MAX(floor, levels[cur_level_idx].upper_floor[map_y*MAP_SIZE + map_x]);
    }
    
    float lf_height = get_height_at_point(player_x-PLAYER_RADIUS, player_y, 0);
    float rt_height = get_height_at_point(player_x+PLAYER_RADIUS, player_y, 0);
    float tp_height = get_height_at_point(player_x, player_y-PLAYER_RADIUS, 0);
    float bt_height = get_height_at_point(player_x, player_y+PLAYER_RADIUS, 0);

    //float exact_height = get_height_at_point(player_x, player_y, 0);
    float player_contact_height = player_z-PLAYER_HEIGHT;
    float max_takeable_step = MAX(lf_height, MAX(rt_height, MAX(tp_height, bt_height)));
    //int got_takeable_step = 0;
    if(lf_height <= (player_contact_height+2)) {
        //got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, lf_height);
    }
    if(rt_height <= (player_contact_height+2)) {
        //got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, rt_height);
    }
    if(tp_height <= (player_contact_height+2)) {
        //got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, tp_height);
    }
    if(bt_height <= (player_contact_height+2)) {
        //got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, bt_height);
    }

    float target_height = max_takeable_step+PLAYER_HEIGHT;
    if(editor_mode_enabled) {
        if(target_height > player_z) {
            player_z += (target_height - player_z)*0.15;
        }
    } else {
        //if(got_takeable_step) {
            player_z += (target_height - player_z)*0.15;
        //}
    }
    player_z = MIN(MAX_WALL_HEIGHT, player_z);

    if(IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_U)) {
        player_ang += 0.0035f*frame_time;
    }
    if(IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_O)) {
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

    if (IsKeyDown(KEY_I)) {
        pitch += .0015f*frame_time;
    } else if (IsKeyDown(KEY_K)) {
        pitch -= .0015f*frame_time;
    } else if (IsKeyPressed(KEY_SPACE)) {
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

//(120.0f*.0174f)

int flat_mip_offsets[6] = {
    0, 32*32, 32*32+16*16, 32*32+16*16+8*8, 32*32+16*16+8*8+4*4, 32*32+16*16+8*8+4*4+2*2
};
int flat_mip_scales[6] = {
    //32,16,8,4,2,1
    5,4,3,2,1,0
};
int wall_mip_offsets[6] = {
    0, 32*32, 32*32+16*16, 32*32+16*16+8*8, 32*32+16*16+8*8+4*4, 32*32+16*16+8*8+4*4+2*2
};
int wall_mip_scales[6] = {
    32,16,8,4,2,1
};

void init_level(int fresh_map) {
    player_ang =  levels[cur_level_idx].start_ang;
    player_x = levels[cur_level_idx].start_x;
    player_y = levels[cur_level_idx].start_y;
    player_z = levels[cur_level_idx].start_z;
    if(fresh_map) {  
        player_x = 16;
        player_y = 16;
        memset(levels, 0, sizeof(levels));
        for(int y = 0; y < MAP_SIZE; y++) {
            for(int x = 0; x < MAP_SIZE; x++) {
                int idx = y*MAP_SIZE+x;
                if(x == 0 || y == 0 || x == MAP_SIZE-1 || y == MAP_SIZE-1) { 
                    levels[cur_level_idx].ceil[idx] = 5;
                    levels[cur_level_idx].floor[idx] = 5;
                    levels[cur_level_idx].upper_ceil[idx] = 5;
                    levels[cur_level_idx].upper_floor[idx] = 5;
                } else {
                    levels[cur_level_idx].ceil[idx] = 32;
                    levels[cur_level_idx].floor[idx] = 10;
                    levels[cur_level_idx].upper_ceil[idx] = 32;
                    levels[cur_level_idx].upper_floor[idx] = 10;

                    levels[cur_level_idx].ctex[idx] = SKYBOX_TEX_IDX;
                }
                levels[cur_level_idx].sprite_index[idx] = EMPTY_SPRITE_INDEX;
                levels[cur_level_idx].n_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                levels[cur_level_idx].e_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                levels[cur_level_idx].s_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                levels[cur_level_idx].w_sprite_index[idx] = EMPTY_SPRITE_INDEX;
            }
        }
        levels[cur_level_idx].start_x = 16;
        levels[cur_level_idx].start_y = 16;
    }
    memset(levels[cur_level_idx].parameter, 0, sizeof(levels[cur_level_idx].parameter));
    for(int i = 0; i < MAP_SIZE*MAP_SIZE; i++) {
        //levels[cur_level_idx].c_light[i] = BRIGHT;
        //levels[cur_level_idx].f_light[i] = BRIGHT;
        //levels[cur_level_idx].uc_light[i] = BRIGHT;
        //levels[cur_level_idx].uf_light[i] = BRIGHT;
        //levels[cur_level_idx].un_light[i] = BRIGHT;
        //levels[cur_level_idx].ue_light[i] = BRIGHT;
        //levels[cur_level_idx].us_light[i] = BRIGHT;
        //levels[cur_level_idx].uw_light[i] = BRIGHT;
        //levels[cur_level_idx].ln_light[i] = BRIGHT;
        //levels[cur_level_idx].le_light[i] = BRIGHT;
        //levels[cur_level_idx].ls_light[i] = BRIGHT;
        //levels[cur_level_idx].lw_light[i] = BRIGHT;
        //levels[cur_level_idx].ud_light[i] = BRIGHT;
        //levels[cur_level_idx].ld_light[i] = BRIGHT;

        //levels[cur_level_idx].sprite_index[i] = EMPTY_SPRITE_INDEX;
        //levels[cur_level_idx].n_sprite_index[i] = EMPTY_SPRITE_INDEX;
        //levels[cur_level_idx].e_sprite_index[i] = EMPTY_SPRITE_INDEX;
        //levels[cur_level_idx].s_sprite_index[i] = EMPTY_SPRITE_INDEX;
        //levels[cur_level_idx].w_sprite_index[i] = EMPTY_SPRITE_INDEX;

        //levels[cur_level_idx].sprite_index[i].loc = 0;
        //levels[cur_level_idx].sprite_index[i].index = EMPTY_SPRITE_INDEX; //MIN(levels[cur_level_idx].sprite_index[i].index, NUM_SPRITES-1);
    }
    player_z = get_height_at_point(player_x, player_y, 0) + PLAYER_HEIGHT;

}



void draw_player() {
    float y = 15*sin(player_ang);
    float x = 15*cos(player_ang);
    DrawCircle(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 5, RED);
    DrawLine(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 
        player_x*SCALE_FACTOR+x, player_y*SCALE_FACTOR+y, BLUE);
}

Font font;

typedef enum {
    TEXTURE,
    DECAL,
    SPRITE
} asset_type;
typedef struct {
    const char* name;
    const asset_type type;
} asset;

u8 heightmap[32*32] = {
    0,0,0,0,0,0,0,0,4,4,3,0,0,0,0,0,
    0,0,3,2,2,8,8,6,7,6,0,0,0,0,0,0,
    0,3,3,6,7,12,11,7,0,0,0,3,3,0,0,0,
    0,0,0,7,7,11,11,7,5,0,0,3,0,0,0,0,
    0,0,0,4,4,8,8,8,6,0,0,3,3,0,0,0,
    0,0,0,0,0,0,0,6,4,2,0,0,3,0,0,0,
    0,0,0,0,0,0,0,4,3,2,0,0,12,12,0,0,
    0,0,0,0,0,0,0,2,2,2,0,12,12,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,4,4,4,4,4,4,4,4,0,0,0,0,0,
    0,0,0,4,4,8,8,8,8,8,4,2,2,2,0,0,
    0,0,0,4,8,10,10,10,8,4,2,4,2,0,0,0,
    0,0,0,4,8,12,12,10,8,0,2,2,2,0,0,0,
    0,0,0,4,8,10,10,10,8,0,0,0,0,0,0,0,
};

void load_resources() {
    font = LoadFont("C:/Windows/Fonts/courbd.ttf");

    asset assets[] = {
        {"flat_tex0.png", TEXTURE},
        {"flat_tex1.png", TEXTURE},
        {"wall_tex0.png", TEXTURE},
        {"wall_tex1.png", TEXTURE},
        {"bookshelf.png", TEXTURE},
        {"grass.png", TEXTURE},
        {"glass_window.png", DECAL},
        {"moss.png", DECAL},
        {"chandelier.png", DECAL},
        {"tree.png", SPRITE},
        {"moss.png", SPRITE},
        {"chandelier.png", SPRITE},
        {"glass_window2.png", SPRITE},
        {"glass_window3.png", SPRITE},
        {"fence.png", SPRITE},
        {"bush.png", SPRITE},
    }; 
    const int num_assets = sizeof(assets) / sizeof(assets[0]);

    char buf[80];
    int tex_idx = 0;
    int decal_idx = 0;
    int sprite_idx = 0;
    size_t tex_num_bytes = sizeof(u8)*4*TEX_SIZE*TEX_SIZE;
    decals[decal_idx++] = calloc(tex_num_bytes, 1);
    u8* backing_texture_data = my_malloc(tex_num_bytes*(NUM_TEXTURES+NUM_DECALS+NUM_SPRITES), "assets");
    for(int asset_idx = 0; asset_idx < num_assets; asset_idx++) {
        sprintf(buf, "resources/%s", assets[asset_idx].name);
        Image tex = LoadImage(buf);
        if(tex.data == NULL) {
            printf("ERROR LOADING ASSET resources/%s\n", assets[asset_idx].name);
            exit(1);
        }
        u8* data_ptr = backing_texture_data+(tex_num_bytes*asset_idx);
        memcpy(data_ptr, tex.data, tex_num_bytes);
        UnloadImage(tex);
        switch(assets[asset_idx].type) {
            case SPRITE:
                sprites[sprite_idx++] = data_ptr;
                break;
            case DECAL:
                decals[decal_idx++] = data_ptr;
                break;
            case TEXTURE:
                textures[tex_idx++] = data_ptr;
                break;
        }
    }



    Image skybox_tex = LoadImage("resources/skybox.png");
    skybox = my_malloc(4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT, "skybox");
    memcpy(skybox, skybox_tex.data, 4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT);
    textures[SKYBOX_TEX_IDX] = skybox;
    UnloadImage(skybox_tex);
    Image height_tex = LoadImage("resources/flat_tex_heightmap.png");
    memcpy(heightmap, height_tex.data, 32*32);
    UnloadImage(height_tex);
}


void recalculate_world_sprites() {

}

void handle_editor() {
    int dy = 0;
    int dx = 0;
    if (IsKeyDown(KEY_X)) {
        player_z -= 0.1f;
    } else if (IsKeyDown(KEY_C)) {
        player_z += 0.1f;
    }
    int key = GetKeyPressed();
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
        u8* spr_ptr = NULL;
        cell_types lower_cell_type = levels[cur_level_idx].lower_cell_types[editor_selected_map_idx];
        cell_types upper_cell_type = levels[cur_level_idx].upper_cell_types[editor_selected_map_idx];
            switch(editor_selected_side) {
            case WALL_SIDE_BOTTOM:
                height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                break;

            case WALL_SIDE_UPPER_NORTH: do {
                if(upper_cell_type != NORMAL_CELL) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                }
            } while(0);
                break;
            case WALL_SIDE_UPPER_EAST: do {
                if(upper_cell_type == NW_TO_SE_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                }
            } while(0);
                break;
            case WALL_SIDE_UPPER_SOUTH:
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_WEST: do {
                if(upper_cell_type == NE_TO_SW_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                }
            } while(0);
                break;
            case WALL_SIDE_UPPER_DIAG:
            case WALL_SIDE_UPPER_BOTTOM:
                height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                break;


            case WALL_SIDE_LOWER_NORTH: do {
                if(lower_cell_type != NORMAL_CELL) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                }
            } while(0);
                break;
            case WALL_SIDE_LOWER_EAST: do {
                if(lower_cell_type == NW_TO_SE_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                }
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
            } while(0);
                break;


            case WALL_SIDE_TOP:   
                height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                break;
            case WALL_SIDE_UPPER_TOP:  
            case WALL_SIDE_LOWER_DIAG:
                height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
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
        if(height_ptr != NULL) {
            int nval = *height_ptr+dy;
            nval = CLAMP(nval, 0, MAX_WALL_HEIGHT);
            *height_ptr = nval;
        }
        if(spr_ptr != NULL) {
            if(dy == -1 && editor_selected_side != N_SPRITE) {
                editor_selected_side = N_SPRITE;
                levels[cur_level_idx].n_sprite_index[editor_selected_map_idx] = *spr_ptr;
                *spr_ptr = EMPTY_SPRITE_INDEX;
            } else if (dy == 1 && editor_selected_side != S_SPRITE) {
                editor_selected_side = S_SPRITE;
                levels[cur_level_idx].s_sprite_index[editor_selected_map_idx] = *spr_ptr;
                *spr_ptr = EMPTY_SPRITE_INDEX;
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
    } else if ((key == KEY_R) || (key == KEY_F) || (key >= KEY_KP_0 && key <= KEY_KP_9) || (key >= '0' && key <= '9')) {
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
            } else if (key == KEY_F) {
                u8 ndec_idx = ((*tex_ptr)>>4) + 1;
                if(ndec_idx >= NUM_DECALS) {
                    ndec_idx = 0;
                }
                *tex_ptr &= 0x0F;
                *tex_ptr |= (ndec_idx << 4);
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

Image draw_img;
Texture2D draw_tex;
int frame;

u8* draw_pix = NULL;
float* z_buffer = NULL;
int needs_window = 1;
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
    if(prev_use_vsync != use_vsync || prev_fullscreen != fullscreen) {
        CloseWindow();
        needs_window = 1;
    }

    //SetConfigFlags(FLAG_VSYNC_HINT);

    if(needs_window) {   
        //if(use_vsync) { 
        //    SetConfigFlags(0);
        //} else {
        //    SetConfigFlags(FLAG_VSYNC_HINT);
        //}
        //pitch = 0.0f;

        InitWindow(OUTPUT_WIDTH, OUTPUT_HEIGHT, "raycast");

        
        font = LoadFont("C:/Windows/Fonts/courbd.ttf");
        needs_window = 0;
    } else {
        SetWindowSize(OUTPUT_WIDTH, OUTPUT_HEIGHT);
    }   
    
    if(use_vsync) {
        printf("opening window with vsync\n");
        SetWindowState(FLAG_VSYNC_HINT);
        SetTargetFPS(GetMonitorRefreshRate(0));
    } else {
        printf("opening window without vsync\n");
        ClearWindowState(FLAG_VSYNC_HINT);
        SetTargetFPS(12000);
        //SetTargetFPS(6000);
    }        
    if(fullscreen) {
        SetWindowState(FLAG_FULLSCREEN_MODE);  
    } else  {
        SetWindowState(0);
    }


    if(draw_pix != NULL) {
        free(draw_pix);
        free(edit_id_buffer);
        //UnloadImage(draw_img);
        UnloadTexture(draw_tex);
    }
    draw_pix = my_malloc(sizeof(u8)*4*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "framebuffer");
    edit_id_buffer = my_malloc(sizeof(edit_wall_id)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "edit-buffer");
    z_buffer = my_malloc(sizeof(float)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "z-buffer");

    draw_img = (Image){
        .data = draw_pix,
        .width = FP_SCREEN_HEIGHT,
        .height = FP_SCREEN_WIDTH,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };

    draw_tex = LoadTextureFromImage(draw_img);
}


float prev_frame_time = 0;
draw_mode render_mode = PIXEL_BUFFER;


float skybox_u_offset;

float get_abs_time() {
#ifdef PLATFORM_WEB 
    return emscripten_get_now()/1000.0f;
#else
    return GetTime();
#endif
}

void run_game() {
#ifndef PLATFORM_WEB
    if(!IsWindowFocused()) {
        BeginDrawing();
        EndDrawing();
        return;
    }
#endif
    float frame_start_time = get_abs_time();

    Vector2 mouse_delta = GetMouseDelta();

#ifdef PLATFORM_WEB
    if(editor_mode_enabled) {
        emscripten_exit_pointerlock();
        ShowCursor();
        
    } else {
        emscripten_request_pointerlock("#canvas", EM_TRUE);
    }
#else
    if(editor_mode_enabled) {
        ShowCursor();
    } else {
        HideCursor();
        SetMousePosition(OUTPUT_WIDTH/2, OUTPUT_HEIGHT/2);
    }
#endif


    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_pos = GetMousePosition();
        handle_click(FP_SCREEN_WIDTH*mouse_pos.x/OUTPUT_WIDTH, FP_SCREEN_HEIGHT*mouse_pos.y/OUTPUT_HEIGHT);
    }
    if (IsKeyPressed(KEY_E)) {
        editor_mode_enabled = !editor_mode_enabled;
    }
    if (IsKeyPressed(KEY_Z)) {
        render_mode++;
        if(render_mode >= NUM_RENDER_MODES) {
            render_mode = 0;
        }
    }


    if(editor_mode_enabled) {
        // clear editor buffer?
        
        handle_editor();
    } else if (IsKeyPressed(KEY_R)) {
        
        if(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
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
        
    } else if (IsKeyPressed(KEY_V)) {
        requested_use_vsync = !requested_use_vsync;
    } else if (IsKeyPressed(KEY_F)) {
        requested_fullscreen = !requested_fullscreen;
    }

    float frame_time_ms = GetFrameTime()*1000.0f;
    if(requested_render_res != cur_render_res_idx || requested_render_scale != cur_render_scale || requested_use_vsync != use_vsync || requested_fullscreen != fullscreen) {
        float prev_pitch = pitch;
        change_resolution();
        pitch = prev_pitch;
    } else {
        update_player(frame_time_ms, mouse_delta);
    }

    float seconds = get_running_time();
    float quarter_seconds = seconds*4;
    int iquarter_seconds = quarter_seconds;

    skybox_u_offset = (seconds*2); // scrolls every 2 seconds

    //skybox_u_offset &= SKYBOX_TEX_WIDTH-1;
    int flash_frame = iquarter_seconds&0b1;
    BeginDrawing(); {   
        //if(sixteenth_seconds&1) {
        //    
        //}

        for(int i = 0; i < FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH; i++) {
            z_buffer[i] = DARK_DIST;
        }
        //memset(draw_img.data, 0xFFFFFFFF, FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH*4);
        //z_buffer[(screen_x*FP_SCREEN_HEIGHT+y)] = 1024.0f;
        draw_first_person_level(draw_img.data, edit_id_buffer, z_buffer,
            0, FP_SCREEN_WIDTH, flash_frame, 
            &levels[cur_level_idx], player_x, player_y, player_z, -player_ang, pitch,
            editor_mode_enabled, editor_selected_map_idx, editor_selected_side
        );

        switch(render_mode) {
            case EDITOR_BUFFER:
                //ClearBackground(BLACK);                

                UpdateTexture(draw_tex, (u32*)edit_id_buffer); //draw_img.data);
                break;
            case PIXEL_BUFFER:
                UpdateTexture(draw_tex, (u32*)draw_img.data);
                break;
            case Z_BUFFER:
                ClearBackground(BLACK);
                //float recip_far = 1.0f/DARK_DIST;
                //float recip_near = 1.0f/NEAR_PLANE_DIST;

                for(int i = 0; i < FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH; i++) {
                    float z = z_buffer[i];
                    float normalized = (z-NEAR_PLANE_DIST)/(DARK_DIST-NEAR_PLANE_DIST);
                    int byte_z = normalized*255;
                    //255*CLAMP(z/DARK_DIST, 0.1f, 1.0f);
                    ((u32*)z_buffer)[i] = (0xFF000000 | (byte_z<<16) | (byte_z<<8) | byte_z);
                }
                UpdateTexture(draw_tex, (u32*)z_buffer);
                break;

        }
        float scale = ((float)OUTPUT_WIDTH/((float)FP_SCREEN_WIDTH));
        DrawTextureEx(draw_tex, (Vector2){.x=OUTPUT_WIDTH,.y=0}, 90.0f, scale, WHITE);

        float avg_frame_time = (prev_frame_time + frame_time_ms)/2.0f;
        prev_frame_time = frame_time_ms;
        char buf[80]; 
        sprintf(buf, "%i %i -> %i %i FOV%.0f %s", cur_render_width, cur_render_height, cur_output_width, cur_output_height, cur_fov, use_vsync ? "vsync" : "");
        DrawTextEx(font, buf, (Vector2){.x = 5, .y = 5}, 18, 1, RED);
        sprintf(buf, "%4.0f fps", 1000.0f/avg_frame_time);
        DrawTextEx(font, buf, (Vector2){.x = 5, .y = 20}, 18, 1, RED);
        sprintf(buf, "%.2f %.2f %.2f %.2f\n", player_x, player_y, player_z, player_ang*RAD2DEG);
        DrawTextEx(font, buf, (Vector2){.x = 5, .y = 35}, 18, 1, RED);
    } EndDrawing();
    frame++;
    float frame_end_time = get_abs_time();
    running_time += (frame_end_time - frame_start_time);
}


#define MAP_SAVE_FILE "map_save"
void init_game() {

    load_resources();
    int num_loaded_bytes;
    u8* loaded_bytes = LoadFileData(MAP_SAVE_FILE, &num_loaded_bytes);
    if(num_loaded_bytes == sizeof(levels)) {
        memcpy(levels, loaded_bytes, sizeof(levels));
        printf("Loaded map data\n");
        init_level(0);
    } else {
        printf("Initializing new map data\n");
        init_level(1);
    }     
    recalculate_world_sprites();
}

int main(void) {
    printf("SIZEOF LEVELS %zu\n", sizeof(levels));

    init_game();
    change_resolution();
    frame = 0;

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(run_game, 0, 1);
#else 
    while(!WindowShouldClose()) {

        run_game();
    }
#endif


    levels[cur_level_idx].start_x = player_x;
    levels[cur_level_idx].start_y = player_y;
    levels[cur_level_idx].start_z = player_z;
    levels[cur_level_idx].start_ang = player_ang;
    if(!SaveFileData(MAP_SAVE_FILE, levels, sizeof(levels))) {
        printf("Error saving file :(\n");
    }
}