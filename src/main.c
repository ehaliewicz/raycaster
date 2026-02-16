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

int draw_editor_buffer = 0;
int editor_mode_enabled = 0;
int editor_selected_map_idx = -1;
editor_wall_side editor_selected_side;

int resolutions[NUM_RESOLUTIONS][2] = {
    {640, 480},
    {800, 600},
    {1024, 768},
    {1280, 1024},
    {1280, 720},
    {1920, 1080}
};
int res_is_wide[NUM_RESOLUTIONS] = {
    0,0,0,0,1,1
};

int cur_render_res_idx = -1;
int requested_render_res = 0;
int cur_render_scale = 1;
int requested_render_scale = 1;
int cur_output_width;
int cur_output_height;
int cur_render_width;
int cur_render_height;
int use_vsync = 1;
int requested_use_vsync = 1;
float cur_fov = 85.0f;


edit_wall_id *edit_id_buffer = NULL; //[FP_SCREEN_WIDTH*FP_SCREEN_HEIGHT];

void handle_click(int render_x, int render_y) {
    edit_wall_id id = edit_id_buffer[(FP_SCREEN_WIDTH-1-render_x)*FP_SCREEN_HEIGHT+(render_y)];
    editor_selected_map_idx = id.cell_idx>>2;
    editor_selected_side = id.side>>2;
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



level levels[1] = {
    {
        .start_x = 2,
        .start_y = 2,
        .start_z = 2,
    }
};

float player_x;
float player_y;
float player_z;
float player_ang;
float pitch = 0;
int cur_level_idx;
int disable_collision = 0;



#define PLAYER_RADIUS (0.25f)


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
    if((check_cell_type == NE_TO_SW_DIAG && in_top_left) || 
       (check_cell_type == NW_TO_SE_DIAG && in_top_right)) {
            return return_ceil ? this_level->upper_ceil[map_idx] : this_level->upper_floor[map_idx];
    } else if(check_cell_type == SLOPE_Y) {
        float first_height = this_level->floor[map_idx];
        float second_height = this_level->upper_floor[map_idx];
        if(return_ceil) {
            first_height = this_level->ceil[map_idx];
            second_height = this_level->upper_ceil[map_idx];
        }
        float height = first_height + (suby * (second_height - first_height));

        return height; 
    } else if (check_cell_type == SLOPE_X) { 
        float first_height = this_level->floor[map_idx];
        float second_height = this_level->upper_floor[map_idx];
        if(return_ceil) {
            first_height = this_level->ceil[map_idx];
            second_height = this_level->upper_ceil[map_idx];
        }
        float height = first_height + (subx * (second_height - first_height));

        return height; 
    } else {
        return return_ceil ? this_level->ceil[map_idx] : this_level->floor[map_idx];
    }
}

int collides(float px, float py, level this_level) {
    if (disable_collision) { return 0; }
    if(editor_mode_enabled) { return 0; }
    int x = px;
    int y = py;
    int idx = y*MAP_SIZE+x;
    int floor = this_level.floor[idx];

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
    if(floor_height > (player_z-PLAYER_HEIGHT)+2) {
        return 1;
    }
    return 0;
}


void update_player(float frame_time, Vector2 mouse_delta) {
    float y = sin(player_ang);
    float x = cos(player_ang);
    float strafe_right_x = -y;
    float strafe_right_y = x;
    float strafe_left_x = y;
    float strafe_left_y = -x;
    float move_speed = .06f * frame_time / 16.0f;
    level cur_level = levels[cur_level_idx];
    float r = PLAYER_RADIUS;
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

    float exact_height = get_height_at_point(player_x, player_y, 0);
    float player_contact_height = player_z-PLAYER_HEIGHT;
    float max_takeable_step = MAX(lf_height, MAX(rt_height, MAX(tp_height, bt_height)));
    int got_takeable_step = 0;
    if(lf_height <= (player_contact_height+1)) {
        got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, lf_height);
    }
    if(rt_height <= (player_contact_height+1)) {
        got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, rt_height);
    }
    if(tp_height <= (player_contact_height+1)) {
        got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, tp_height);
    }
    if(bt_height <= (player_contact_height+1)) {
        got_takeable_step = 1;
        max_takeable_step = MAX(max_takeable_step, bt_height);
    }


    //if(got_takeable_step) {
        player_z = max_takeable_step + PLAYER_HEIGHT; 
    //}

    if(IsKeyDown(KEY_LEFT)) {
        player_ang -= 0.0035f*frame_time;
    }
    if(IsKeyDown(KEY_RIGHT)) {
        player_ang += 0.0035f*frame_time;
    }
    if(!editor_mode_enabled) {
        if(mouse_delta.y != 0) {
            pitch -= .0015f*mouse_delta.y;
        }
        if(mouse_delta.x != 0) {
            player_ang += mouse_delta.x*.0017f;
        }
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

void init_level(int init) {
    player_ang =  1.5707963f;
    player_x = levels[cur_level_idx].start_x;
    player_y = levels[cur_level_idx].start_y;
    if(init) {  
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
            }
        }
        levels[cur_level_idx].start_x = 16;
        levels[cur_level_idx].start_y = 16;
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

void load_resources() {
    font = LoadFont("C:/Windows/Fonts/courbd.ttf");

    Image tex0 = LoadImage("resources/wall_tex0.png");
    Image tex1 = LoadImage("resources/wall_tex1.png");
    Image tex2 = LoadImage("resources/flat_tex0.png");
    Image tex3 = LoadImage("resources/flat_tex1.png");
    Image tex4 = LoadImage("resources/bookshelf.png");
    Image window_tex = LoadImage("resources/glass_window.png");
    Image moss_tex = LoadImage("resources/moss.png");
    Image chandelier_tex = LoadImage("resources/chandelier.png");
    Image skybox_tex = LoadImage("resources/skybox.png");


    size_t tex_num_bytes = sizeof(u8)*4*TEX_SIZE*TEX_SIZE;
    u8* backing_texture_data = malloc(tex_num_bytes*(NUM_TEXTURES+NUM_DECALS));

    u8* tex0_data = backing_texture_data+(tex_num_bytes*0);
    u8* tex1_data = backing_texture_data+(tex_num_bytes*1);
    u8* tex2_data = backing_texture_data+(tex_num_bytes*2);
    u8* tex3_data = backing_texture_data+(tex_num_bytes*3);
    u8* tex4_data = backing_texture_data+(tex_num_bytes*4);
    u8* window_tex_data = backing_texture_data+(tex_num_bytes*5);
    u8* moss_tex_data = backing_texture_data+(tex_num_bytes*6);
    u8* chandelier_tex_data = backing_texture_data+(tex_num_bytes*7);
    
    u8* copy_ptrs[][2] = {
        {tex0_data, tex0.data},
        {tex1_data, tex1.data},
        {tex2_data, tex2.data},
        {tex3_data, tex3.data},
        {tex4_data, tex4.data},
        {window_tex_data, window_tex.data},
        {moss_tex_data, moss_tex.data},
        {chandelier_tex_data, chandelier_tex.data}

    };
    for(int mip = 0; mip < 1; mip++) {
        int dim = TEX_SIZE>>mip;
        for(int i = 0; i < 8; i++) {
            u8* src = copy_ptrs[i][1];
            u8* dst = copy_ptrs[i][0];
            memcpy(dst, src, tex_num_bytes);
        }
    }
    
    textures[0] = tex2_data;
    textures[1] = tex3_data;
    textures[2] = tex0_data;
    textures[3] = tex1_data;
    textures[4] = tex4_data;
    textures[5] = tex1_data;

    decals[0] = calloc(tex_num_bytes, 1);
    decals[1] = window_tex_data;
    decals[2] = moss_tex_data;
    decals[3] = chandelier_tex_data;
    skybox = malloc(4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT);
    memcpy(skybox, skybox_tex.data, 4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT);
    textures[SKYBOX_TEX_IDX] = skybox;
}

void handle_editor() {
    int dy = 0;
    if(IsKeyPressed(KEY_DOWN)) {
        dy = -1;
    } else if (IsKeyPressed(KEY_UP)) {
        dy = 1;
    }
    if(dy != 0) {
        u8* height_ptr = NULL;
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
        }
        if(height_ptr != NULL) {
            int nval = *height_ptr+dy;
            nval = CLAMP(nval, 0, MAX_WALL_HEIGHT);
            *height_ptr = nval;
        }
    } else if (IsKeyPressed(KEY_T)) {
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
        }
        if(type_ptr != NULL) {
            u8 nval = *type_ptr + 1;
            if(nval >= NUM_CELL_TYPES) {
                nval = 0;
            }
            *type_ptr = nval;
        }
    } else if (IsKeyPressed(KEY_L)) {
        levels[cur_level_idx].light[editor_selected_map_idx] += 1;
        if(levels[cur_level_idx].light[editor_selected_map_idx] >= NUM_LIGHT_LEVELS) {
            levels[cur_level_idx].light[editor_selected_map_idx] = 0;
        }
    } else if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_F)) {
        u8* tex_ptr = NULL;
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
        }
        if(tex_ptr != NULL) {
            if(IsKeyPressed(KEY_R)) {
                u8 ntex_idx = ((*tex_ptr)&0xF)+1;
                if(ntex_idx > SKYBOX_TEX_IDX) {
                    ntex_idx = 0;
                }
                if(ntex_idx >= NUM_TEXTURES) {
                    ntex_idx = SKYBOX_TEX_IDX;
                }
                *tex_ptr &= 0xF0;
                *tex_ptr |= ntex_idx;
            } else if (IsKeyPressed(KEY_F)) {
                u8 ndec_idx = ((*tex_ptr)>>4) + 1;
                if(ndec_idx >= NUM_DECALS) {
                    ndec_idx = 0;
                }
                *tex_ptr &= 0x0F;
                *tex_ptr |= (ndec_idx << 4);
            }
        }
    }
    
}

Image draw_img;
Texture2D draw_tex;
int frame;

u8* draw_pix = NULL;
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
    use_vsync = requested_use_vsync;
    if(prev_use_vsync != use_vsync) {
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
        SetTargetFPS(6000);
        //SetTargetFPS(6000);
    }


    if(draw_pix != NULL) {
        free(draw_pix);
        free(edit_id_buffer);
        //UnloadImage(draw_img);
        UnloadTexture(draw_tex);
    }
    draw_pix = malloc(sizeof(u8)*4*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH);
    edit_id_buffer = malloc(sizeof(edit_wall_id)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH);

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

void run_game() {
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
        draw_editor_buffer = !draw_editor_buffer;
    }


    if(editor_mode_enabled) {
        handle_editor();
    } else if (IsKeyPressed(KEY_R)) {
        if(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            requested_render_scale++;
            if(requested_render_scale > 2) {
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
    }

    float frame_time_ms = GetFrameTime()*1000.0f;
    if(requested_render_res != cur_render_res_idx || requested_render_scale != cur_render_scale || requested_use_vsync != use_vsync) {
        float prev_pitch = pitch;

        change_resolution();
        pitch = prev_pitch;
    } else {
        update_player(frame_time_ms, mouse_delta);
    }
    BeginDrawing(); {   
        GetTime();
        
        draw_first_person_level(draw_img.data, edit_id_buffer, 
            0, FP_SCREEN_WIDTH, 
            frame, 
            &levels[cur_level_idx], player_x, player_y, player_z, player_ang, pitch,
            editor_mode_enabled, editor_selected_map_idx, editor_selected_side
        );


        if(draw_editor_buffer) {
            UpdateTexture(draw_tex, (u32*)edit_id_buffer);
        } else {
            UpdateTexture(draw_tex, draw_img.data);
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
        sprintf(buf, "%.2f %.2f %.2f\n", player_x, player_y, player_z);
        DrawTextEx(font, buf, (Vector2){.x = 5, .y = 35}, 18, 1, RED);
    } EndDrawing();
    frame++;
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
}

int main(void) {
    
    init_game();
    change_resolution();
    frame = 0;

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(main_loop, 0, 1);
#else 
    while(!WindowShouldClose()) {

        run_game();
    }
#endif


    levels[cur_level_idx].start_x = player_x;
    levels[cur_level_idx].start_y = player_y;
    if(!SaveFileData(MAP_SAVE_FILE, levels, sizeof(levels))) {
        printf("Error saving file :(\n");
    }
}