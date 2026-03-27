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
#include "platform.h"

#include "collision.h"
#include "common.h"
#include "draw.h"
#include "entity.h"
#include "lz.h"
#include "network.h"
#include "raycast.h"
#include "resources.h"
#include "6dof.h"

typedef enum {
    PIXEL_BUFFER = 0,
    Z_BUFFER = 1,
    EDITOR_BUFFER = 2,
} draw_mode;
#define NUM_RENDER_MODES 3

int editor_mode_enabled = 0;
int editor_selected_idx = -1;
editor_selected_thing editor_selected_side;

const int resolutions[NUM_RESOLUTIONS][2] = {
    {640, 480},
    {800, 600},
    {1024, 768},
    {1280, 1024},
    {1280, 720},
    {1920, 1080},
    {3440, 1300}
};
const int res_is_wide[NUM_RESOLUTIONS] = {
    0,0,0,0,1,1,0
};
const int res_is_superwide[NUM_RESOLUTIONS] = {
    0,0,0,0,0,0,1
};

int cur_render_res_idx = -1;
int requested_render_res = 4;
int cur_render_scale = -1;
int requested_render_scale = 1;
int cur_output_width = 1280;
int cur_output_height = 720;
int cur_render_width;
int cur_render_height;
int use_vsync = 1;
int requested_use_vsync = 1;
int requested_fullscreen = 0;
int fullscreen = 0;
float cur_fov = 90.0f;



void handle_click(edit_wall_id* prev_rendered_id_buffer, int render_x, int render_y) {

    edit_wall_id id = prev_rendered_id_buffer[(RENDER_WIDTH-1-render_x)*RENDER_HEIGHT+(render_y)];
    //editor_selected_map_idx = (id) & 0xFFFF; //id.cell_idx;
    //editor_selected_side = (id>>16)&0xFF;// id.side;
    
    //editor_selected_idx = id&0b1111111111;
    //editor_selected_side = (id>>10)&0b11111;
    if(id.type == MAP_CELL_EDIT_ID_TYPE) {
        editor_selected_idx = id.idx>>5;
        editor_selected_side = id.idx&0b11111;
    } else {
        editor_selected_idx = id.idx;
        editor_selected_side = ENTITY;
    }
}


//void log(char* )
//const char buf[80];
void console_log(const char *format, ...) {
    //va_list arg;
    //int cnt;

    //va_start(arg, format);
    //vsdebug_printf((char * __restrict__)buf, format, arg);
    //va_end(arg);
    // Analyze cnt and check for stream errors here
    return; //(uintmax_t)cnt;
}

void* my_malloc(long long unsigned int bytes, char* for_str) {
    debug_printf("Allocating %llu bytes for %s\n", bytes, for_str);
    return malloc(bytes+64);
}

void* my_calloc(long long unsigned int bytes, char* for_str) {
    debug_printf("Allocating %llu bytes for %s\n", bytes, for_str);
    return calloc(bytes+64, 1);
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



edit_wall_id* edit_id_buffer_pointers[2] = {NULL, NULL}; //[RENDER_WIDTH*FP_SCREEN_HEIGHT];

unsigned int draw_textures[2];
unsigned int seg_tex_handles[2];
int frame;

u32* seg_draw_bufs[2] = { NULL, NULL };
u32* transpose_buf = NULL;
u32* draw_pix_pointers[2] = {NULL, NULL};
u16* zbuf_pointers[2] = {NULL, NULL};

u32* skybox;

level *levels = NULL;

float player_x;
float player_y;
float player_z;
float player_yaw;
float player_pitch = 0.0f;
float pitch = 0;
int cur_level_idx;
int disable_collision = 0;



int door_timer_running = 0;
int timer_door = 0; // the map idx of the door we're opening
float start_open_time; // one second to open, one second open, one second to close?

float cur_player_height = STANDING_HEIGHT;
float cur_player_speed = WALK_SPEED;
float player_vel_z = 0.0f;

#define FALL_GRAVITY_ACCEL -0.060f
#define JUMP_GRAVITY_ACCEL -0.008f
#define JUMP_VEL 0.15f
#define FALL_CROUCH_DURATION 0.20f

float player_stand_dur = 0.0f;
float player_jump_dur = 0.0f;

int got_revolver = 0;
int got_whiskey = 0;

typedef struct {
    float world_x, world_y, world_z;
    int ttl;
    int sprite_idx;
} particle;
#define MAX_PARTICLES 32
particle particles[MAX_PARTICLES];
void init_particle_system() {
    for(int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].ttl = 0;
    }
}
void update_particles() {
    for(int i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].ttl <= 0) { continue; }
        particles[i].ttl--;
    }
}
void draw_particles() {
    for(int i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].ttl <= 0) { continue; }
        request_draw_sprite(particles[i].world_x, particles[i].world_y, particles[i].world_z, INVALID_ENTITY_ID, particles[i].sprite_idx);
    }
}
void add_particle(float x, float y, float z, int ttl, int sprite_idx) {
    // adds if possible :)
    // todo: replace particle with smallest TTL
    for(int i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].ttl > 0) { continue; }
        particles[i].world_x = x;
        particles[i].world_y = y;
        particles[i].world_z = z;
        particles[i].ttl = ttl;
        particles[i].sprite_idx = sprite_idx;
        break;
    }
}

#define REVOLVER_FIRE_DURATION (.28f)
float revolver_firing = 0.0f;

typedef enum {
    FIGHTING,
    WON,
    DEAD
} game_state;

game_state cur_game_state = FIGHTING;

int entities_woke = 0;

float adjust_position_for_door(float pos, int open_amount) {
    int map_pos = my_floorf(pos);
    float adjusted = pos;
    float sub = pos - my_floorf(pos);
    if(sub >= 0.9f) {
        adjusted = my_floorf(pos+1.0f)+PLAYER_RADIUS+0.1f;
    } else if(open_amount >= DOOR_FULLY_OPEN) {
        float max_in_cell = (((float)open_amount-DOOR_FULLY_OPEN)/(255-DOOR_FULLY_OPEN));
        if(sub > max_in_cell) {
            adjusted = my_floorf(pos) + max_in_cell;

            if((int)player_x != map_pos) {
                adjusted = my_floorf(pos)+(PLAYER_RADIUS-0.1f);
            }
        }
    } else {
        adjusted = my_floorf(pos)- (PLAYER_RADIUS+0.1f);
    }
    return adjusted;
}

void update_player(float frame_time, Vector2 mouse_delta) {
    const float frame_mult = frame_time / 16.0f;
    const float frame_seconds = frame_time/1000.0f;
    const float max_fall = FALL_GRAVITY_ACCEL*8.0f*frame_mult;
    //const float max_fall = GRAVITY_ACCEL*8.0f*frame_mult;
    
    cur_player_speed = player_stand_dur >= 0.0f ? (platform_is_key_down(KEY_SHIFT) ? SPRINT_SPEED : WALK_SPEED) : AIR_SPEED;


    float y = my_sinf(player_yaw);
    float x = my_cosf(player_yaw);
    float strafe_left_x = -y;
    float strafe_left_y = x;
    float strafe_right_x = y;
    float strafe_right_y = -x;
    float move_speed = cur_player_speed * frame_mult;
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
        int pmx = my_floorf(player_x);
        int pmy = my_floorf(player_y);
        if(pmy*MAP_SIZE+pmx == timer_door) {
            cell_types door_type = levels[cur_level_idx].lower_cell_types[timer_door];

            // we are *INSIDE* the door
            if(door_closing) {
                float subx = player_x - my_floorf(player_x);
                float suby = player_y - my_floorf(player_y);
                if(door_type == DOOR_Y) {
                    player_y = adjust_position_for_door(player_y, int_open_amount);
                } else if(door_type == DOOR_X) {
                    player_x = adjust_position_for_door(player_x, int_open_amount);
                }
            }
        }
    }

    if(platform_is_key_down(KEY_ENTER) || platform_is_mouse_button_pressed(MOUSE_BUTTON_MIDDLE)) {
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

    int moved = 0;
    float vel_x = 0.0f;
    float vel_y = 0.0f;

    if (platform_is_key_down(KEY_W)) {
        moved = 1;
        vel_x += move_speed*x;
        vel_y += move_speed*y;
    }
    if(platform_is_key_down(KEY_A)) {
        moved = 1;
        vel_x += move_speed*strafe_left_x;
        vel_y += move_speed*strafe_left_y;
    }
    if(platform_is_key_down(KEY_S)) {
        moved = 1;
        vel_x += -move_speed*x;
        vel_y += -move_speed*y;
    }

    if(platform_is_key_down(KEY_D)) {
        moved = 1;
        vel_x += move_speed*strafe_right_x;
        vel_y += move_speed*strafe_right_y;
    }

    if(moved) {
        float new_x = player_x + vel_x;
        float new_y = player_y + vel_y;
        float probe_x = new_x + (vel_x > 0 ? r : -r);
        float probe_y = new_y + (vel_y > 0 ? r : -r);

        if((collides(player_x, player_y, player_z, probe_x, player_y - r, player_z, cur_level, disable_collision, editor_mode_enabled) == 0) && 
            (collides(player_x, player_y, player_z, probe_x, player_y, player_z, cur_level, disable_collision, editor_mode_enabled) == 0) &&
            (collides(player_x, player_y, player_z, probe_x, player_y + r, player_z, cur_level, disable_collision, editor_mode_enabled) == 0)) {
            player_x = new_x;
        }
        if((collides(player_x, player_y, player_z, player_x - r, probe_y, player_z, cur_level, disable_collision, editor_mode_enabled) == 0) && 
            (collides(player_x, player_y, player_z, player_x,    probe_y, player_z, cur_level, disable_collision, editor_mode_enabled) == 0) &&
            (collides(player_x, player_y, player_z, player_x + r, probe_y, player_z, cur_level, disable_collision, editor_mode_enabled) == 0)) {
            player_y = new_y;
        }
    }

    int map_x = my_floorf(player_x);
    int map_y = my_floorf(player_y);
    float sub_x = player_x-map_x;
    float sub_y = player_y-map_y;
    int cur_cell_sprite_idx = levels[cur_level_idx].sprite_index[map_y*MAP_SIZE+map_x];
    if(sub_y >= 0.25f && sub_y <= 0.75f && sub_x >= 0.25f && sub_x <= 0.75f) {
        if(cur_cell_sprite_idx == REVOLVER_SPRITE_INDEX) {
            got_revolver = 1;
        } else if (cur_cell_sprite_idx == WHISKEY_SPRITE_INDEX) {
            got_whiskey = 1;
        }
        levels[cur_level_idx].sprite_index[map_y*MAP_SIZE+map_x] = EMPTY_SPRITE_INDEX;
    }


    float ct_floor_height = get_height_at_point(player_x, player_y, player_z, 0, 1);
    float lf_floor_height = get_height_at_point(player_x-PLAYER_RADIUS, player_y, player_z, 0, 1);
    float rt_floor_height = get_height_at_point(player_x+PLAYER_RADIUS, player_y, player_z, 0, 1);
    float tp_floor_height = get_height_at_point(player_x, player_y-PLAYER_RADIUS, player_z, 0, 1);
    float bt_floor_height = get_height_at_point(player_x, player_y+PLAYER_RADIUS, player_z, 0, 1);

    float ct_ceiling_height = get_height_at_point(player_x, player_y, player_z, 1, 1);
    float lf_ceiling_height = get_height_at_point(player_x-PLAYER_RADIUS, player_y, player_z, 1, 1);
    float rt_ceiling_height = get_height_at_point(player_x+PLAYER_RADIUS, player_y, player_z, 1, 1);
    float tp_ceiling_height = get_height_at_point(player_x, player_y-PLAYER_RADIUS, player_z, 1, 1);
    float bt_ceiling_height = get_height_at_point(player_x, player_y+PLAYER_RADIUS, player_z, 1, 1);

    float lowest_height = MIN(MAX_WALL_HEIGHT, 
        MIN(ct_ceiling_height, MIN(lf_ceiling_height, MIN(rt_ceiling_height, MIN(tp_ceiling_height, bt_ceiling_height))))
    );


    float player_foot = player_z - cur_player_height;

    // A probe can only pull you up if it's within step range of where you already are.
    // This stops a corner probe from teleporting you onto a ledge you haven't reached.
    float max_takeable_step = ct_floor_height;
    if (lf_floor_height - player_foot <= MAX_STEP_HEIGHT) max_takeable_step = MAX(max_takeable_step, lf_floor_height);
    if (rt_floor_height - player_foot <= MAX_STEP_HEIGHT) max_takeable_step = MAX(max_takeable_step, rt_floor_height);
    if (tp_floor_height - player_foot <= MAX_STEP_HEIGHT) max_takeable_step = MAX(max_takeable_step, tp_floor_height);
    if (bt_floor_height - player_foot <= MAX_STEP_HEIGHT) max_takeable_step = MAX(max_takeable_step, bt_floor_height);

    float target_height = max_takeable_step + cur_player_height;


    float target_dz = 0.0f;
    if(editor_mode_enabled) {

    } else {
        target_dz = (target_height - player_z);
    }
    if(player_vel_z <= 0.0f && (my_fabsf(target_dz) < 0.01f || target_dz > 0.0f)) {
        player_stand_dur += frame_seconds;
        if(0) { //player_stand_dur >= 0.0f && player_stand_dur <= FALL_CROUCH_DURATION) {
            float lerp_dur = MIN(player_stand_dur, FALL_CROUCH_DURATION)/FALL_CROUCH_DURATION;
            cur_player_height = lerp(FALL_HEIGHT, STANDING_HEIGHT, lerp_dur);
        } else {
            cur_player_height = (platform_is_key_down(KEY_CONTROL) && !editor_mode_enabled) ? CROUCHING_HEIGHT : STANDING_HEIGHT;
            
        }
        player_vel_z = 0.0f;
        player_foot += target_dz*0.05f;
    } else {
        player_stand_dur = 0.0f;
        player_vel_z += (player_vel_z > 0.0f ? JUMP_GRAVITY_ACCEL : FALL_GRAVITY_ACCEL)*frame_mult;
        
        player_vel_z = CLAMP(player_vel_z, max_fall, JUMP_VEL);
        if(player_vel_z < 0 && -player_vel_z > -target_dz) {
            player_vel_z = target_dz;
        }
        player_foot += player_vel_z;
    }
    player_z = player_foot + cur_player_height;
    player_z = MIN(lowest_height-HEAD_MARGIN, player_z);

    int jumped = 0;
    if(platform_is_key_down(KEY_SPACE) && ((player_stand_dur>.15f) || (player_jump_dur > 0.0f && player_jump_dur < 0.15f))) {
        player_vel_z += JUMP_VEL*frame_mult;
        player_stand_dur = 0.0f;
        player_jump_dur += frame_seconds;
        jumped = 1;
    } else {
        if(player_vel_z > 0.0f) { player_vel_z = MIN(player_vel_z, 0.1f); }
        player_jump_dur = 0.0f;
    }



    if(platform_is_key_down(KEY_LEFT) || platform_is_key_down(KEY_U)) {
        player_yaw += 0.0035f*frame_time;
    }
    if(platform_is_key_down(KEY_RIGHT) || platform_is_key_down(KEY_O)) {
        player_yaw -= 0.0035f*frame_time;
    }
    
//#define MOUSE_SENSITIVITY 0.00003f
#define PIXELS_PER_RADIAN (RENDER_HEIGHT)

    if(!editor_mode_enabled) {
        if(mouse_delta.y != 0) {
            player_pitch += (-mouse_delta.y) / PIXELS_PER_RADIAN;
        }
        if(mouse_delta.x != 0) {
            player_yaw -= mouse_delta.x*.0017f;
        }
    }

    // cleanup angle
    if(player_yaw < 0.0f) {
        player_yaw += 6.28f;
    } else if (player_yaw > 6.28f) {
        player_yaw -= 6.28f;
    }

    float pitch_speed = 0.0005f*frame_mult;
    float dy = (platform_is_key_down(KEY_I) ? 1.0f : (platform_is_key_down(KEY_K)) ? -1.0f : 0.0f);
    player_pitch += dy*pitch_speed;

    if (platform_is_key_pressed(KEY_SPACE)) {
        //pitch = 0;
    }

    //player_pitch = CLAMP(player_pitch, -EIGTH_CIRCLE_RADS, EIGTH_CIRCLE_RADS);
    pitch = my_sinf(player_pitch) * 16.0f * (HEIGHT_SCALE * FOCAL_LENGTH / MAX_WALL_HEIGHT);


    if(got_revolver && revolver_firing == 0.0f && platform_is_mouse_button_pressed(MOUSE_BUTTON_LEFT) && !editor_mode_enabled) {
        platform_play_sound(GUNSHOT_WAV);
        revolver_firing = REVOLVER_FIRE_DURATION;
        u16 dist_fixed = zbuf_pointers[0][(RENDER_WIDTH/2)*RENDER_HEIGHT+RENDER_HEIGHT/2];
        edit_wall_id obj_id = edit_id_buffer_pointers[0][(RENDER_WIDTH/2)*RENDER_HEIGHT + RENDER_HEIGHT/2];
        float dist = ((float)dist_fixed/FIXED_POINT_MULT);
        float dir_x = my_cosf(player_yaw);
        float dir_y = my_sinf(player_yaw);
        float dir_z = my_sinf(player_pitch)*16.0f;

        float world_x = player_x + dir_x * dist;
        float world_y = player_y + dir_y * dist;
        float world_z = player_z + dir_z * dist - 1.0f;

        //debug_printf("vertical angle of ray %f\n", player_pitch);
        //debug_printf("world %f %f %f \n", world_x, world_y, world_z);
        //debug_printf("player z %f\n", player_z);
        add_particle(world_x, world_y, world_z, 20, SMOKE_PARTICLE_IDX);

        if(obj_id.type == ENTITY_EDIT_ID_TYPE) {
            damage_entity(10, obj_id.idx);
            if(!entities_woke) {
                wakeup_entities(player_x, player_y, player_z);
                entities_woke = 1;
            }
        }

    } else if (revolver_firing > 0.0f) {
        revolver_firing -= frame_seconds;
        revolver_firing = MAX(0.0f, revolver_firing);
    }

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
    player_yaw =  QUARTER_CIRCLE_RADS; levels[cur_level_idx].start_ang;
    player_x = levels[cur_level_idx].start_x;
    player_y = levels[cur_level_idx].start_y;
    player_z = levels[cur_level_idx].start_z;

    levels[0].sprite_index[17*MAP_SIZE+11] = REVOLVER_SPRITE_INDEX;
    levels[0].sprite_index[17*MAP_SIZE+10] = WHISKEY_SPRITE_INDEX;

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
                    levels[level].f_sprite_index[idx] = EMPTY_SPRITE_INDEX;
                    levels[level].c_sprite_index[idx] = EMPTY_SPRITE_INDEX;
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
    
    player_z = get_height_at_point(player_x, player_y, player_z, 0, 1) + cur_player_height + 3.0f;

}



void draw_player() {
    //float y = 15*my_sinf(player_ang);
    //float x = 15*my_cosf(player_ang);
    //DrawCircle(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 5, RED);
    //DrawLine(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 
    //    player_x*SCALE_FACTOR+x, player_y*SCALE_FACTOR+y, BLUE);
}

//Font font;


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
        cell_types lower_cell_type = levels[cur_level_idx].lower_cell_types[editor_selected_idx];
        cell_types upper_cell_type = levels[cur_level_idx].upper_cell_types[editor_selected_idx];
            switch(editor_selected_side) {
            case ENTITY:
                break;
            case WALL_SIDE_BOTTOM:
                height_ptr = &levels[cur_level_idx].ceil[editor_selected_idx];
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_idx];
                break;

            case WALL_SIDE_UPPER_NORTH: do {
                if(upper_cell_type != NORMAL_CELL) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_idx];
                }
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_idx];
            } while(0);
                break;
            case WALL_SIDE_UPPER_EAST: do {
                if(upper_cell_type == NW_TO_SE_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_idx];
                }
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_idx];
            } while(0);
                break;
            case WALL_SIDE_UPPER_SOUTH:
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_idx];
                    anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_WEST: do {
                if(upper_cell_type == NE_TO_SW_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].ceil[editor_selected_idx];
                }
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_idx];
            } while(0);
                break;
            case WALL_SIDE_UPPER_DIAG:
            case WALL_SIDE_UPPER_BOTTOM:
                height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_idx];
                anchor_ptr = &levels[cur_level_idx].ceil_anchor[editor_selected_idx];
                break;


            case WALL_SIDE_LOWER_NORTH: do {
                if(lower_cell_type != NORMAL_CELL) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_idx];
                }
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_idx];
            } while(0);
                break;
            case WALL_SIDE_LOWER_EAST: do {
                if(lower_cell_type == NW_TO_SE_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_idx];
                }
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_idx];
            } while(0);
                break;
            case WALL_SIDE_LOWER_SOUTH:
                height_ptr = &levels[cur_level_idx].floor[editor_selected_idx];
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_WEST: do {
                if(lower_cell_type == NE_TO_SW_DIAG) {
                    height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_idx];
                } else {
                    height_ptr = &levels[cur_level_idx].floor[editor_selected_idx];
                }
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_idx];
            } while(0);
                break;


            case WALL_SIDE_TOP:   
                height_ptr = &levels[cur_level_idx].floor[editor_selected_idx];
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_TOP:  
            case WALL_SIDE_LOWER_DIAG:
                height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_idx];
                anchor_ptr = &levels[cur_level_idx].floor_anchor[editor_selected_idx];
                break;
            case FLOOR_SPRITE:
                spr_ptr = &levels[cur_level_idx].f_sprite_index[editor_selected_idx];
                break;
            case CEIL_SPRITE:
                spr_ptr = &levels[cur_level_idx].c_sprite_index[editor_selected_idx];
                break;
            case MIDDLE_SPRITE:
                spr_ptr = &levels[cur_level_idx].m_sprite_index[editor_selected_idx];
                height_ptr = &levels[cur_level_idx].m_sprite_offset[editor_selected_idx];
                break;
            case CELL_SPRITE:
                spr_ptr = &levels[cur_level_idx].sprite_index[editor_selected_idx];
                break;
            case N_SPRITE:
                spr_ptr = &levels[cur_level_idx].n_sprite_index[editor_selected_idx];
                break;
            case E_SPRITE:
                spr_ptr = &levels[cur_level_idx].e_sprite_index[editor_selected_idx];
                break;
            case S_SPRITE:
                spr_ptr = &levels[cur_level_idx].s_sprite_index[editor_selected_idx];
                break;
            case W_SPRITE:
                spr_ptr = &levels[cur_level_idx].w_sprite_index[editor_selected_idx];
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

        if(spr_ptr != NULL && (!platform_is_key_down(KEY_SHIFT))) {

            if(dy == -1) { 
                if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side == CEIL_SPRITE)) {
                    // move to ceil position
                    editor_selected_side = MIDDLE_SPRITE;
                    levels[cur_level_idx].m_sprite_index[editor_selected_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side != FLOOR_SPRITE)) {
                    // move to middle position
                    editor_selected_side = FLOOR_SPRITE;
                    levels[cur_level_idx].f_sprite_index[editor_selected_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if ((!platform_is_key_down(KEY_CONTROL)) && editor_selected_side != N_SPRITE) {
                    editor_selected_side = N_SPRITE;
                    levels[cur_level_idx].n_sprite_index[editor_selected_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                }
            } else if (dy == 1) {
                 if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side == FLOOR_SPRITE)) {
                    // move to middle position
                    editor_selected_side = MIDDLE_SPRITE;
                    levels[cur_level_idx].m_sprite_index[editor_selected_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if (platform_is_key_down(KEY_CONTROL) && (editor_selected_side != CEIL_SPRITE)) {
                    // move to floor position
                    editor_selected_side = CEIL_SPRITE;
                    levels[cur_level_idx].c_sprite_index[editor_selected_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                } else if ((!platform_is_key_down(KEY_CONTROL)) && editor_selected_side != S_SPRITE) {
                    editor_selected_side = S_SPRITE;
                    levels[cur_level_idx].s_sprite_index[editor_selected_idx] = *spr_ptr;
                    *spr_ptr = EMPTY_SPRITE_INDEX;
                }
             } else if (dx == -1 && editor_selected_side != W_SPRITE) {
                editor_selected_side = W_SPRITE;
                levels[cur_level_idx].w_sprite_index[editor_selected_idx] = *spr_ptr;
                *spr_ptr = EMPTY_SPRITE_INDEX;
            } else if (dx == 1 && editor_selected_side != E_SPRITE) {
                levels[cur_level_idx].e_sprite_index[editor_selected_idx] = *spr_ptr;
                editor_selected_side = E_SPRITE;
                *spr_ptr = EMPTY_SPRITE_INDEX;
            }
        }

    } else if (key == KEY_P) {
        int idx = editor_selected_idx;
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
                type_ptr = &levels[cur_level_idx].upper_cell_types[editor_selected_idx];
                break;
            case WALL_SIDE_TOP:   
            case WALL_SIDE_LOWER_NORTH:
            case WALL_SIDE_LOWER_EAST:
            case WALL_SIDE_LOWER_SOUTH:
            case WALL_SIDE_LOWER_WEST:
            case WALL_SIDE_UPPER_TOP:  
            case WALL_SIDE_LOWER_DIAG:
                type_ptr = &levels[cur_level_idx].lower_cell_types[editor_selected_idx];
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
                light_ptr = &levels[cur_level_idx].c_light[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_BOTTOM:
                light_ptr = &levels[cur_level_idx].uc_light[editor_selected_idx];
                break;
            case WALL_SIDE_TOP:
                light_ptr = &levels[cur_level_idx].f_light[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_TOP:
                light_ptr = &levels[cur_level_idx].uf_light[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_NORTH:
                light_ptr = &levels[cur_level_idx].ln_light[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_NORTH:
                light_ptr = &levels[cur_level_idx].un_light[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_EAST:
                light_ptr = &levels[cur_level_idx].le_light[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_EAST:
                light_ptr = &levels[cur_level_idx].ue_light[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_SOUTH:
                light_ptr = &levels[cur_level_idx].ls_light[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_SOUTH:
                light_ptr = &levels[cur_level_idx].us_light[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_WEST:
                light_ptr = &levels[cur_level_idx].lw_light[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_WEST:
                light_ptr = &levels[cur_level_idx].uw_light[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_DIAG:
                light_ptr = &levels[cur_level_idx].ud_light[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_DIAG:
                light_ptr = &levels[cur_level_idx].ld_light[editor_selected_idx];
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
            case ENTITY:
                break;
            case WALL_SIDE_BOTTOM:
                tex_ptr = &levels[cur_level_idx].ctex[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_BOTTOM:
                tex_ptr = &levels[cur_level_idx].uctex[editor_selected_idx];
                break;
            case WALL_SIDE_TOP:
                tex_ptr = &levels[cur_level_idx].ftex[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_TOP:
                tex_ptr = &levels[cur_level_idx].uftex[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_NORTH:
                tex_ptr = &levels[cur_level_idx].lntex[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_NORTH:
                tex_ptr = &levels[cur_level_idx].untex[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_EAST:
                tex_ptr = &levels[cur_level_idx].letex[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_EAST:
                tex_ptr = &levels[cur_level_idx].uetex[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_SOUTH:
                tex_ptr = &levels[cur_level_idx].lstex[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_SOUTH:
                tex_ptr = &levels[cur_level_idx].ustex[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_WEST:
                tex_ptr = &levels[cur_level_idx].lwtex[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_WEST:
                tex_ptr = &levels[cur_level_idx].uwtex[editor_selected_idx];
                break;
            case WALL_SIDE_UPPER_DIAG:
                tex_ptr = &levels[cur_level_idx].udtex[editor_selected_idx];
                break;
            case WALL_SIDE_LOWER_DIAG:
                tex_ptr = &levels[cur_level_idx].ldtex[editor_selected_idx];
                break;
            case MIDDLE_SPRITE:
                spr_ptr = &levels[cur_level_idx].m_sprite_index[editor_selected_idx];
                break;
            case CEIL_SPRITE:
                spr_ptr = &levels[cur_level_idx].c_sprite_index[editor_selected_idx];
                break;
            case FLOOR_SPRITE:
                spr_ptr = &levels[cur_level_idx].f_sprite_index[editor_selected_idx];
                break;
            case CELL_SPRITE:
                spr_ptr = &levels[cur_level_idx].sprite_index[editor_selected_idx];
                break;
            case N_SPRITE:
                spr_ptr = &levels[cur_level_idx].n_sprite_index[editor_selected_idx];
                break;
            case E_SPRITE:
                spr_ptr = &levels[cur_level_idx].e_sprite_index[editor_selected_idx];
                break;
            case S_SPRITE:
                spr_ptr = &levels[cur_level_idx].s_sprite_index[editor_selected_idx];
                break;
            case W_SPRITE:
                spr_ptr = &levels[cur_level_idx].w_sprite_index[editor_selected_idx];
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

    for(int x = 0; x < RENDER_WIDTH; x++) {
        prev_row_buf[x] = 1.0f;
    }

    for(int y = 0; y < FP_SCREEN_HEIGHT; y++) {
        if((y&1) == 0) {
            for(int x = 0; x < RENDER_WIDTH; x++) {
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
                r *= actual_lum;`
                g *= actual_lum;
                b *= actual_lum;
                u32 intr = (r*255.0f);
                u32 intg = (g*255.0f);
                u32 intb = (b*255.0f);

                cur_row_buf[x+1] = quarter_cur_lum;
                fb[x*FP_SCREEN_HEIGHT+y] = 0xFF000000 | (intr<<16) | (intg << 8) | (intb << 0);
            }
            u8* tmp = cur_row_buf;
            prev_row_buf = cur_row_buf;
            cur_row_buf = tmp;
        }
    }
}
#endif

int max_left_right_rays = -1;
int max_top_down_rays = -1;

void change_resolution() {



    cur_render_res_idx = requested_render_res;
    cur_render_scale = requested_render_scale;
    cur_output_width = resolutions[cur_render_res_idx][0];
    cur_output_height = resolutions[cur_render_res_idx][1];
    cur_render_width = cur_output_width / cur_render_scale;
    cur_render_height = cur_output_height / cur_render_scale;
    cur_fov = res_is_superwide[cur_render_res_idx] ? 120.0f : res_is_wide[cur_render_res_idx] ? 100.0f : 85.0f;
    
    int prev_use_vsync = use_vsync;
    int prev_fullscreen = fullscreen;
    use_vsync = requested_use_vsync;
    fullscreen = requested_fullscreen;
    if(prev_use_vsync != use_vsync) {
        platform_set_vsync(use_vsync);
    }
    //debug_printf("vsync %i\n", use_vsync);

    //SetConfigFlags(FLAG_VSYNC_HINT);


    platform_set_window_size(OUTPUT_WIDTH, OUTPUT_HEIGHT);

    if(prev_fullscreen != fullscreen) {
        if(fullscreen) {
            platform_set_fullscreen();
        } else {
            platform_set_windowed();
        }
    }
    if(draw_pix_pointers[0] != NULL) {
        free(seg_draw_bufs[0]);
        free(seg_draw_bufs[1]);
        free(draw_pix_pointers[0]);
        free(draw_pix_pointers[1]);
        free(zbuf_pointers[0]);
        //free(zbuf_pointers[1]);
        free(edit_id_buffer_pointers[0]);
        //free(edit_id_buffer_pointers[1]);
        platform_release_texture(draw_textures[0]);
        platform_release_texture(draw_textures[1]);
        platform_release_texture(seg_tex_handles[0]);
        platform_release_texture(seg_tex_handles[1]);
    }
    draw_pix_pointers[0] = my_malloc(sizeof(u32)*RENDER_HEIGHT*RENDER_WIDTH, "framebuffer");
    draw_pix_pointers[1] = my_malloc(sizeof(u32)*RENDER_HEIGHT*RENDER_WIDTH, "framebuffer");
    edit_id_buffer_pointers[0] = my_malloc(sizeof(edit_wall_id)*RENDER_HEIGHT*RENDER_WIDTH, "edit-buffer");
    //edit_id_buffer_pointers[1] = my_malloc(sizeof(edit_wall_id)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "edit-buffer");
    zbuf_pointers[0] = my_malloc(sizeof(u16)*RENDER_HEIGHT*RENDER_WIDTH, "z-buffer");
    //zbuf_pointers[1] = my_malloc(sizeof(u16)*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH, "z-buffer");

    unsigned int tex_handle0 = platform_create_texture(RENDER_HEIGHT, RENDER_WIDTH);
    unsigned int tex_handle1 = platform_create_texture(RENDER_HEIGHT, RENDER_WIDTH);

    draw_textures[0] = tex_handle0;
    draw_textures[1] = tex_handle1;
    // janky framebuffer texture :)
    //textures[NUM_TEXTURES-1] = draw_pix;



    max_top_down_rays  = RENDER_WIDTH + 2*RENDER_HEIGHT;
    max_left_right_rays = 2*RENDER_WIDTH + RENDER_HEIGHT;
    int top_down_ray_buffer_size = sizeof(u32)*max_top_down_rays*RENDER_HEIGHT;
    int left_right_ray_buffer_size = sizeof(u32)*max_left_right_rays*RENDER_WIDTH;
    
    seg_draw_bufs[0] = my_malloc(top_down_ray_buffer_size, "seg01 buffer");
    seg_draw_bufs[1] = my_malloc(left_right_ray_buffer_size, "seg23 buffer");
    transpose_buf = my_malloc(max(max_left_right_rays, max_top_down_rays) * max(RENDER_HEIGHT, RENDER_WIDTH) * sizeof(u32), "transpose buffer");
    seg_tex_handles[0] = platform_create_texture(max_top_down_rays, RENDER_HEIGHT);
    seg_tex_handles[1] = platform_create_texture(max_left_right_rays, RENDER_WIDTH);
    //for(int y = 0; y < RENDER_HEIGHT; y++) {
    //    float cy = ((((float)y) / RENDER_HEIGHT))*255.0f;
    //    int iy = cy;
    //    iy &= 0xFF;
    //    for(int x = 0; x < max_top_down_rays; x++) {
    //        seg_draw_bufs[0][y*max_top_down_rays+x] = (0xFF<<24)|(iy<<16)|(iy<<8)|(iy);
    //        seg_draw_bufs[1][y*max_top_down_rays+x] = (0xFF<<24)|(iy<<16)|(iy<<8)|(iy);
    //    }
    //}
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

//#define DRAW_INVENTORY

float player_height = STANDING_HEIGHT;

void init_game();

int launch_in_edit_mode = 0;
int killed_all_the_foxs = 0;




void run_game() {
    platform_begin_frame();
    if(!platform_is_window_focused()) {
        platform_begin_drawing();
        platform_end_drawing();
        return;
    }
    //printf("editor mode %i\n", editor_mode_enabled);
    float frame_start_time = get_abs_time();

    Vector2 mouse_delta = platform_get_mouse_delta();


    if(editor_mode_enabled) {
        platform_show_cursor();
    } else {
        platform_hide_cursor();
        platform_set_mouse_position(OUTPUT_WIDTH/2, OUTPUT_HEIGHT/2);
    }

    float frame_time_ms = platform_get_frame_time()*1000.0f;
    if(requested_render_res != cur_render_res_idx || requested_render_scale != cur_render_scale || requested_use_vsync != use_vsync || requested_fullscreen != fullscreen) {
        float prev_pitch = player_pitch;
        change_resolution();
        player_pitch = prev_pitch;
    }

    clear_requested_sprites();

    update_particles();
    

    if(cur_game_state == DEAD || cur_game_state == WON) {
        if(platform_is_key_pressed(KEY_ENTER)) {
            init_game();
        }
    } else {
        update_player(frame_time_ms, mouse_delta);
    }
    
    //u16* zbuffer_prev_pix = zbuf_pointers[(frame+1)&0b1];
    //edit_wall_id* edit_prev_buf = edit_id_buffer_pointers[(frame+1)&0b1];
    u32* upload_draw_pix = draw_pix_pointers[(frame+1)&0b1];

    
    u32* render_draw_pix = draw_pix_pointers[frame&0b1];
    u16* zbuffer_draw = zbuf_pointers[0];//frame&0b1];
    edit_wall_id* edit_draw_buf = edit_id_buffer_pointers[0];//frame&0b1];


    int upload_tex = draw_textures[frame&0b1];
    int draw_tex = draw_textures[(frame+1)%1];


    if(platform_is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_pos = platform_get_mouse_position();
        handle_click(edit_draw_buf, RENDER_WIDTH*mouse_pos.x/OUTPUT_WIDTH, RENDER_HEIGHT*mouse_pos.y/OUTPUT_HEIGHT);
    }
    if (launch_in_edit_mode && platform_is_key_pressed(KEY_E)) {
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
        
        if(platform_is_key_down(KEY_CONTROL)) { // | platform_is_key_down(KEY_RSHIFT)) {
            requested_render_scale <<= 1;
            if(requested_render_scale > 4) {
                requested_render_scale = 1;
            }
        } else if(platform_is_key_down(KEY_SHIFT)) {
            requested_render_res++;
            if(requested_render_res >= NUM_RESOLUTIONS) {
                requested_render_res = 0;
            }
        }
        
    } else if (platform_is_key_pressed(KEY_V) && platform_is_key_down(KEY_SHIFT)) {
        requested_use_vsync = !requested_use_vsync;
    } else if (platform_is_key_pressed(KEY_F) && platform_is_key_down(KEY_SHIFT)) {
        requested_fullscreen = !requested_fullscreen;
    }



    static float last_other_player_x, last_other_player_y, last_other_player_z;
    static int got_other_player_pos;
    // send and receive every 30 frames
    //float position[4] = {player_x, player_y, player_z, player_ang};
    //float other_position[4];
    //if(udp_frame(udp_conn, position, other_position, 1, ((frame&0b11)==0))) {
    //    got_other_player_pos = 1;
    //    last_other_player_x = other_position[0];
    //    last_other_player_y = other_position[1];
    //    last_other_player_z = other_position[2];
    //    // we got a packet baybee
    //}

    
    if(!editor_mode_enabled) {
        step_entities(player_x, player_y, player_z);
    }
    draw_entities();
    if(got_other_player_pos) {
        request_draw_sprite(last_other_player_x, last_other_player_y, last_other_player_z-cur_player_height, OTHER_PLAYER_ENTITY_ID, 20);
    }

    float seconds = get_running_time();
    float quarter_seconds = seconds*4;
    int iquarter_seconds = quarter_seconds;

    skybox_u_offset = (seconds); // scrolls every 2 seconds

    // avoid some math issues with a zero or too small pitch
    if(my_fabsf(player_pitch) < 0.001f) {
        if(player_pitch < 0.0f) {
            player_pitch = -0.001f;
        } else {
            player_pitch = 0.001f;
        }
    }


    camera cam = mk_camera(
        player_x, player_z, player_y, 
        player_pitch,
        player_yaw, 
        RENDER_WIDTH, RENDER_HEIGHT,
        NEAR_PLANE_DIST, FAR_PLANE_DIST
    );


    int flash_frame = iquarter_seconds&0b1;
    platform_begin_drawing(); {   

        float scale = ((float)OUTPUT_WIDTH/((float)RENDER_WIDTH));
        platform_draw_texture(draw_tex, (Vector2){.x=OUTPUT_WIDTH/2,.y=OUTPUT_HEIGHT/2}, 90.0f, scale, RENDER_HEIGHT, RENDER_WIDTH);

        for(int i = 0; i < RENDER_HEIGHT*RENDER_WIDTH; i++) {
            zbuffer_draw[i] = DARK_DIST*FIXED_POINT_MULT;
            edit_draw_buf[i] = ENTITY_EDIT_ID(INVALID_ENTITY_ID);
        }

        // fixes a bug at angle zero
        // the DDA algorithm had equal distance for both x and y steps
        // and picking one by the normal rules causes a bug in certain places
        // this removes any need for special casing inside the raycast function
        //if(fabsf(player_ang) < 0.01f) {
        //    player_ang = 0.01f;
        //}

    #ifdef DRAW_INVENTORY
        #define NUM_INV_SLOTS 8
        float inv_box_size = CLAMP((FP_SCREEN_HEIGHT/12.0f), 52.0f, 128.0f);
        float inventory_size = inv_box_size*NUM_INV_SLOTS;
        float side_margins = (FP_SCREEN_WIDTH-inventory_size);
        float side_margin = side_margins/2.0f;
        float bot_margin = FP_SCREEN_HEIGHT/40.0f;
        for(int i = 0; i < NUM_INV_SLOTS; i++) {
            request_draw_screen_space_sprite(
                FP_SCREEN_WIDTH-i*inv_box_size - side_margin, 
                FP_SCREEN_WIDTH-(i+1)*inv_box_size - side_margin, 
                FP_SCREEN_HEIGHT-inv_box_size-bot_margin, FP_SCREEN_HEIGHT-bot_margin, 
                INVENTORY_BOX_SPRITE);
        }
    #endif 
        if(got_revolver) {
            float revolver_size = CLAMP((RENDER_HEIGHT/2.5f), 128.0f, 512.0f);
            float base_height = RENDER_HEIGHT-revolver_size;
            float recoil_height = base_height-revolver_size;
            float pos = lerp(base_height, recoil_height, (revolver_firing/REVOLVER_FIRE_DURATION));

            request_draw_screen_space_sprite(
                0*revolver_size, 
                1*revolver_size, 
                pos,//FP_SCREEN_HEIGHT-revolver_size, 
                pos+revolver_size,//FP_SCREEN_HEIGHT, 
                REVOLVER_FIRST_PERSON_SPRITE_INDEX, BRANDISHED_ITEM_ENTITY_ID);
            
        }
        draw_particles();
    
      
        int needs_join = 0;
        if(cur_game_state == DEAD || cur_game_state == WON) {
            float du = 1.0f / RENDER_WIDTH;
            float dv = 1.0f / RENDER_HEIGHT;
            u32* bmp = sprites[(cur_game_state == DEAD) ? YOU_DIED_IDX : YOU_WIN_IDX];
            for(int x = 0; x < RENDER_WIDTH; x++) {
                float u = x*du;
                u32* column = get_extra_big_texture_column(bmp, u);

                draw_extra_big_sprite_col(upload_draw_pix, column, x);
            }
        } else {


            needs_join = 1;
            launch_render_frame(render_draw_pix, edit_draw_buf, zbuffer_draw,
                0, RENDER_WIDTH, flash_frame, 
                &levels[cur_level_idx], player_x, player_y, player_z, 
                player_yaw, pitch,
                editor_mode_enabled, editor_selected_idx, editor_selected_side
            );

            // step 1:
            // calculate triangle position in screen-space
            // requires calculating segments

            // a segment represents a 90 degree quadrant around the player (0-90, 90-180, 180-270, 270-360)
            // 1 - how many rays in a quadrant are necessary to fill the screen area covered by that quadrant 
            // (generally 1 ray per opposite side screen border)
            // 2 - screen-space area covered by that quadrant
            // 3 - world-space ray bounds on the far side.

            // to raycast inside a segment
            // we do  for(int i = 0; i < ray_count; i++) { ray = float3_lerp(i/ray_count, first_ray_end_loc, last_ray_end_loc); }
            // then we normalize that ray direction


            float3 world_vp = calc_vanishing_point_world(cam);
            float2 screen_vp = project_vanishing_point_world_to_screen(
                cam, world_vp
            );

            // essentially zero initializing
            // probably zero initialize would be fine actually
            // since a 0 ray count effectively short-circuits everything


            segment segments[4] = { 0 };

            if (screen_vp.y < cam.dims.y) {
                // top segment
                segments[0] = get_segment_parameters(
                    0,
                    cam, screen_vp, cam.dims.y - screen_vp.y, 
                    ((float2){.x=0.0f, .y = 1.0f}), 1, MAX_WALL_HEIGHT, max_top_down_rays);
            }

            if(screen_vp.y > 0) {
                segments[1] = get_segment_parameters(
                    1,
                    cam, screen_vp, screen_vp.y,
                    ((float2){.x=0.0f, .y=-1.0f}), 1, MAX_WALL_HEIGHT, max_top_down_rays
                );
            }

            if(screen_vp.x < cam.dims.x) {
                segments[2] = get_segment_parameters(
                    2,
                    cam, screen_vp,  cam.dims.x - screen_vp.x, 
                    ((float2){.x=1.0f, .y=0.0f}), 0, MAX_WALL_HEIGHT, max_left_right_rays);
            }

            if (screen_vp.x > 0) {
                segments[3] = get_segment_parameters(
                    3,
                    cam, screen_vp, screen_vp.x, 
                    ((float2){.x=-1.0f, .y=0.0f}), 0, MAX_WALL_HEIGHT, max_left_right_rays);
            }

            for(int seg_idx = 0; seg_idx < 4; seg_idx++) {
                int next_free_pix_min, next_free_pix_max;
                if (seg_idx == 0) {
                    next_free_pix_min = CLAMP(my_roundf(screen_vp.y), 0, RENDER_HEIGHT-1);
                    next_free_pix_max = RENDER_HEIGHT-1;
                } else if (seg_idx == 1) {
                    next_free_pix_min = 0;
                    next_free_pix_max = CLAMP(my_roundf(screen_vp.y), 0, RENDER_HEIGHT-1);
                } else if (seg_idx == 3) {
                    next_free_pix_min = 0;
                    next_free_pix_max = CLAMP(my_roundf(screen_vp.x), 0, RENDER_WIDTH-1);
                } else {
                    next_free_pix_min = CLAMP(my_roundf(screen_vp.x), 0, RENDER_WIDTH-1);
                    next_free_pix_max = RENDER_WIDTH-1;
                }
                segments[seg_idx].next_free_pixel_min = next_free_pix_min;
                segments[seg_idx].next_free_pixel_max = next_free_pix_max;
            }

            mat4 world_to_screen_mat = get_world_to_screen_matrix(cam);
            int update_seg_textures[2] = {0,0};
            for(int seg_idx = 0; seg_idx < 4; seg_idx++) {
                int ray_offset = 0;
                if (seg_idx&1) {
                    // segments 1 and 3's offsets are segment 0 and 1 respectively
                    ray_offset = segments[seg_idx-1].ray_count;
                }
                if(segments[seg_idx].ray_count) { 
                    update_seg_textures[seg_idx>>1] = 1;
                }
                //execute_rays_in_segment(
                //    seg_draw_bufs[seg_idx>>1], ray_offset,
                //    segments[seg_idx], cam, world_to_screen_mat,
                //    (seg_idx > 1) ? 0 : 1, &levels[cur_level_idx], (seg_idx < 2) ? RENDER_HEIGHT : RENDER_WIDTH
                //);
            }
            for(int seg_idx = 0; seg_idx < 4; seg_idx++) {
                segment seg = segments[seg_idx];
                if(seg.ray_count > 0) {

                    printf("transpose seg buffer %i\n", seg_idx);
                    int src_height = (seg_idx < 2) ? RENDER_HEIGHT : RENDER_WIDTH;
                    int seg_height = ((seg.next_free_pixel_max+1) - seg.next_free_pixel_min);
                    int seg_width = seg.ray_count;
                    int y_offset = src_height-1 - seg.next_free_pixel_max;


                    u32* src_pix_arr = seg_draw_bufs[seg_idx>>1];
                    u32* dst_pix_arr = transpose_buf;
                    int x_offset = (seg_idx & 1) ? (segments[seg_idx-1].ray_count) : 0;

                    int x1 = x_offset; int y1 = y_offset;
                    int w = seg_width; int h = seg_height;
                    for(int x = 0; x < w; x++) {
                        for(int y = 0; y < h; y++) {
                            u32 rgba = src_pix_arr[(x1 + x) * h + (y + y1)];
                            dst_pix_arr[y*w+x] = rgba;
                        }
                    }

                    platform_update_texture(seg_tex_handles[seg_idx>>1], dst_pix_arr, x1, y1, w, h);


                    //platform_update_texture(
                    //    seg_tex_handles[i], seg_draw_bufs[i],
                    //    (i < 2) ? max_top_down_rays : max_left_right_rays, 
                    //    (i < 2) ? RENDER_HEIGHT : RENDER_WIDTH
                    //);
                }
            }
            //if(update_segs[0]) {
            //    platform_update_texture(seg01_tex_handle, seg01_offscreen_buf, max_top_down_rays, RENDER_HEIGHT);
            //}
            //if(update_segs[1]) {
            //    platform_update_texture(seg23_tex_handle, seg23_offscreen_buf, max_left_right_rays, RENDER_WIDTH);
            //}

            
            //max_left_right_rays = 2*RENDER_WIDTH + RENDER_HEIGHT;
            //max_top_down_rays  = RENDER_WIDTH + 2*RENDER_HEIGHT;
            //int left_right_ray_buffer_size = max_left_right_rays*RENDER_WIDTH;
            //int top_down_ray_buffer_size = max_top_down_rays*RENDER_HEIGHT

            float3 seg_v0 = adjust_screen_pixel_for_mesh(screen_vp, cam.dims);


            float scales[4] = {
                segments[0].ray_count / max_top_down_rays,
                segments[1].ray_count / max_top_down_rays,
                segments[2].ray_count / max_left_right_rays,
                segments[3].ray_count / max_left_right_rays
            };
            float offsets[4] = {
                0.0f, scales[0], 0.0f, scales[2],
            };

            for(int seg_idx = 0; seg_idx < 4; seg_idx++) {
                float u_offset = offsets[seg_idx];
                float u_scale = scales[seg_idx];
                segment seg = segments[seg_idx];
                if(seg.ray_count > 0) {
                    printf("draw seg %i\n", seg_idx);

                    float3 seg0_v1 = adjust_screen_pixel_for_mesh(seg.max_screen, cam.dims);
                    float3 seg0_v2 = adjust_screen_pixel_for_mesh(seg.min_screen, cam.dims);
                    
                    float seg_attributes[7*3] = {
                        seg_v0.x, seg_v0.y, seg_v0.z, 0.0f, 0.0f, 1.0f, seg_idx,
                        seg0_v1.x, seg0_v1.y, seg0_v1.z, 1.0f, 0.0f, 0.0f, seg_idx,
                        seg0_v2.x, seg0_v2.y, seg0_v2.z, 0.0f, 1.0f, 0.0f, seg_idx,

                    };
                    
                    //platform_draw_segment(
                    //    seg_tex_handles[seg_idx>>1], 
                    //    seg_idx, 
                    //    seg_attributes,
                    //    offsets, scales
                    //);
                }  
            }
            

        }
        
        switch(render_mode) {
            case EDITOR_BUFFER:
                join_render_frame();
                needs_join = 0;    
                for(int i = 0; i < RENDER_HEIGHT*RENDER_WIDTH; i++) {
                    u16 edit_draw_id = edit_draw_buf[i].full_val;
                    //u16 map_idx = edit_draw_buf[i]&0b1111111111;
                    //u16 side = (edit_draw_buf[i]>>10)&0b11111;
                    u8 r = ((edit_draw_id)>>10)&0b11111;
                    u8 g = ((edit_draw_id)>>5)&0b11111;
                    u8 b = ((edit_draw_id)>>0)&0b11111;
                    upload_draw_pix[i] = (0xFF<<24)|(r<<16)|(g<<8)|b; // 16 bits
                }
                platform_update_texture(upload_tex, (u32*)upload_draw_pix, 0, 0, RENDER_HEIGHT, RENDER_WIDTH);
                break;
            case PIXEL_BUFFER:
                //draw_screen_segments();

                
                platform_update_texture(upload_tex, (u32*)upload_draw_pix, 0, 0, RENDER_HEIGHT, RENDER_WIDTH);
                break;
            case Z_BUFFER:
                join_render_frame();
                needs_join = 0;
                for(int i = 0; i < RENDER_HEIGHT*RENDER_WIDTH; i++) {
                    float z = zbuffer_draw[i]/FIXED_POINT_MULT;
                    float normalized = (z-NEAR_PLANE_DIST)/(DARK_DIST-NEAR_PLANE_DIST);
                    int byte_z = normalized*255;
                    ((u32*)upload_draw_pix)[i] = (0xFF000000 | (byte_z<<16) | (byte_z<<8) | byte_z);
                }
                platform_update_texture(upload_tex, (u32*)upload_draw_pix, 0, 0, RENDER_HEIGHT, RENDER_WIDTH);
                break;

        }
        if(needs_join) {
            join_render_frame();
        }




        float avg_frame_time = (prev_frame_time + frame_time_ms)/2.0f;
        prev_frame_time = frame_time_ms;
        //char buf[80]; 
        //debug_printf(buf, "%i %i -> %i %i FOV%.0f %s", cur_render_width, cur_render_height, cur_output_width, cur_output_height, cur_fov, use_vsync ? "vsync" : "");
        //platform_draw_text(buf, (Vector2){.x = 5, .y = 5}, 18, 1, RED);
        //platform_draw_text(buf, (Vector2){.x = 5, .y = 20}, 18, 1, RED);
        //debug_printf(buf, "%.2f %.2f %.2f %.2f\n", player_x, player_y, player_z, player_ang*RAD2DEG);
        //platform_draw_text(buf, (Vector2){.x = 5, .y = 35}, 18, 1, RED);
        //debug_printf("p %f %f %f\n", player_x, player_y, player_z);
        debug_printf("y %f p %f\n", player_yaw, player_pitch);
        debug_printf("cp %f %f %f\n", cam.pos.x, cam.pos.y, cam.pos.z);
        //debug_printf("cf %f %f %f\n", cam.forward.x, cam.forward.z, cam.forward.y);
        //debug_printf("cr %f %f %f\n", cam.right.x, cam.right.z, cam.right.y);
        //debug_printf("cu %f %f %f\n", cam.up.x, cam.up.z, cam.up.y);
        //debug_printf("vp_world %f %f %f\n", vp_world.x, vp_world.y, vp_world.z);
        //debug_printf("vp_screen %f %f\n", vp_screen.x, vp_screen.y);
        //debug_printf("seg %f,%f %f,%f\n", segs[0].min_screen.x, segs[0].min_screen.y, segs[0].max_screen.x, segs[0].max_screen.y);
        //debug_printf("seg ws min/max %f,%f %f,%f\n", 
        //    segs[0].cam_local_plane_ray_min.x, segs[0].cam_local_plane_ray_min.y,
        //    segs[0].cam_local_plane_ray_max.x, segs[0].cam_local_plane_ray_max.y
        //);
        debug_printf("%4.0f ms %4.0f fps\n", avg_frame_time, 1000.0f/avg_frame_time);
    } platform_end_drawing();

    


#ifdef CAMERA_TEXTURE
    if(0) {
        int scale_y = FP_SCREEN_HEIGHT/32;
        int scale_x = FP_SCREEN_WIDTH/32;
        for(int y = 0; y < 32; y++) {
            for(int x = 0; x < 32; x++) {
                int cr = 0;
                int cg = 0;
                int cb = 0;
                for(int sy = 0; sy < scale_y; sy++) {
                    for(int sx = 0; sx < scale_x; sx++) {
                        int fb_y = x*scale_y+sy;
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
#endif
    
    frame++;
    float frame_end_time = get_abs_time();
    running_time += (frame_end_time - frame_start_time);
    platform_end_frame();
}


#define MAP_SAVE_FILE "map_save"
void init_game() {
    entities_woke = 0;
    cur_game_state = FIGHTING;
    got_whiskey = 0;
    got_revolver = 0;
    init_particle_system();
    init_raycast_module();
    init_entities_module();

    if(load_resources()) {
        debug_printf("Error loading resources\n");
        exit(1);

    } 
    debug_printf("Loading map data...\n");
    int num_loaded_bytes;
    u8* loaded_bytes = platform_load_file_data(MAP_SAVE_FILE, &num_loaded_bytes);
    
    if(num_loaded_bytes == sizeof(level)*NUM_LEVELS) {
        levels = (level*)loaded_bytes;
        init_level(0);
    } else if(num_loaded_bytes > 0) {
        compressed* comp = (compressed*)loaded_bytes;

        if(comp->uncompressed_size != sizeof(level)*NUM_LEVELS) {
            debug_printf("Uncompressed size doesn't match expectations");
            exit(1);

        }
        u8* decompressed_ptr;
        int decompressed_size = decompress(comp, &decompressed_ptr);
        if(decompressed_size == -1) {
            debug_printf("failed to decompress, header mismatch? :(");
            exit(1);
        }
        levels = (level*)decompressed_ptr;
        init_level(0);
    } else {
        levels = my_malloc(sizeof(level)*NUM_LEVELS, "level data");
        init_level(1);
    }
        debug_printf("Done.\n");

    
    if(!killed_all_the_foxs) { //launch_in_edit_mode) {
        int max_foxs = 30;
        int ticks = 1;
        for(int x = 1; x < 16; x++) {
            for(int y = 1; y < 31; y++) {
                if(urand() % 25 == 0) {
                    spawn_entity(FOX, x, y, 8.5f, 0.0f, ticks++);
                    if(max_foxs-- == 0) {
                        goto no_more_foxs;
                    }
                }
                //spawn_entity(FOX, x+0.5f, y, 8.5f, 0.0f, ticks++);
                //spawn_entity(FOX, x, y+0.5f, 8.5f, 0.0f, ticks++);
                //spawn_entity(FOX, x+0.5f, y+0.5f, 8.5f, 0.0f, ticks++);
            }
        }
        no_more_foxs:;
    }
}


// predeclare 'winmain'
#ifdef PLATFORM_WEB
int WinMain();
#else 
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow);
#endif

// entry point
#ifdef DEBUG
int main(int argc, char** argv) {
#elif CL_COMPILER
int main(int argc, char** argv) {
#elif PLATFORM_WEB
int main(int argc, char** argv) {
#else
int atexit(void (*fn)(void)) { return 0; }
int mainCRTStartup(void) { 
#endif

#ifdef PLATFORM_WEB
    WinMain();
#else 
    HINSTANCE hInst = GetModuleHandleA(NULL);
    ExitProcess(WinMain(hInst, NULL, NULL, SW_SHOW));
#endif
}

#ifdef PLATFORM_WEB
int WinMain() {
#else
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
#endif

    char* args = GetCommandLineA();

    if(strstr(args, "--editor") != NULL) {
        int res = strcmp(args, "--editor");
        printf("launching in edit mode\n");
        launch_in_edit_mode = 1;
        editor_mode_enabled = 1;
    }
    
    init_game();
    //printf("initting window with %i %i\n", OUTPUT_WIDTH, OUTPUT_HEIGHT);
    platform_init_window(OUTPUT_WIDTH, OUTPUT_HEIGHT, "raycaster");
    //change_resolution();
    frame = 0;

    platform_hide_cursor();


#ifdef PLATFORM_WEB
    emscripten_set_main_loop(run_game, 0, 1);
#else 
    while(!platform_window_should_close()) {
        if(platform_is_key_pressed(KEY_B)) {
            //spawn_entity(FOX, player_x, player_y, player_z, 0, 2);
        }
        if(frame == 5) {

            // clean up some cruft
            SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
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
    levels[cur_level_idx].start_ang = player_yaw;


    if(launch_in_edit_mode) {
        debug_printf("Saving map file...\n");
        size_t level_size = sizeof(level)*NUM_LEVELS;
        u8* level_data = (u8*)levels;

        compressed* comp = compress(level_data, sizeof(level)*NUM_LEVELS);
        size_t comp_size_bytes = ((comp->num_opcodes+7)>>3)+comp->num_operand_bytes;
        //debug_printf("Compressed %i down to %llu bytes\n", comp->uncompressed_size, sizeof(compressed)+comp_size_bytes);

        u8* decomp;
        int decompressed_bytes = decompress(comp, &decomp);
        if(decompressed_bytes == -1) {
            debug_printf("decompress not enough bytes\n");
            exit(1);
            return 1;
        }
        for(size_t i = 0; i < level_size; i++) {
            if(level_data[i] != decomp[i]) {
                debug_printf("miscompare at %zu\n", i);
                exit(1);
            return 1;
            }
        }

        if(!platform_save_file_data(MAP_SAVE_FILE, comp, sizeof(compressed)+comp_size_bytes)) {
        //if(!platform_save_file_data(MAP_SAVE_FILE, levels, sizeof(level)*NUM_LEVELS)) {
            debug_printf("Error saving file :(\n");
            return 1;
        }
        debug_printf("Done.\n");
    }
    return 0;


}

void game_over() {
    cur_game_state = DEAD;
}

void you_win() {
    cur_game_state = WON;
    killed_all_the_foxs = 1;
}